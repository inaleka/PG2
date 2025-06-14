//
// Created by dimonchik on 5/17/25.
//

#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H
#include <opencv2/opencv.hpp>
#include <tuple>
#include "Mesh.hpp"
#include "assert.h"
#include "ShaderProgram.hpp"

class HeightMap {
    glm::vec2 get_subtex_st(const int x, const int y) {
        return glm::vec2(x * 1.0f / 16, y * 1.0f / 16);
    }

    // choose subtexture based on height
    glm::vec2 get_subtex_by_height(float height) {
        if (height > 0.9)
            return get_subtex_st(0, 4); // snow
        if (height > 0.8)
            return get_subtex_st(5, 2); // black stone
        if (height > 0.5)
            return get_subtex_st(0, 1); // rock
        if (height > 0.3)
            return get_subtex_st(2, 0); // soil
        return get_subtex_st(0, 0); // grass
    }
public:
    HeightMap() {};

    std::vector<Mesh> GenHeightMap(
        const cv::Mat& hmap,
        const unsigned int mesh_step_size,
        float heightScale,
        double& minVal,
        double& maxVal,
        const float& scaleXZ,
        ShaderProgram& shader
    ) {
        std::vector<Mesh> meshes;

        std::cout << "Note: heightmap size:" << hmap.size << ", channels: " << hmap.channels() << std::endl;

        if (hmap.channels() != 1) {
            std::cerr << "WARN: requested 1 channel, got: " << hmap.channels() << std::endl;
        }

        // Compute min/max for dynamic normalization
        cv::minMaxLoc(hmap, &minVal, &maxVal);
        std::cout << "Heightmap raw min: " << minVal << " max: " << maxVal << std::endl;

        // Avoid division by zero
        double denom = (maxVal - minVal > 1e-5) ? (maxVal - minVal) : 1.0;

        // Center mesh in world space
        float x_offset = (hmap.cols - mesh_step_size) / 2.0f;
        float z_offset = (hmap.rows - mesh_step_size) / 2.0f;

        for (unsigned int x_coord = 0; x_coord < (hmap.cols - mesh_step_size); x_coord += mesh_step_size)
        {
            for (unsigned int z_coord = 0; z_coord < (hmap.rows - mesh_step_size); z_coord += mesh_step_size) {
                std::vector<Vertex> vertices;
                std::vector<GLuint> indices;
                // Normalize to [0,1] using image min/max, then center to [-1,1]
                float h0 = (hmap.at<uchar>(cv::Point(x_coord, z_coord)) - minVal) / denom;
                float h1 = (hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord)) - minVal) / denom;
                float h2 = (hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord + mesh_step_size)) - minVal) / denom;
                float h3 = (hmap.at<uchar>(cv::Point(x_coord, z_coord + mesh_step_size)) - minVal) / denom;

                // Center heights to [-1, 1]
                float ch0 = (h0 - 0.5f) * 2.0f;
                float ch1 = (h1 - 0.5f) * 2.0f;
                float ch2 = (h2 - 0.5f) * 2.0f;
                float ch3 = (h3 - 0.5f) * 2.0f;

                glm::vec3 p0((x_coord - x_offset) * scaleXZ, ch0 * heightScale, (z_coord - z_offset) * scaleXZ);
                glm::vec3 p1((x_coord + mesh_step_size - x_offset) * scaleXZ, ch1 * heightScale, (z_coord - z_offset) * scaleXZ);
                glm::vec3 p2((x_coord + mesh_step_size - x_offset) * scaleXZ, ch2 * heightScale, (z_coord + mesh_step_size - z_offset) * scaleXZ);
                glm::vec3 p3((x_coord - x_offset) * scaleXZ, ch3 * heightScale, (z_coord + mesh_step_size - z_offset) * scaleXZ);

                float max_h = std::max({ h0, h1, h2, h3 });

                glm::vec2 tc0 = get_subtex_by_height(max_h);
                glm::vec2 tc1 = tc0 + glm::vec2(1.0f / 16, 0.0f);
                glm::vec2 tc2 = tc0 + glm::vec2(1.0f / 16, 1.0f / 16);
                glm::vec2 tc3 = tc0 + glm::vec2(0.0f, 1.0f / 16);

                // normals for both triangles, CCW
                glm::vec3 n1 = glm::normalize(glm::cross(p1 - p0, p2 - p0)); // for p1
                glm::vec3 n2 = glm::normalize(glm::cross(p2 - p0, p3 - p0)); // for p3
                glm::vec3 navg = glm::normalize(n1 + n2);                    // average for p0, p2 - common

                // place vertices and ST to mesh
                vertices.emplace_back(Vertex{ p0, navg, tc0 });
                vertices.emplace_back(Vertex{ p1, n1,   tc1 });
                vertices.emplace_back(Vertex{ p2, navg, tc2 });
                vertices.emplace_back(Vertex{ p3, n2,   tc3 });

                // place indices
                size_t base_idx = vertices.size() - 4;
                indices.insert(indices.end(), {
                    static_cast<GLuint>(base_idx + 2), static_cast<GLuint>(base_idx + 1), static_cast<GLuint>(base_idx + 0),
                    static_cast<GLuint>(base_idx + 3), static_cast<GLuint>(base_idx + 2), static_cast<GLuint>(base_idx + 0)
                    });
                meshes.emplace_back(Mesh(GL_TRIANGLES, shader, vertices, indices,
                    glm::vec3(0.0f), glm::vec3(0.0f)));
            }
        }

        return meshes;
    }

    // x, z: world-space coordinates
    float getWorldHeightAt(float x, float z, const cv::Mat& hmap, float heightScale) {
        // Convert world x,z to heightmap pixel coordinates
        int mesh_step_size = 30; // whatever you use in mesh generation (e.g. 30)
        float x_offset = (hmap.cols - mesh_step_size) / 2.0f;
        float z_offset = (hmap.rows - mesh_step_size) / 2.0f;

        // Convert to image coordinates
        int img_x = static_cast<int>(x * mesh_step_size + x_offset);
        int img_z = static_cast<int>(z * mesh_step_size + z_offset);

        // Clamp to valid range
        img_x = std::max(0, std::min(img_x, hmap.cols - 1));
        img_z = std::max(0, std::min(img_z, hmap.rows - 1));

        double minVal, maxVal;
        cv::minMaxLoc(hmap, &minVal, &maxVal);
        double denom = (maxVal - minVal > 1e-5) ? (maxVal - minVal) : 1.0;
        float h = (hmap.at<uchar>(cv::Point(img_x, img_z)) - minVal) / denom;
        float ch = (h - 0.5f) * 2.0f;
        return ch * heightScale;
    }
};

#endif HEIGHTMAP_H
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
private:
    const int HEIGHT_LEVELS = 16;

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

        // ��������� min/max ��� ������������
        cv::minMaxLoc(hmap, &minVal, &maxVal);
        std::cout << "Heightmap raw min: " << minVal << " max: " << maxVal << std::endl;

        // �������� ������� �� ����
        double denom = (maxVal - minVal > 1e-5) ? (maxVal - minVal) : 1.0;

        // ���������� ����� � ������� ������������
        float x_offset = (hmap.cols - mesh_step_size) / 2.0f;
        float z_offset = (hmap.rows - mesh_step_size) / 2.0f;

        for (unsigned int x_coord = 0; x_coord < (hmap.cols - mesh_step_size); x_coord += mesh_step_size)
        {
            for (unsigned int z_coord = 0; z_coord < (hmap.rows - mesh_step_size); z_coord += mesh_step_size) {
                std::vector<Vertex> vertices;
                std::vector<GLuint> indices;

                // �������� ������ ��� 4 ����� �����
                float h0 = (hmap.at<uchar>(cv::Point(x_coord, z_coord)) - minVal) / denom;
                float h1 = (hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord)) - minVal) / denom;
                float h2 = (hmap.at<uchar>(cv::Point(x_coord + mesh_step_size, z_coord + mesh_step_size)) - minVal) / denom;
                float h3 = (hmap.at<uchar>(cv::Point(x_coord, z_coord + mesh_step_size)) - minVal) / denom;

                // ����������� ������������� � ��������� ���������� ���������������
                auto enhanceHeight = [](float h) -> float {
                    // ����������� �������� (�������� � �����)
                    h = (h - 0.5f) * 2.5f + 0.5f; // ����������� �������� � 2.5 ����
                    h = std::max(0.0f, std::min(1.0f, h)); // ������������ [0,1]

                    // ��������� ��������� ������� ��� ������� ����������������
                    h = std::pow(h, 0.7f); // ������ �������� ����� �������

                    return h;
                    };

                h0 = enhanceHeight(h0);
                h1 = enhanceHeight(h1);
                h2 = enhanceHeight(h2);
                h3 = enhanceHeight(h3);

                // ����������� � ���������� ������ (0 - HEIGHT_LEVELS-1)
                int level0 = static_cast<int>(h0 * HEIGHT_LEVELS);
                int level1 = static_cast<int>(h1 * HEIGHT_LEVELS);
                int level2 = static_cast<int>(h2 * HEIGHT_LEVELS);
                int level3 = static_cast<int>(h3 * HEIGHT_LEVELS);

                // ������������ ������
                level0 = std::max(0, std::min(level0, HEIGHT_LEVELS - 1));
                level1 = std::max(0, std::min(level1, HEIGHT_LEVELS - 1));
                level2 = std::max(0, std::min(level2, HEIGHT_LEVELS - 1));
                level3 = std::max(0, std::min(level3, HEIGHT_LEVELS - 1));

                // ����������� ������ � ������
                float discrete_h0 = (level0 / float(HEIGHT_LEVELS - 1)) * 2.0f - 1.0f; // [-1, 1]
                float discrete_h1 = (level1 / float(HEIGHT_LEVELS - 1)) * 2.0f - 1.0f;
                float discrete_h2 = (level2 / float(HEIGHT_LEVELS - 1)) * 2.0f - 1.0f;
                float discrete_h3 = (level3 / float(HEIGHT_LEVELS - 1)) * 2.0f - 1.0f;

                // ������� ������
                glm::vec3 p0((x_coord - x_offset) * scaleXZ, discrete_h0 * heightScale, (z_coord - z_offset) * scaleXZ);
                glm::vec3 p1((x_coord + mesh_step_size - x_offset) * scaleXZ, discrete_h1 * heightScale, (z_coord - z_offset) * scaleXZ);
                glm::vec3 p2((x_coord + mesh_step_size - x_offset) * scaleXZ, discrete_h2 * heightScale, (z_coord + mesh_step_size - z_offset) * scaleXZ);
                glm::vec3 p3((x_coord - x_offset) * scaleXZ, discrete_h3 * heightScale, (z_coord + mesh_step_size - z_offset) * scaleXZ);

                // ���������� ���������� (���������� ��� ����)
                glm::vec2 tc0(0.0f, 0.0f);
                glm::vec2 tc1(1.0f, 0.0f);
                glm::vec2 tc2(1.0f, 1.0f);
                glm::vec2 tc3(0.0f, 1.0f);

                // ��������� ������� ��� �������������
                glm::vec3 n1 = glm::normalize(glm::cross(p1 - p0, p2 - p0)); // ��� ������� ������������
                glm::vec3 n2 = glm::normalize(glm::cross(p2 - p0, p3 - p0)); // ��� ������� ������������
                glm::vec3 navg = glm::normalize(n1 + n2);                    // ������� ��� ����� ������

                // ��������� �������
                vertices.emplace_back(Vertex{ p0, navg, tc0 });
                vertices.emplace_back(Vertex{ p1, n1,   tc1 });
                vertices.emplace_back(Vertex{ p2, navg, tc2 });
                vertices.emplace_back(Vertex{ p3, n2,   tc3 });

                // ��������� ������� (CCW)
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

    // x, z: ���������� � ������� ������������
    float getWorldHeightAt(float x, float z, const cv::Mat& hmap, float heightScale) {
        // ����������� ������� x,z � ���������� �������� ����� �����
        int mesh_step_size = 30; // ��� �� ������, ��� ������������ ��� ��������� ����
        float x_offset = (hmap.cols - mesh_step_size) / 2.0f;
        float z_offset = (hmap.rows - mesh_step_size) / 2.0f;

        // ����������� � ���������� �����������
        int img_x = static_cast<int>(x * mesh_step_size + x_offset);
        int img_z = static_cast<int>(z * mesh_step_size + z_offset);

        // ������������ ���������� ����������
        img_x = std::max(0, std::min(img_x, hmap.cols - 1));
        img_z = std::max(0, std::min(img_z, hmap.rows - 1));

        double minVal, maxVal;
        cv::minMaxLoc(hmap, &minVal, &maxVal);
        double denom = (maxVal - minVal > 1e-5) ? (maxVal - minVal) : 1.0;
        float h = (hmap.at<uchar>(cv::Point(img_x, img_z)) - minVal) / denom;

        // ��������� �� �� ��������� ������
        h = (h - 0.5f) * 2.5f + 0.5f; // ����������� ��������
        h = std::max(0.0f, std::min(1.0f, h)); // ������������ [0,1]
        h = std::pow(h, 0.7f); // ��������� �������

        // ����������� � ���������� �������
        int level = static_cast<int>(h * HEIGHT_LEVELS);
        level = std::max(0, std::min(level, HEIGHT_LEVELS - 1));

        // ���������� ���������� ������
        float discrete_h = (level / float(HEIGHT_LEVELS - 1)) * 2.0f - 1.0f;
        return discrete_h * heightScale;
    }
};

#endif // HEIGHTMAP_H
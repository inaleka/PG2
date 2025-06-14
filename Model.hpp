#pragma once

#include <filesystem>
#include <string>
#include <vector> 
#include <glm/glm.hpp>
#include <opencv2/opencv.hpp>

#include "assets.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "OBJloader.hpp"
#include "HeightMap.h"


class Model {
public:
    std::vector<Mesh> meshes;
    std::string name;
    glm::vec3 origin{};
    glm::vec3 orientation{};
    glm::vec3 scale{ 1.0f };
    ShaderProgram shader;
    bool transparent{ false };
    glm::vec3 AABBMin{ FLT_MAX };
    glm::vec3 AABBMax{ -FLT_MAX };
    glm::vec3 AABBTransformedMin{ 0.0f };
    glm::vec3 AABBTransformedMax{ 0.0f };
    bool transformed{ false };

    glm::mat4 modelMatrix{ 1.0f };  // model matrix for transformations

    // constructor: load model from file
    Model(const std::filesystem::path& filename, ShaderProgram shader) : shader(shader) {
        loadModel(filename);
    }
    Model(ShaderProgram shader) : shader(shader) {};

    void setPos(const glm::vec3& pos) {
        origin = pos;
        transformed = true;
    }

    void setOrientation(const glm::vec3& orientation) {
        this->orientation = orientation;
        transformed = true;
    }

    void setScale(const glm::vec3& scale) {
        this->scale = scale;
        transformed = true;
    }

    void updateAABBAndModelMatrix() {
        if (!transformed) return;
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, origin);
        modelMatrix = glm::rotate(modelMatrix, orientation.x, glm::vec3(1, 0, 0));
        modelMatrix = glm::rotate(modelMatrix, orientation.y, glm::vec3(0, 1, 0));
        modelMatrix = glm::rotate(modelMatrix, orientation.z, glm::vec3(0, 0, 1));
        modelMatrix = glm::scale(modelMatrix, scale);

        glm::vec3 corners[8] = {
            AABBMin,
            glm::vec3(AABBMin.x, AABBMin.y, AABBMax.z),
            glm::vec3(AABBMin.x, AABBMax.y, AABBMin.z),
            glm::vec3(AABBMin.x, AABBMax.y, AABBMax.z),
            glm::vec3(AABBMax.x, AABBMin.y, AABBMin.z),
            glm::vec3(AABBMax.x, AABBMin.y, AABBMax.z),
            glm::vec3(AABBMax.x, AABBMax.y, AABBMin.z),
            AABBMax
        };

        glm::vec3 newMin(FLT_MAX), newMax(-FLT_MAX);
        for (auto& corner : corners) {
            glm::vec3 transformed = glm::vec3(modelMatrix * glm::vec4(corner, 1.0));
            newMin = glm::min(newMin, transformed);
            newMax = glm::max(newMax, transformed);
        }
        AABBTransformedMax = newMax;
        AABBTransformedMin = newMin;

        transformed = false;
    }

    glm::vec3 getAABBMin() {
        updateAABBAndModelMatrix();
        return AABBTransformedMin;
    }

    glm::vec3 getAABBMax() {
        updateAABBAndModelMatrix();
        return AABBTransformedMax;
    }

    float getHeight() {
        return getAABBMax().y - getAABBMin().y;
    }

    void draw(const glm::mat4& projection, const glm::mat4& view, const Lights& lights,
        const glm::vec3& viewPos) {
        updateAABBAndModelMatrix();

        // checks the sizes of spot and point lights
        // sorts them out and takes the max amount if the size exceeds the limit
        std::vector<PointLight> closestPointLights;
        std::vector<SpotLight> closestSpotLights;
        if (lights.pointLights.size() > MAX_POINT_LIGHTS) {
            closestPointLights = selectPointLights(lights.pointLights);
        }
        else {
            closestPointLights = lights.pointLights;
        }
        if (lights.spotLights.size() >= MAX_SPOT_LIGHTS)
            closestSpotLights = selectSpotLights(lights.spotLights);
        else {
            closestSpotLights = lights.spotLights;
        }
        for (auto& mesh : meshes) {
            mesh.draw(projection, view, modelMatrix, viewPos,
                lights.ambientLight, lights.sun, closestSpotLights, closestPointLights);
        }
    }

private:
#include <tuple>
    static constexpr uint MAX_POINT_LIGHTS = 15;
    static constexpr uint MAX_SPOT_LIGHTS = 15;


    void loadModel(const std::filesystem::path& path) {
        // load mesh (all meshes) of the model, (in the future: load material of each mesh, load textures...)
        // call LoadOBJFile, LoadMTLFile (if exist), process data, create mesh and set its properties
        //    notice: you can load multiple meshes and place them to proper positions, 
        //            multiple textures (with reusing) etc. to construct single complicated Model  

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;

        if (!loadOBJ(path.string().c_str(), positions, uvs, normals)) {
            std::cerr << "Failed to load model: " << path << std::endl;
            return;
        }
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;

        // build vertex data and indices
        for (size_t i = 0; i < positions.size(); ++i) {
            Vertex v;
            v.position = positions[i];
            if (i < uvs.size()) v.texcoord = uvs[i];
            if (i < normals.size()) v.normal = normals[i];
            vertices.push_back(v);
            indices.push_back(static_cast<GLuint>(i));
            AABBMax = glm::max(AABBMax, positions[i]);
            AABBMin = glm::min(AABBMin, positions[i]);

        }
        AABBTransformedMax = AABBMax;
        AABBTransformedMin = AABBMin;
        // create Mesh and store it
        meshes.emplace_back(GL_TRIANGLES, shader, vertices, indices, origin, orientation);

        // set model name based on the filename stem
        name = path.stem().string();

        std::cout << "Loaded model: " << path << "\n"
            << "Origin: (" << origin.x << ", " << origin.y
            << ", " << origin.z << ")\n"
            << "Vertices: " << vertices.size() << "\n"
            << "Indices: " << indices.size() << "\n"
            << "Meshes: " << meshes.size() << std::endl;
    }

    std::vector<PointLight> selectPointLights(const std::vector<PointLight>& lights) {
        // selects n=MAX_POINT_LIGHTS closest to the position of our Model object
        std::vector<PointLight> sorted = lights;
        glm::vec3 objectPos = glm::vec3(modelMatrix[3]);
        std::sort(sorted.begin(), sorted.end(),
            [objectPos](const PointLight& a, PointLight const& b) {
                float distA = glm::distance(a.position, objectPos);
                float distB = glm::distance(b.position, objectPos);
                return distA < distB;
            });
        sorted.resize(MAX_POINT_LIGHTS);
        return sorted;
    }

    std::vector<SpotLight> selectSpotLights(const std::vector<SpotLight>& lights) {
        // selects n=MAX_SPOT_LIGHTS closest to the position of our Model object
        std::vector<SpotLight> sorted = lights;
        glm::vec3 objectPos = glm::vec3(modelMatrix[3]);
        std::sort(sorted.begin(), sorted.end(),
            [objectPos](const SpotLight& a, SpotLight const& b) {
                float distA = glm::distance(a.position, objectPos);
                float distB = glm::distance(b.position, objectPos);
                return distA < distB;
            });
        sorted.resize(MAX_SPOT_LIGHTS);
        return sorted;
    }
};

class Terrain : public Model {
public:
    Terrain(ShaderProgram shader) : Model(shader) {
        loadTerrainModel();
        origin = glm::vec3(0.0f, 0.0f, 0.0f);
    };

    void getHeightOnMap(glm::vec3& pos, float modelHeight = 0) {
        float denom = (maxMapVal - minMapVal > 1e-5) ? (maxMapVal - minMapVal) : 1.0;

        // Offsets to recenter terrain
        float x_offset = (hmap.cols - mesh_step_size) / 2.0f;
        float z_offset = (hmap.rows - mesh_step_size) / 2.0f;

        // Convert world x, z into image coordinates
        int col = static_cast<int>((pos.x / mapScaleXZ) + x_offset);
        int row = static_cast<int>((pos.z / mapScaleXZ) + z_offset);

        if (col < 0 || row < 0 || col >= hmap.cols || row >= hmap.rows) {
            pos.y = 0.0f;  // Outside bounds
            return;
        }
        // Get pixel value and compute height
        float raw = static_cast<float>(hmap.at<uchar>(cv::Point(col, row)));
        float normalized = (raw - static_cast<float>(minMapVal)) / denom;
        float centered = (normalized - 0.5f) * 2.0f;
        pos.y = centered * height_scale + modelHeight;
    }

private:
    int mesh_step_size = 30; // Controls mesh triangle density/detail
    float height_scale = 0.5f; // Controls height exaggeration
    cv::Mat hmap;
    double minMapVal, maxMapVal;
    float mapScaleXZ = 1 / 20.0f;

    void loadTerrainModel() {
        hmap = cv::imread("resources/textures/heights.png", cv::IMREAD_GRAYSCALE);
        if (hmap.empty()) {
            throw std::runtime_error("No heightmap in file: resources/textures/heights.png");
        }
        HeightMap map{};
        auto terrainMeshes = map.GenHeightMap(hmap, mesh_step_size,
            height_scale, minMapVal, maxMapVal, mapScaleXZ, shader);
        for (auto& mesh : terrainMeshes) {
            meshes.push_back(mesh);
        }
        name = "Terrain";
        std::cout << "Loaded heightmap: resources/textures/heights.png" << std::endl;
    }
};
#pragma once

#include <string>
#include <vector>
#include <iostream>

#include <glm/glm.hpp> 
#include <glm/ext.hpp>

#include "assets.hpp"
#include "ShaderProgram.hpp"
#include "Lights.hpp"

class Mesh {
public:
    // mesh data
    glm::vec3 origin{};
    glm::vec3 orientation{};

    GLuint texture_id{ 0 }; // texture id=0  means no texture
    GLenum primitive_type = GL_POINT;
    ShaderProgram shader;

    // mesh material
    glm::vec4 ambient_material{ 1.0f }; //white, non-transparent 
    glm::vec4 diffuse_material{ 1.0f }; //white, non-transparent 
    glm::vec4 specular_material{ 1.0f }; //white, non-transparent
    float reflectivity{ 1.0f };

    // vertex data
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    // indirect (indexed) draw 
    Mesh(GLenum primitive_type, ShaderProgram shader, std::vector<Vertex> const& vertices, std::vector<GLuint> const& indices,
     glm::vec3 const& origin, glm::vec3 const& orientation, GLuint const texture_id = 0)
    : primitive_type(primitive_type), shader(shader), vertices(vertices), indices(indices),
      origin(origin), orientation(orientation), texture_id(texture_id)
    {
        // Create buffers and VAO using DSA
        glCreateVertexArrays(1, &VAO);
        glCreateBuffers(1, &VBO);
        glCreateBuffers(1, &EBO);

        // Upload data directly to VBO and EBO (no binding)
        glNamedBufferData(VBO, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        glNamedBufferData(EBO, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

        // Attach buffers to VAO
        glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(Vertex));
        glVertexArrayElementBuffer(VAO, EBO);

        // Vertex attributes
        // layout(location = 0) => position
        glEnableVertexArrayAttrib(VAO, 0);
        glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
        glVertexArrayAttribBinding(VAO, 0, 0);

        // layout(location = 1) => texcoord
        glEnableVertexArrayAttrib(VAO, 1);
        glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texcoord));
        glVertexArrayAttribBinding(VAO, 1, 0);

        // layout(location = 2) => normal
        glEnableVertexArrayAttrib(VAO, 2);
        glVertexArrayAttribFormat(VAO, 2, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
        glVertexArrayAttribBinding(VAO, 2, 0);

        tex0Loc = glGetUniformLocation(shader.getID(), "tex0");
        numSpotLoc = glGetUniformLocation(shader.getID(), "numSpotLights");
        numPointLoc = glGetUniformLocation(shader.getID(), "numPointLights");
    }



    // Helper to apply all lights to the shader
    void applyLights(const AmbientLight& ambientLight,
        const DirectionalLight& dirLight,
        const std::vector<SpotLight>& spotLights,
        const std::vector<PointLight>& pointLights)
    {
        GLuint shaderID = shader.getID();
        // Ambient Light
        ambientLight.apply(shaderID, 0);

        // Directional light
        dirLight.apply(shaderID, 0);

        // Spotlights
        int spotIndex = 0;
        for (const auto& spot : spotLights)
            spot.apply(shaderID, spotIndex++);

        glProgramUniform1i(shaderID, numSpotLoc, spotIndex);


        // Point lights
        int pointIndex = 0;
        for (const auto& point : pointLights)
            point.apply(shaderID, pointIndex++);

        glProgramUniform1i(shaderID, numPointLoc, pointIndex);
    }


    void draw(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model, const glm::vec3 viewPos,
        const AmbientLight& ambientLight, const DirectionalLight& dirLight, 
        const std::vector<SpotLight>& spotLights, const std::vector<PointLight>& pointLights) {
        shader.activate();

        // Set texture if available
        if (texture_id != 0) {
            glBindTextureUnit(0, texture_id);
            glProgramUniform1i(shader.getID(), tex0Loc, 0);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // set transformation matrices
        glm::mat4 mvp = projection * view * model;
        GLint uProj = glGetUniformLocation(shader.getID(), "uP_m");
        GLint uView = glGetUniformLocation(shader.getID(), "uV_m");
        GLint uModel = glGetUniformLocation(shader.getID(), "uM_m");

        if (uProj != -1) glUniformMatrix4fv(uProj, 1, GL_FALSE, &projection[0][0]);
        if (uView != -1) glUniformMatrix4fv(uView, 1, GL_FALSE, &view[0][0]);
        if (uModel != -1) glUniformMatrix4fv(uModel, 1, GL_FALSE, &model[0][0]);

        GLint uViewPos = glGetUniformLocation(shader.getID(), "viewPos");
        if (uViewPos != -1) glUniform3fv(uViewPos, 1, &viewPos[0]);


        // ****** APPLY LIGHTS HERE ******
        applyLights(ambientLight, dirLight, spotLights, pointLights);

        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(primitive_type, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }




    void clear(void) {
		// clear texture
        if (texture_id != 0) {
            glDeleteTextures(1, &texture_id);
            texture_id = 0;
        }

        primitive_type = GL_POINT;
        // clear rest of the member variables to safe default
        vertices.clear();
        indices.clear();
        origin = glm::vec3(0.0f);
        orientation = glm::vec3(0.0f);

        // delete all allocations 
        if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
        if (EBO) { glDeleteBuffers(1, &EBO); EBO = 0; }
        if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    };

private:
    // OpenGL buffer IDs
    // ID = 0 is reserved (i.e. uninitalized)
    GLuint tex0Loc;
    GLuint numSpotLoc;
    GLuint numPointLoc;
    unsigned int VAO{ 0 }, VBO{ 0 }, EBO{ 0 };
};



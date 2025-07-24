#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <vector>
#include "ShaderProgram.hpp"

namespace Particles {
    struct Particle {
        glm::vec3 position, velocity;
        float life = 0.7f;
        bool active = false;
    };

	constexpr int MAX_PARTICLES = 1000;                // Maximum number of particles in the pool
	inline std::vector<Particle> pool(MAX_PARTICLES);  // Particle pool

	// Update all active particles
    inline void update(float dt) {
        for (auto& p : pool) {
            if (!p.active) continue;
            p.position += p.velocity * dt;
            p.life -= dt;
            if (p.life <= 0.0f) p.active = false;
        }
    }

	// Spawn new particles at a given origin
    inline void spawn(const glm::vec3& origin, int count = 10) {
        int spawned = 0;
        for (auto& p : pool) {
            if (!p.active) {
                p.position = origin;
                p.velocity = glm::sphericalRand(2.0f);
                p.life = 0.5f + float(rand()) / RAND_MAX;
                p.active = true;
                if (++spawned >= count) break;
            }
        }
    }

    // Draw all active particles as GL_POINTS
    // shader must have a vec4 uniform "color" and a mat4 uniform "uMVP"
    inline void drawParticles(const glm::mat4& projection, const glm::mat4& view, ShaderProgram& shader) {
        std::vector<glm::vec3> points;

        for (const auto& p : pool) {
            if (p.active) {
                points.push_back(p.position);
                points.push_back(p.position + p.velocity * 0.1f);
            }
        }

        if (points.empty()) return;

        // Create VAO and VBO using DSA
        GLuint VAO = 0, VBO = 0;
        glCreateVertexArrays(1, &VAO);
        glCreateBuffers(1, &VBO);

        glNamedBufferData(VBO, points.size() * sizeof(glm::vec3), points.data(), GL_STATIC_DRAW);

        // Attach VBO to VAO, layout location 0
        glVertexArrayVertexBuffer(VAO, 0, VBO, 0, sizeof(glm::vec3));
        glEnableVertexArrayAttrib(VAO, 0);
        glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(VAO, 0, 0); // bind attribute index 0 to binding index 0

        // Activate shader and set uniforms via DSA
        shader.activate();
        GLuint programID = shader.getID();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp = projection * view * model;

        GLint locMVP = glGetUniformLocation(programID, "uMVP");
        GLint locColor = glGetUniformLocation(programID, "color");

        glProgramUniformMatrix4fv(programID, locMVP, 1, GL_FALSE, &mvp[0][0]);
        glProgramUniform4f(programID, locColor, 1.0f, 0.5f, 0.0f, 1.0f); 

        glEnable(GL_PROGRAM_POINT_SIZE);
        glLineWidth(3.5f);
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(points.size()));
        glBindVertexArray(0);

        // Cleanup
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }


}
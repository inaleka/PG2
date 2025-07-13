#pragma once
#include "Entity.hpp"
#include <glm/glm.hpp>
#include <cmath>
#include <functional>

namespace Behaviors {
    using Behavior = Entity::Behavior;

    // Walk in a circle
    inline Behavior WalkInCircle(glm::vec3 center, float radius, float speed = 1.0f) {
        float angle = 0.0f;
        return [=](Entity& self, float dt) mutable {
            angle += speed * dt;
            glm::vec3 target = center + glm::vec3(cos(angle) * radius, 0, sin(angle) * radius);
            glm::vec3 dir = glm::normalize(target - self.position);
            self.applyForce(dir * self.movementSpeed);
            };
    }

    inline Behavior FlyUp() {
        return [=](Entity& self, float dt) mutable {
            if (self.position.y <= 8.0f) self.applyForce(glm::vec3(0.0f, 1.0f, 0.0f) * self.movementSpeed);
        };
    }

    inline Behavior FollowCamera() {
        return [=](Entity& self, float dt) mutable {
            if (self.camera != nullptr) {
                if (glm::length(glm::distance(self.camera->position, self.position)) >= 2.0f)
                    self.setSpeed(glm::normalize(self.camera->position - self.position) * 0.5f);
                else
                    self.setSpeed(glm::vec3(0));
            }
        };
    }

    // Bob up and down
    inline Behavior Bob(float amplitude = 0.5f, float speed = 1.0f) {
		std::cout << "Bob behavior initialized with amplitude: " << amplitude << " and speed: " << speed << std::endl;
        float baseY = 0.0f;
        bool first = true;
        return [=](Entity& self, float dt) mutable {
            if (first) { first = false; }
            self.updatePos(0, static_cast<float>(sin(glfwGetTime() * speed) * amplitude), 0);
            };
    }

}
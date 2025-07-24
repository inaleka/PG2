#pragma once
#include <glm/glm.hpp>
#include <functional>
#include <vector>
#include "Model.hpp"
#include "Camera.hpp"


class Entity {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 rotation = glm::vec3(0.0f);

    float yaw = -90.0f, pitch = 0.0f;
    float movementSpeed = 10.0f;
    float drag = 1.0f;
    float gravity = -9.81f;
    bool isGrounded = true;

    Camera* camera;
    Model* model; // optional visual

    using Behavior = std::function<void(Entity&, float)>;
    std::vector<Behavior> behaviors;

    Entity(glm::vec3 pos, Model* mdl = nullptr, Camera* camera = nullptr)
        : position(pos), velocity(0.0f), acceleration(0.0f), model(mdl), camera(camera){
    }

    Entity() : position(0.0f), model(nullptr), velocity(0.0f), acceleration(0.0f), camera(nullptr) {}


    void update(float dt, float groundHeight) {
        // Gravity
        if (!isGrounded) acceleration.y += gravity;

        for (auto& b : behaviors) b(*this, dt);

        // Physics
        velocity += acceleration * dt;
        velocity.x *= pow(drag, dt);
        velocity.z *= pow(drag, dt);

        position += velocity * dt;
        if (position.y <= groundHeight) {
            position.y = groundHeight;
            velocity.y = 0;
            isGrounded = true;
        }
        else {
            isGrounded = false;
        }
        acceleration = glm::vec3(0.0f);
        if (model) {
            model->setPos(position);
            model->setRotation(rotation); 
        }
    }

    void applyForce(const glm::vec3& force) { acceleration += force; }
    void reverseSpeedXZ() {
        velocity.x = -velocity.x;
        velocity.z = -velocity.z;
    }

    void setSpeed(glm::vec3 speed) { velocity = speed; }

    void setGravity(const float gravity) {
        this->gravity = gravity;
    }

    void updatePos(const float x = 0, const float y = 0, const float z = 0) {
        position += glm::vec3(x, y, z);
        if (model) model->setPos(position);
    }

    void jump(float strength) {
        if (isGrounded) { velocity.y = strength; isGrounded = false; }
    }
};

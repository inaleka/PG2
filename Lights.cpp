#include "Lights.hpp"
#include <glm/gtc/type_ptr.hpp>

// Factory methods
DirectionalLight DirectionalLight::createDefault() {
    return DirectionalLight(
        glm::vec3(-0.5f, -1.0f, -0.2f), // direction
        glm::vec3(0.2f),                 // ambient
        glm::vec3(0.5f),                 // diffuse
        glm::vec3(1.0f)                  // specular
    );
}

PointLight PointLight::createDefault(const glm::vec3& position, const glm::vec3& color) {
    return PointLight(
        position,                // position
        color * 0.1f,            // ambient
        color * 0.8f,            // diffuse
        glm::vec3(1.0f),         // specular
        1.0f,                    // constant  
        0.0f,                    // linear    
        0.0f                     // quadratic 
    );
}

SpotLight SpotLight::createDefault(const glm::vec3& pos, const glm::vec3& dir) {
    return SpotLight(
        pos,                                  // position
        dir,                                  // direction
        glm::cos(glm::radians(15.0f)),        // cutOff
        glm::cos(glm::radians(30.0f)),        // outerCutOff
        glm::vec3(0.2f, 0.0f, 0.0f),          // ambient
        glm::vec3(1.0f, 0.0f, 0.0f),          // diffuse
        glm::vec3(1.0f, 0.0f, 0.0f),          // specular
        1.0f,                                 // constant 
        0.0f,                                 // linear    
        0.0f                                  // quadratic 
    );
}

AmbientLight AmbientLight::createDefault(const glm::vec3& color) {
    return AmbientLight(color);
}

// Apply methods for each light type
void DirectionalLight::apply(ShaderProgram& shader, int index) const {
    std::string prefix = "dirLights[" + std::to_string(index) + "]";
    shader.setUniform(prefix + ".direction", direction);
    shader.setUniform(prefix + ".ambient", ambient);
    shader.setUniform(prefix + ".diffuse", diffuse);
    shader.setUniform(prefix + ".specular", specular);
}

void PointLight::apply(ShaderProgram& shader, int index) const {
    std::string prefix = "pointLights[" + std::to_string(index) + "]";
    shader.setUniform(prefix + ".position", position);
    shader.setUniform(prefix + ".ambient", ambient);
    shader.setUniform(prefix + ".diffuse", diffuse);
    shader.setUniform(prefix + ".specular", specular);
    shader.setUniform(prefix + ".constant", constant);
    shader.setUniform(prefix + ".linear", linear);
    shader.setUniform(prefix + ".quadratic", quadratic);
}

void SpotLight::apply(ShaderProgram& shader, int index) const {
    std::string prefix = "spotLights[" + std::to_string(index) + "]";
    shader.setUniform(prefix + ".position", position);
    shader.setUniform(prefix + ".direction", direction);
    shader.setUniform(prefix + ".cutOff", cutOff);
    shader.setUniform(prefix + ".outerCutOff", outerCutOff);
    shader.setUniform(prefix + ".ambient", ambient);
    shader.setUniform(prefix + ".diffuse", diffuse);
    shader.setUniform(prefix + ".specular", specular);
    shader.setUniform(prefix + ".constant", constant);
    shader.setUniform(prefix + ".linear", linear);
    shader.setUniform(prefix + ".quadratic", quadratic);
}

void AmbientLight::apply(ShaderProgram& shader, int /*index*/) const {
    shader.setUniform("ambientLight.color", color);
}


void Lights::initDirectionalLight() {
    sun = DirectionalLight::createDefault();
}

void Lights::initPointLight(const glm::vec3& position, const glm::vec3& color) {
    pointLights.emplace_back(PointLight::createDefault(position, color));
}

void Lights::initSpotLight(const glm::vec3& pos, const glm::vec3& dir) {
    spotLights.emplace_back(SpotLight::createDefault(pos, dir));
}

void Lights::initAmbientLight(const glm::vec3& color) {
    ambientLight = AmbientLight::createDefault(color);
};
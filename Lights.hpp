#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <GL/glew.h>
#include "ShaderProgram.hpp"

// Base class for all lights
struct LightSource {
    glm::vec3 ambient{ 0.1f };
    glm::vec3 diffuse{ 0.8f };
    glm::vec3 specular{ 1.0f };
    virtual void apply(ShaderProgram& shader, int index) const = 0;
    virtual std::string getType() const = 0;
    virtual ~LightSource() = default;
};

struct DirectionalLight : public LightSource {
    glm::vec3 direction{ -0.2f, -1.0f, -0.3f };
    DirectionalLight() = default;
    DirectionalLight(const glm::vec3& dir,
        const glm::vec3& amb,
        const glm::vec3& diff,
        const glm::vec3& spec)
        : direction(dir)
    {
        ambient = amb;
        diffuse = diff;
        specular = spec;
    }
    static DirectionalLight createDefault();
    void apply(ShaderProgram& shader, int index) const override;
    std::string getType() const override { return "directional"; }
};

struct PointLight : public LightSource {
    glm::vec3 position{ 0.0f };
    float constant{ 1.0f };
    float linear{ 0.09f };
    float quadratic{ 0.032f };
    PointLight() = default;
    PointLight(const glm::vec3& pos,
        const glm::vec3& amb,
        const glm::vec3& diff,
        const glm::vec3& spec,
        float c,
        float l,
        float q)
        : position(pos), constant(c), linear(l), quadratic(q)
    {
        ambient = amb;
        diffuse = diff;
        specular = spec;
    }
    static PointLight createDefault(const glm::vec3& position, const glm::vec3& color);
    void apply(ShaderProgram& shader, int index) const override;
    std::string getType() const override { return "point"; }
};

struct SpotLight : public LightSource {
    glm::vec3 position{ 0.0f };
    glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
    float cutOff{ glm::cos(glm::radians(12.5f)) };
    float outerCutOff{ glm::cos(glm::radians(17.5f)) };
    float constant{ 1.0f };
    float linear{ 0.09f };
    float quadratic{ 0.032f };
    SpotLight() = default;
    SpotLight(const glm::vec3& pos,
        const glm::vec3& dir,
        float cut, float outer,
        const glm::vec3& amb,
        const glm::vec3& diff,
        const glm::vec3& spec,
        float c,
        float l,
        float q)
        : position(pos), direction(dir),
        cutOff(cut), outerCutOff(outer),
        constant(c), linear(l), quadratic(q)
    {
        ambient = amb;
        diffuse = diff;
        specular = spec;
    }
    static SpotLight createDefault(const glm::vec3& pos, const glm::vec3& dir);
    void apply(ShaderProgram& shader, int index) const override;
    std::string getType() const override { return "spot"; }
};

struct AmbientLight {
    glm::vec3 color{ 0.1f, 0.1f, 0.1f };

    AmbientLight() = default;
    explicit AmbientLight(const glm::vec3& color) : color(color) {}

    static AmbientLight createDefault(const glm::vec3& color = glm::vec3(0.1f, 0.1f, 0.1f));
    void apply(ShaderProgram& shader, int index = 0) const;
    std::string getType() const { return "ambient"; }
    ~AmbientLight() = default;
};

struct Lights {
    AmbientLight ambientLight;
    DirectionalLight sun;
    std::vector<SpotLight> spotLights;
    std::vector<PointLight> pointLights;

    void initDirectionalLight();
    void initPointLight(const glm::vec3& position, const glm::vec3& color);
    void initSpotLight(const glm::vec3& pos, const glm::vec3& dir);
    void initAmbientLight(const glm::vec3& color = glm::vec3(0.1f, 0.1f, 0.1f));
};
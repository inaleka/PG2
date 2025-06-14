#pragma once  

// OpenCV (does not depend on GL)  
//#include <opencv2\opencv.hpp>  

// include anywhere, in any order  
#include <iostream>  
#include <fstream>
#include <chrono>  
#include <stack>  
#include <random>  
#include <vector>  
#include <unordered_map>  
#include <nlohmann/json.hpp>  
#include <opencv2/opencv.hpp>
#include <string>
#include <cstring>

// OpenGL Extension Wrangler: allow all multiplatform GL functions  
#include <GL/glew.h>  
// WGLEW = Windows GL Extension Wrangler (change for different platform)  
#ifdef _WIN32
    #include <GL/wglew.h>  // Windows-specific WGL extensions
#elif defined(__linux__)
    #include <GL/glxew.h>  // Linux-specific GLX extensions
#endif 
#include <GL/gl.h>  

// GLFW toolkit  
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.  
#include <GLFW/glfw3.h>  

// OpenGL math (and other additional GL libraries, at the end)  
#include <glm/glm.hpp>  
#include <glm/gtc/type_ptr.hpp>  

// User includes  
#include "assets.hpp"  
#include "ShaderProgram.hpp"  
#include "Model.hpp"  
#include "Mesh.hpp"
#include "camera.hpp"
#include "Lights.hpp"
#include "Entity.hpp"
#include "Behavior.hpp"
#include "Particles.hpp"

// callbacks
#include "gl_err_callback.h"

class App {
public:
    GLFWwindow* window;
    glm::mat4 projectionMatrix;  // projection matrix
    glm::mat4 viewMatrix;        // view matrix
    float fov;                   // field of view

    // lights struct
    Lights lights;

    App();
    bool init();
    int run();
    void shootProjectile();
    void initAssets();
    GLuint textureInit(const std::filesystem::path& file_name, bool& isTransparent);
    GLuint gen_tex(cv::Mat& image, bool& isTransparent);
    void initLights();
    void updateProjection();
    void toggleFullscreen();

    static void mouse_clicked_callback(GLFWwindow* window, int button, int action, int mods);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    ~App();

protected:
    // all objects of the scene addressable by name  
    std::unordered_map<std::string, Model> scene;
    Terrain *terrain;
    ShaderProgram shader;
    ShaderProgram particleShader;
    // entities
    std::unordered_map<std::string, Entity> entities;
    std::unordered_map<std::string, Entity> projectiles;

private:
    // default window settings
    int windowWidth;
    int windowHeight;
    GLFWmonitor* savedMonitor = nullptr;
    int savedX = 0, savedY = 0, savedWidth = 0, savedHeight = 0;
    bool isFullscreen = false;

    bool AA;
    int AASamples;
    std::string windowTitle{ "OpenGL Scene" };
    bool vsync;                  // V-Sync state
    glm::vec4 currentColor;      // RGBA format  

    // sun settings
    GLuint sunVAO = 0, sunVBO = 0;
    ShaderProgram sunShader;
    GLuint sunTexture = 0;

    Camera camera{ glm::vec3(0.0f, 0.0f, 3.0f) }; // camera with initial position
    double cursorLastX = 0.0;                     // last X mouse position
    double cursorLastY = 0.0;                     // last Y mouse position
    bool firstMouse = true;                       // first mouse movement flag

    void loadConfig();
    void printGLInfo();
};
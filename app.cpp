#include "app.hpp"
#include "Particles.hpp"


bool AABBintersect(const glm::vec3& minA, const glm::vec3& maxA,
    const glm::vec3& minB, const glm::vec3& maxB) {
    return (minA.x <= maxB.x && maxA.x >= minB.x) &&
        (minA.y <= maxB.y && maxA.y >= minB.y) &&
        (minA.z <= maxB.z && maxA.z >= minB.z);
}


App::App() : window(nullptr), fov(60.0f), vsync(true), currentColor(1.0f, 0.0f, 0.0f, 1.0f) {
    std::cout << "Application initialized\n";
}

void App::loadConfig() {
    // load window configuration from JSON file
    try {
        std::ifstream configFile("app_settings.json");
        if (!configFile.is_open()) {
            throw std::runtime_error("Failed to open config file");
        }

        // parse JSON
        nlohmann::json config = nlohmann::json::parse(configFile);

        windowWidth = config["default_resolution"].value("x", 800);
        windowHeight = config["default_resolution"].value("y", 600);
        windowTitle = config.value("appname", "OpenGL Scene");
        fov = config.value("fov", 60.0f);
        AA = config["AA"].value("enabled", false);
        AASamples = config["AA"].value("samples", 0);
        // close file
        configFile.close();

        std::cout << "Window configuration loaded successfully:\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading window configurations: " << e.what()
            << " using default settings" << std::endl;
        windowWidth = 800;
        windowHeight = 600;
        fov = 60.0f;
    }
}

void App::printGLInfo() {
    // basic OpenGL information
    std::cout << "\nOpenGL Context Information:" << std::endl;
    std::cout << "===========================" << std::endl;

    // vendor and renderer information
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    std::cout << "Vendor: \t" << (vendor ? vendor : "<Unknown>") << '\n';

    const char* renderer = (const char*)glGetString(GL_RENDERER);
    std::cout << "Renderer: \t" << (renderer ? renderer : "<Unknown>") << '\n';

    // version information
    const char* gl_version = (const char*)glGetString(GL_VERSION);
    std::cout << "OpenGL Version: \t" << (gl_version ? gl_version : "<Unknown>") << '\n';

    const char* glsl_version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    std::cout << "GLSL Version: \t\t" << (glsl_version ? glsl_version : "<Unknown>") << '\n';

    // numeric version verification
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    std::cout << "OpenGL Context Version: \t" << major << "." << minor << '\n';

    if (major < 4 || (major == 4 && minor < 6)) {
        throw std::runtime_error("OpenGL 4.6 context not created!");
    }

    // profile information
    GLint profile_mask;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_mask);
    std::cout << "Context Profile: \t";

    if (profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) {
        std::cout << "Core Profile";
    }
    else if (profile_mask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) {
        std::cout << "Compatibility Profile";
    }
    else {
        std::cout << "<Unknown Profile>";
    }
    std::cout << '\n';

    // context flags
    GLint context_flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    std::cout << "Context Flags: \t\t";

    if (context_flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
        std::cout << "[Forward Compatible] ";
    if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        std::cout << "[Debug] ";
    if (context_flags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT)
        std::cout << "[Robust Access] ";
    if (context_flags & GL_CONTEXT_FLAG_NO_ERROR_BIT)
        std::cout << "[No Error] ";

    std::cout << "\n===========================\n\n";
}

bool App::init() {
	// initialize GLFW window hints
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glDepthMask(GL_FALSE);                      // не писать глубину
    glDisable(GL_CULL_FACE);                    // отключить отсечение граней


    

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    // assume ALL objects are non-transparent
    // glEnable(GL_CULL_FACE);

    // request debug context
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    // load window configuration
    loadConfig();

    // init GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    // request MSAA
    if (AA) glfwWindowHint(GLFW_SAMPLES, AASamples);

    // Open GL Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // create window
    window = glfwCreateWindow(windowWidth, windowHeight, windowTitle.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);

    // init GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLEW");
    }
    // enable antialiasing
    if (AA) glEnable(GL_MULTISAMPLE);

    if (glewIsSupported("GL_ARB_direct_state_access")) {
        std::cout << "DSA is supported via ARB extension!" << std::endl;
    }

    // initial view matrix
    updateProjection();
    camera.position = glm::vec3(0.0f, 4.0f, 3.0f);
    viewMatrix = camera.GetViewMatrix();

    // print OpenGL information
    std::cout << "\nInitializing OpenGL context...\n";
    printGLInfo();

    // print OpenGL errors
    if (GLEW_ARB_debug_output) {
        glDebugMessageCallback(MessageCallback, 0);
        glEnable(GL_DEBUG_OUTPUT);
        std::cout << "GL_DEBUG enabled.\n" << std::endl;
    }
    else {
        std::cout << "GL_DEBUG NOT SUPPORTED!\n" << std::endl;
    }

    // activate Vsync
    glfwSwapInterval(vsync ? 1 : 0);

    // activate callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetMouseButtonCallback(window, mouse_clicked_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback); // mouse movement
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // init resources
    try {
        initAssets();
        std::cout << "Assets initialized successfully\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Asset initialization failed: " << e.what() << std::endl;
        return false;
    }
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthFunc(GL_LEQUAL);
    return true;
}

void App::initAssets(void) {
    /*
     * Terrain init
     */
    bool isTransparent = false;
    shader = ShaderProgram("resources/shaders/tex.vert", "resources/shaders/tex.frag");
    terrain = new Terrain{ shader };
    GLuint texture_terrain = textureInit("resources/textures/moon.png", isTransparent);

    terrain->transparent = isTransparent;
    for (auto& mesh : terrain->meshes) {
        mesh.texture_id = texture_terrain;
    }
    //terrain->getHeightOnMap(camera.position, 0.2f);
    
    //Model skybox("resources/objects/cube.obj", shader);  // загружает mesh из .obj
    //
    //skybox.setScale(glm::vec3(100.0f));
    //skybox.setPos(glm::vec3(0.0f));

    //GLuint texID = textureInit("resources/textures/space.jpg", isTransparent);

    //if (!skybox.meshes.empty()) {
    //    skybox.meshes[0].texture_id = texID;
    //}
    //else {
    //    std::cerr << "Skybox error: no meshes loaded!" << std::endl;
    //}

    //scene.emplace("skybox", skybox);

    /*
     * Triangle init
     */
    glm::vec3 initPos = glm::vec3(2.0f, 0.0f, 0.0f);
    isTransparent = true;

    // load model
    Model triangleModel1("resources/objects/triangle.obj", shader);
    terrain->getHeightOnMap(initPos, triangleModel1.getHeight() / 2.0f);
    triangleModel1.setPos(initPos);  // center the model

    // load texture
    GLuint texture = textureInit("resources/textures/tex_256.png", isTransparent);
    triangleModel1.transparent = isTransparent;
    // assign all textures to all meshes
    for (auto& mesh : triangleModel1.meshes) {
        mesh.texture_id = texture;
    }

    // add to scene
    scene.emplace("triangle1", std::move(triangleModel1));

    initPos = glm::vec3(-2.0f, 0.0f, 0.0f);
    Model torchModel("resources/objects/torch.obj", shader);
    terrain->getHeightOnMap(initPos, torchModel.getHeight() / 2.0f);
    torchModel.setPos(initPos);  // center the model

    torchModel.transparent = isTransparent;
    // assign all textures to all meshes
    for (auto& mesh : torchModel.meshes) {
        mesh.texture_id = texture;
    }

    // add to scene
    scene.emplace("torch", std::move(torchModel));

    /*
     * Entities and particles init
     */
     // Load the bot model from an OBJ file
    Model botModel("resources/objects/cube.obj", shader);
    initPos = glm::vec3{ 0.0f, 0.0f, 0.0f };
    terrain->getHeightOnMap(initPos, botModel.getHeight() / 2.0f);
    botModel.transparent = isTransparent;
    for (auto& mesh : botModel.meshes) {
        mesh.texture_id = texture;
    }
    std::string botName = "bot";
    scene.emplace(botName, std::move(botModel));
    auto botModelPtr = &scene.at("bot"); // store pointer for entity

    auto cameraPtr = &camera;
    Entity bot(initPos, botModelPtr, cameraPtr);
    bot.behaviors.push_back(Behaviors::FollowCamera());
    bot.setSpeed(glm::vec3(0.3f, 0.0f, 0.0f));
    entities.emplace(botName, std::move(bot));

    Model botModel1("resources/objects/cube_lava.obj", shader);
    initPos = glm::vec3{ 0.0f, 0.0f, -3.0f };
    terrain->getHeightOnMap(initPos, botModel1.getHeight() / 2.0f);
    botModel1.transparent = isTransparent;
    for (auto& mesh : botModel1.meshes) {
        mesh.texture_id = texture;
    }
    std::string botName1 = "bot1";
    scene.emplace(botName1, std::move(botModel1));
    auto botModelPtr1 = &scene.at("bot1"); // store pointer for entity

    Entity bot1(initPos, botModelPtr1);
    bot1.behaviors.push_back(Behaviors::FlyUp());
    bot1.setSpeed(glm::vec3(0.0f, 0.0f, 0.0f));
    entities.emplace(botName1, std::move(bot1));



    // init particles shader
    particleShader = ShaderProgram("resources/shaders/particle.vert", "resources/shaders/particle.frag");

    // initialize lights
    initLights();

}

void App::updateProjection() {
    float aspect = static_cast<float>(windowWidth) / windowHeight;
    projectionMatrix = glm::perspective(
        glm::radians(fov), aspect, 0.1f, 100.0f
    );
}

GLuint App::textureInit(const std::filesystem::path& file_name, bool& isTransparent)
{
    cv::Mat image = cv::imread(file_name.string(), cv::IMREAD_UNCHANGED);  // Read with (potential) Alpha
    if (image.empty()) {
        throw std::runtime_error("No texture in file: " + file_name.string());
    }

    // or print warning, and generate synthetic image with checkerboard pattern 
    // using OpenCV and use as a texture replacement

    GLuint texture = gen_tex(image, isTransparent);

    return texture;
}

GLuint App::gen_tex(cv::Mat& image, bool& isTransparent)
{
    GLuint ID = 0;
    if (image.empty())
        throw std::runtime_error("Image empty?\n");


    // Generates an OpenGL texture object
    glCreateTextures(GL_TEXTURE_2D, 1, &ID);

    switch (image.channels()) {
    case 3:
        // Create and clear space for data - immutable format
        glTextureStorage2D(ID, 1, GL_RGB8, image.cols, image.rows);
        // Assigns the image to the OpenGL Texture object
        glTextureSubImage2D(ID, 0, 0, 0, image.cols, image.rows, GL_BGR, GL_UNSIGNED_BYTE, image.data);
        break;
    case 4:
        for (int y = 0; y < image.rows && !isTransparent; ++y) {
            for (int x = 0; x < image.cols; ++x) {
                cv::Vec4b pixel = image.at<cv::Vec4b>(y, x);
                if (pixel[3] < 255) { // pixel[3] is alpha
                    isTransparent = true;
                    break;
                }
            }
        }
        glTextureStorage2D(ID, 1, GL_RGBA8, image.cols, image.rows);
        glTextureSubImage2D(ID, 0, 0, 0, image.cols, image.rows, GL_BGRA, GL_UNSIGNED_BYTE, image.data);
        break;
    default:
        throw std::runtime_error("unsupported channel cnt. in texture:" + std::to_string(image.channels()));
    }

    // MIPMAP filtering + automatic MIPMAP generation - nicest, needs more memory. Notice: MIPMAP is only for image minifying.
    glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // bilinear magnifying
    glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // trilinear minifying
    glGenerateTextureMipmap(ID);  //Generate mipmaps now.

    // Configures the way the texture repeats
    glTextureParameteri(ID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(ID, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return ID;
}

void App::initLights() {
    // init point lights from the file
    std::filesystem::path point_lights_path = "resources/lights/point_lights.lights";
    std::ifstream file_point_light(point_lights_path);

    if (!file_point_light.is_open()) {
        std::cout << "Could not open point light file: " << point_lights_path << std::endl;
    }

    std::string line;
    while (std::getline(file_point_light, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream ss(line);
        float x, y, z, r, g, b;

        if (!(ss >> x >> y >> z >> r >> g >> b)) {
            std::cerr << "Invalid point light entry: " << line << std::endl;
            continue; // or throw
        }
        lights.initPointLight(glm::vec3(x, y, z), glm::vec3(r, g, b));
    }
    file_point_light.close();

    // init spot lights from the file
    std::filesystem::path spot_lights_path = "resources/lights/spot_lights.lights";
    std::ifstream file_spot_light(spot_lights_path);

    if (!file_spot_light.is_open()) {
        std::cout << "Could not open spot light file: " << point_lights_path << std::endl;
    }

    while (std::getline(file_spot_light, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream ss(line);
        float posX, posY, posZ, dirX, dirY, dirZ;

        if (!(ss >> posX >> posY >> posZ >> dirX >> dirY >> dirZ)) {
            std::cerr << "Invalid spot light entry: " << line << std::endl;
        }
        lights.initSpotLight(glm::vec3(posX, posY, posZ),
            glm::vec3(dirX, dirY, dirZ));
    }
    file_spot_light.close();

    // directional light
    lights.initDirectionalLight();

    // ambient light
    lights.initAmbientLight(glm::vec3(0.0f));


}

void App::applyLights()
{
    // Ambient Light
    lights.ambientLight.apply(shader, 0);

    // Directional light
    lights.sun.apply(shader, 0);

    // Spotlights
    int spotIndex = 0;
    for (const auto& spot : lights.spotLights)
        spot.apply(shader, spotIndex++);

    // glProgramUniform1i(shaderID, numSpotLoc, spotIndex);
    shader.setUniform("numSpotLights", spotIndex);


    // Point lights
    int pointIndex = 0;
    for (const auto& point : lights.pointLights)
        point.apply(shader, pointIndex++);

    // glProgramUniform1i(shaderID, numPointLoc, pointIndex);
    shader.setUniform("numPointLights", pointIndex);
}



void App::shootProjectile() {
    bool isTransparent = false;
    glm::vec3 start = camera.position;
    glm::vec3 direction = glm::normalize(camera.front);
    glm::vec3 spawnPos = (start + direction);
    GLuint texture = textureInit("resources/textures/tex_256.png", isTransparent);

    Model projectileModel("resources/objects/cube_bullet.obj", shader);
    projectileModel.transparent = isTransparent;
    for (auto& mesh : projectileModel.meshes) {
        mesh.texture_id = texture;
    }
    projectileModel.setScale(glm::vec3(0.1f));
    projectileModel.setPos(spawnPos);
    std::ostringstream oss;
    oss << "projectile_" << glfwGetTime();
    scene.emplace(oss.str(), std::move(projectileModel));
    auto projectileModelPtr = &scene.at(oss.str()); // store pointer for entity


    Entity projectileEntity(spawnPos, projectileModelPtr);

    projectileEntity.setGravity(0);
    projectileEntity.setSpeed(direction * 0.5f);
    projectiles.emplace(oss.str(), std::move(projectileEntity));


   /* Entity projectile(spawnPos);
    projectile.setGravity(0);
    projectile.setSpeed(direction);
    projectile.model = new Model("resources/objects/cube_lava.obj", shader);

    

    Particles::spawn(spawnPos, 10);

    projectiles[oss.str()] = projectile;*/
}

int App::run() {
    // Enable back-face culling to improve performance by not rendering polygons facing away from the camera
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    // Initialize camera settings
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // capture mouse
    glfwGetCursorPos(window, &cursorLastX, &cursorLastY);        // get initial position
    glViewport(0, 0, windowWidth, windowHeight);

    // time variables
    double lastTime = glfwGetTime();
    double lastFrameTime = glfwGetTime();
    double deltaTime = 0.0;
    int frameCount = 0;

    // Print number of lights before drawing
    std::cout << "numPointLights = " << lights.pointLights.size()
        << ", numSpotLights = " << lights.spotLights.size() << std::endl;

    // index of last spotlight
    size_t movingSpotIndex = lights.spotLights.size() - 1;

    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        double currentFrameTime = glfwGetTime();
        deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        // Process camera movement
        glm::vec3 moveOffset = camera.ProcessInput(window, deltaTime);
        camera.position += moveOffset;

        // Update view matrix from camera
        viewMatrix = camera.GetViewMatrix();

        // Clear buffers
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- ENTITY & PARTICLE LOGIC ---
        float groundHeight = 0.0f; // You could sample from terrain here if desired
        for (auto& [name, ent] : entities) {
            glm::vec3 entityPosition = ent.position;
            terrain->getHeightOnMap(entityPosition, ent.model->getHeight() / 2.0f);
            ent.update(static_cast<float>(deltaTime), entityPosition.y);
            //std::cout << "Bot position: " << ent.position.x << ", " << ent.position.y << ", " << ent.position.z << std::endl;

            // Example: spawn sparks at bot position every time it passes a certain y threshold
            if (ent.position.y > 5.5f) {
                Particles::spawn(ent.position, 10);
            }

        }
        Particles::update(static_cast<float>(deltaTime));

        /*
         *  --- COLLISIONS ---
         */

        for (auto it1 = scene.begin(); it1 != scene.end(); ++it1) {
            for (auto it2 = std::next(it1); it2 != scene.end(); ++it2) {
                if (it1->second.name == "Terrain" || it2->second.name == "Terrain") { continue; }
                auto minA = it1->second.getAABBMin();
                auto maxA = it1->second.getAABBMax();
                auto minB = it2->second.getAABBMin();
                auto maxB = it2->second.getAABBMax();

                if (AABBintersect(minA, maxA, minB, maxB)) {
                    std::cout << "Collision detected between "
                        << it1->first << " and " << it2->first << std::endl;
                    Particles::spawn(it1->second.origin, 5);
                    Particles::spawn(it2->second.origin, 5);
                    auto ent1 = entities.find(it1->first);
                    auto ent2 = entities.find(it2->first);
                    if (ent1 != entities.end()) ent1->second.reverseSpeedXZ();
                    if (ent2 != entities.end()) ent2->second.reverseSpeedXZ();
                }
            }
        }

        for (auto it = projectiles.begin(); it != projectiles.end(); ) {
            Entity& e = it->second;
            e.update(static_cast<float>(deltaTime), 0.0f);

            int index = 0;
            for (auto& [key, e] : projectiles) {
                e.update(deltaTime, 0.0f);

                if (index < lights.pointLights.size()) {
                    lights.pointLights[index].position = e.position;
                 ++index;
                }
            }

            // Delete cube
            if (glm::length(e.position - camera.position) > 10.0f) {
                scene.erase(scene.find(it->first));
                it = projectiles.erase(it);
            }
            else {
                ++it;
            }
        }


        /*
         *  --- SCENE RENDERING ---
         */

        std::vector<Model*> transparent;    // temporary, vector of pointers to transparent objects
        transparent.reserve(scene.size());  // reserve size for all objects to avoid reallocation

        double time = glfwGetTime();
        float sunAngle = float(time) * 0.2f;
        float daylight = glm::clamp(sin(sunAngle), 0.0f, 1.0f);
        float smoothDay = daylight * daylight;

        // Ambient: night / day changes
        lights.ambientLight.color = glm::vec3(0.6f, 0.5f, 0.1f) + glm::vec3(0.5f, 0.5f, 0.4f) * smoothDay;

        // Sun direction and color
        lights.sun.direction = glm::normalize(glm::vec3(cos(sunAngle), -0.5f, sin(sunAngle)));

        // Animate the last spotlight in a circle near the hmap center
        double t = glfwGetTime();
        float radius = 2.0f; // radius of movement
        float height = 4.0f; // height above hmap
        glm::vec3 hmapCenter = glm::vec3(0.0f, 0.0f, 0.0f);
        // Calculate position (X,Z circle, fixed Y)
        glm::vec3 spotPos = hmapCenter + glm::vec3(
            cos(t) * radius,
            height,
            sin(t) * radius
        );


        // Spotlight points downward
        glm::vec3 spotDir = glm::vec3(0.0f, -1.0f, 0.0f);
        // Update the spotlight in the lights struct
        lights.spotLights[movingSpotIndex].position = spotPos;
        lights.spotLights[movingSpotIndex].direction = spotDir;
        lights.spotLights[movingSpotIndex].ambient = glm::vec3(0.1f, 0.1f, 1.0f);

        // Pass lights to the main shader
        applyLights();

        terrain->draw(projectionMatrix, viewMatrix, camera.position);
        // Draw all models in the scene
        for (auto& [name, model] : scene) {
            if (!model.transparent) {
                model.draw(projectionMatrix, viewMatrix, camera.position);
            }
            else
                transparent.emplace_back(&model); // save pointer for painters algorithm
        }
        // THIRD PART - draw only transparent - painter's algorithm (sort by distance from camera, from far to near)
        std::sort(transparent.begin(), transparent.end(), [&](Model const* a, Model const* b) {
            return glm::distance(camera.position, a->origin) > glm::distance(camera.position, b->origin); // sort by distance from camera
            });
        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);
        for (auto p : transparent) {
            p->draw(projectionMatrix, viewMatrix, camera.position);
        }
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        // --- PARTICLE RENDERING ---
        // Use a simple shader for particles, or reuse one of your shaders
        Particles::drawParticles(projectionMatrix, viewMatrix, particleShader);

        // FPS calculation
        frameCount++;
        const double current_time = glfwGetTime();
        const double elapsed = current_time - lastTime;
        // update window title every second
        if (elapsed >= 1.0) {
            int fps = static_cast<int>(frameCount / elapsed);
            // show title + fps + vsync status
            std::string title = windowTitle + " [FPS: " + std::to_string(fps) + "], VSYNC: " + (vsync ? "ON" : "OFF") + ", AA: " + (AA ? "ON" : "OFF");
            glfwSetWindowTitle(window, title.c_str());

            frameCount = 0;
            lastTime = current_time;
        }

        glfwSwapBuffers(window);  // Update window content
        glfwPollEvents();         // Process pending events


    }
    return EXIT_SUCCESS;
    
}



void App::toggleFullscreen() {
    if (!window) return;

    if (!isFullscreen) {
        // Save current window position and size
        glfwGetWindowPos(window, &savedX, &savedY);
        glfwGetWindowSize(window, &savedWidth, &savedHeight);

        // Get primary monitor and its video mode
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // Switch to fullscreen
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        isFullscreen = true;
    }
    else {
        // Restore to windowed mode
        glfwSetWindowMonitor(window, nullptr, savedX, savedY, savedWidth, savedHeight, 0);
        isFullscreen = false;
    }
    
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    updateProjection();
}

App::~App() {
    // cleanup models and shaders
    scene.clear();
    shader.clear();
    delete terrain;

    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
    std::cout << "Application shutdown successfully\n";
}

// ----- callbacks ------
void App::mouse_clicked_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (app) app->shootProjectile();
    }
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (app) app->shootProjectile();
    }
}

void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    //std::cout << "Activate key_callback: Key pressed: " << key << std::endl;
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_V:
            app->vsync = !app->vsync;
            glfwSwapInterval(app->vsync ? 1 : 0);
            break;
        case GLFW_KEY_F11:
            app->toggleFullscreen();
            break;
        case GLFW_KEY_P:
            if (!app->AA) {
                glEnable(GL_MULTISAMPLE);
                app->AA = true;
            } else {
                glDisable(GL_MULTISAMPLE);
                app->AA = false;
            };
            break;
        default:
            break;
        }
    }
}

void App::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    //std::cout << "Activate cursor_position_callback." << std::endl;
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));

    if (app->firstMouse) {
        app->cursorLastX = xpos;
        app->cursorLastY = ypos;
        app->firstMouse = false;
    }

    // calculate offset with inverted Y axis (screen Y goes down, 3D Y goes up)
    float xoffset = xpos - app->cursorLastX;
    float yoffset = app->cursorLastY - ypos; // reversed since y-coordinates go bottom to top

    app->cursorLastX = xpos;
    app->cursorLastY = ypos;

    app->camera.ProcessMouseMovement(xoffset, yoffset);
}

void App::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->windowWidth = width;
        app->windowHeight = height;
        app->updateProjection();
    }
}
// -----------------------------------------
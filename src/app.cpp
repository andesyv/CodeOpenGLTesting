#include "app.h"
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr()
#include <glm/gtc/matrix_transform.hpp> // For glm::perspective
#include <glm/gtc/quaternion.hpp>       // glm::quat
#include <cstdlib>                      // For std::rand()
#include "shapes.h"

#include "modelloader.h"

App::App()
{
    AppSingleton::get().Instances.push_back(this);
}

int App::initGLFW()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw wp creation
    // --------------------
    wp = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CodeOpenGLTesting", NULL, NULL);
    if (wp == NULL)
    {
        std::cout << "Failed to create GLFW wp" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetWindowUserPointer(wp, this);
    glfwSetFramebufferSizeCallback(wp, framebuffer_size_callback);
    glfwSetScrollCallback(wp, scroll_callback);
    return 1;
}

int App::initOpenGL()
{
    glfwMakeContextCurrent(wp);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int versionMajor, versionMinor;
    glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
    glGetIntegerv(GL_MINOR_VERSION, &versionMinor);
    std::cout << "Running OpenGL version: " << versionMajor << "." << versionMinor << std::endl;

    glEnable(GL_DEBUG_OUTPUT);
    // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // If you want to ensure the error happens exactly after the error on the same thread.
    glDebugMessageCallback(&errorCallback, this);
    return 1;
}

void App::showFPS()
{
    static Timer timer{};
    static unsigned int frameCount{0};
    auto elapsed = timer.elapsed<std::chrono::milliseconds>();
    if (elapsed >= 1000)
    {
        const auto fps = frameCount * 1000.f / elapsed;
        std::string title{"CodeOpenGLTesting, fps: " + std::to_string(fps) + ", time dilation: " + std::to_string(timeDilation) + ", camera speed: " + std::to_string(cameraSpeed)};
        glfwSetWindowTitle(wp, title.c_str());
        frameCount = 0;
        timer.reset();
    }
    ++frameCount;
}

void App::processInput(float deltaTime)
{
    if (glfwGetKey(wp, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(wp, true);

    
    double xPos, yPos;
    glfwGetCursorPos(wp, &xPos, &yPos);
    double deltaX{xPos - mouseXPos}, deltaY{yPos - mouseYPos};
    deltaX *= deltaTime * CAMERA_ROTATION_SPEED;
    deltaY *= deltaTime * CAMERA_ROTATION_SPEED;
    mouseXPos = xPos;
    mouseYPos = yPos;
    
    auto& [pTrans, pCamera] = EM.get<component::trans, component::camera>(playerEntity);

    bool bAlt{glfwGetKey(wp, GLFW_KEY_LEFT_ALT) == GLFW_PRESS}, bRMB{glfwGetMouseButton(wp, 1) == GLFW_PRESS};
    if (bAlt || bRMB)
    {
        if (mouseWheelDist < -0.1f || 0.1f < mouseWheelDist)
            cameraSpeed += mouseWheelDist * 0.1f;
        if (glfwGetKey(wp, GLFW_KEY_UP) == GLFW_PRESS)
            cameraSpeed += 0.1f;        
        if (glfwGetKey(wp, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraSpeed -= 0.1f;

        cameraSpeed = std::max(cameraSpeed, 0.1f);



        const bool bObjectCentric = bAlt;
        const bool bChangedFromObjectCentric = !bObjectCentric && (pTrans.flags & pTrans.OBJECTCENTRIC);
        const bool bChangedToObjectCentric = bObjectCentric && !(pTrans.flags & pTrans.OBJECTCENTRIC);
        pTrans.flags = bObjectCentric ? pTrans.flags | pTrans.OBJECTCENTRIC : (pTrans.flags | pTrans.OBJECTCENTRIC) ^ pTrans.OBJECTCENTRIC;


        if (bChangedFromObjectCentric) {
            pTrans = component::trans::objectCentricToNormal(pTrans);
        } else if (bChangedToObjectCentric) {
            pTrans = component::trans::normalToObjectCentric(pTrans);
        }



        if (bRMB) {
            if (glfwGetKey(wp, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(wp, GLFW_KEY_E) == GLFW_PRESS)
                pTrans.pos += pTrans.upVector(glm::conjugate(pTrans.rot)) * cameraSpeed * deltaTime;

            if (glfwGetKey(wp, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(wp, GLFW_KEY_Q) == GLFW_PRESS)
                pTrans.pos -= pTrans.upVector(glm::conjugate(pTrans.rot)) * cameraSpeed * deltaTime;

            if (glfwGetKey(wp, GLFW_KEY_D) == GLFW_PRESS)
                pTrans.pos += pTrans.rightVector(glm::conjugate(pTrans.rot)) * cameraSpeed * deltaTime;

            if (glfwGetKey(wp, GLFW_KEY_A) == GLFW_PRESS)
                pTrans.pos -= pTrans.rightVector(glm::conjugate(pTrans.rot)) * cameraSpeed * deltaTime;

            // W and S are inverted because z in the coordinate system is inverted
            if (glfwGetKey(wp, GLFW_KEY_W) == GLFW_PRESS)
                pTrans.pos -= pTrans.forwardVector(glm::conjugate(pTrans.rot)) * cameraSpeed * deltaTime;

            if (glfwGetKey(wp, GLFW_KEY_S) == GLFW_PRESS)
                pTrans.pos += pTrans.forwardVector(glm::conjugate(pTrans.rot)) * cameraSpeed * deltaTime;
        } else {
            // W and S are inverted because z in the coordinate system is inverted
            if (glfwGetKey(wp, GLFW_KEY_W) == GLFW_PRESS)
                pTrans.pos.z -= cameraSpeed * deltaTime;

            if (glfwGetKey(wp, GLFW_KEY_S) == GLFW_PRESS)
                pTrans.pos.z += cameraSpeed * deltaTime;
        }



        pCamera.pitch += deltaY;
        pCamera.yaw += deltaX;
        pTrans.rot = glm::quat{std::cosf(pCamera.pitch * 0.5f), std::sinf(pCamera.pitch * 0.5f), 0.f, 0.f} *
                     glm::quat{std::cosf(pCamera.yaw * 0.5f), 0.f, std::sinf(pCamera.yaw * 0.5f), 0.f};

        pCamera.view = component::trans::createViewMat(pTrans, bObjectCentric);
    } else {
        if (mouseWheelDist < -0.1f || 0.1f < mouseWheelDist) {
            timeDilation += mouseWheelDist * 0.1f;
        }
        if (glfwGetKey(wp, GLFW_KEY_UP) == GLFW_PRESS)
            timeDilation += 0.1f;
        if (glfwGetKey(wp, GLFW_KEY_DOWN) == GLFW_PRESS)
            timeDilation -= 0.1f;
    }

    mouseWheelDist = 0.f;
}

void App::gameloop()
{
    // Find time since last frame
    const auto deltaTime = frameTimer.elapsed<std::chrono::milliseconds>() * 0.001f;
    frameTimer.reset();

    showFPS();

    // input
    // -----
    processInput(deltaTime);

    // Physics
    // Timer t{};
    calcPhysics(deltaTime * timeDilation);
    // std::cout << "Physics took " << t.elapsed<std::chrono::microseconds>() * 0.001f << "ms." << std::endl;

    // render
    // ------
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(0);
    unsigned int currentShader{0};

    const auto& [camera, playerTrans] = EM.get<component::camera, component::trans>(playerEntity);

    auto view = EM.view<component::mesh, component::mat, component::metadata>();
    for (const auto &entity : view)
    {
        glm::mat4 modelMat{1.f};
        // (Structured bindings ftw!! ENTT is so cool)
        auto &[mesh, material] = view.get<component::mesh, component::mat>(entity);

        // Set shader and shader-params (only if not already set)
        if (material.shader != currentShader) {
            currentShader = material.shader;
            glUseProgram(material.shader);
            glUniformMatrix4fv(glGetUniformLocation(material.shader, "uProj"), 1, GL_FALSE, glm::value_ptr(camera.proj));
            glUniformMatrix4fv(glGetUniformLocation(material.shader, "uView"), 1, GL_FALSE, glm::value_ptr(camera.view));

            glm::vec3 lightPos{0.f, 0.f, 0.f};
            glUniform3fv(glGetUniformLocation(material.shader, "lightPos"), 1, glm::value_ptr(lightPos));
            glUniform3fv(glGetUniformLocation(material.shader, "cameraPos"), 1, glm::value_ptr(playerTrans.pos));
        }

        // Assign a model matrix if it exist
        if (EM.has<component::trans>(entity))
        {
            auto& transform = EM.get<component::trans>(entity);
            // if (view.get<component::metadata>(entity).name != "plane")
            //     transform.rot *= glm::quat{std::cosf(deltaTime * 0.5f), transform.right() * std::sinf(deltaTime * 0.5f)};
            // transform.pos.x += deltaTime * 0.1f;
            modelMat = transform.mat();
        }
        glUniformMatrix4fv(glGetUniformLocation(material.shader, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniform3fv(glGetUniformLocation(material.shader, "color"), 1, glm::value_ptr(material.color));

        glBindVertexArray(mesh.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        if (mesh.bIndices)
            glDrawElements(mesh.drawMode, mesh.indexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(mesh.drawMode, 0, mesh.vertexCount);
    }
    glBindVertexArray(0); // no need to unbind it every time

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(wp);
    glfwPollEvents();
}

/**
 * Note: For ekstra precision during physics calculations
 * we promote variables to doubles.
 */
void App::calcPhysics(float deltaTime)
{
    if (deltaTime <= MIN_TICK_TIME)
        return;

    const auto time = static_cast<double>(deltaTime);

    auto view = EM.view<component::trans, component::phys>();

    unsigned int i{0};
    for (auto it{view.begin()}; it != view.end(); ++it, ++i) {
        auto &[t, p] = view.get<component::trans, component::phys>(*it);

        glm::dvec3 f{0.f, 0.f, 0.f};

        for (auto other{view.begin()}; other != view.end(); ++other) {
            if (it == other)
                continue;
            
            auto& [t2, p2] = view.get<component::trans, component::phys>(*other);
            glm::dvec3 dist = t2.pos - t.pos;

            f += glm::normalize(dist) * calcGravity(p.mass, p2.mass, dist.length());
        }

        // std::cout << "a: " << glm::length(a) << ", deltaTime: " << deltaTime << ", a * deltaTime: " << glm::length(a *deltaTime) << std::endl;
        const auto a = f / static_cast<double>(p.mass);
        if (glm::any(glm::isnan(a)))
            continue;
        p.vel += a * time;
        // Demote double to float for final calculation. (No need to keep variable if it cannot be stored)
        t.pos += static_cast<glm::vec3>(p.vel * time);
    }
}

int App::init(int currentReward)
{
    if (1 < currentReward)
        return 3;
    else if (0 < currentReward)
        return init(currentReward + 2 * initOpenGL());
    else if (-1 < currentReward)
        return init(currentReward + initGLFW());
    else
        return 0;
}

int App::exec()
{
    if (!init())
        return -1;

    glfwMakeContextCurrent(wp);
    // Reset cursor position
    glfwGetCursorPos(wp, &mouseXPos, &mouseYPos);

    // ================== Setup scene ====================================
    setupScene();

    std::cout << "Setup took " << appTimer.elapsed<std::chrono::milliseconds>() << "ms." << std::endl;
    appTimer.reset();
    frameTimer.reset();

    // render loop
    // -----------
    while (!glfwWindowShouldClose(wp))
    {
        gameloop();
    }

    // // optional: de-allocate all resources once they've outlived their purpose:
    // // ------------------------------------------------------------------------
    cleanupScene();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void App::setupScene()
{
    Shader defaultShader{"src/shaders/default.vert", "src/shaders/default.frag"};
    Shader colorShader{"src/shaders/default.vert", "src/shaders/color.frag"};
    Shader phongShader{"src/shaders/phong.vert", "src/shaders/phong.frag"};

    glEnable(GL_CULL_FACE);
    // glFrontFace()
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // uncomment this call to draw in wireframe polygons.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Setup player
    auto entity = EM.create();
    EM.emplace<component::trans>(entity, component::trans{.pos{0.f, 0.f, 40.f}, .rot{
        glm::quat{std::cosf(0.f), std::sinf(0.f), 0.f, 0.f} *
        glm::quat{std::cosf(0.f), 0.f, std::sinf(0.f), 0.f}
    }});
    EM.emplace<component::metadata>(entity, "player");
    auto &camera = EM.emplace<component::camera>(entity);
    int width, height;
    glfwGetFramebufferSize(wp, &width, &height);
    /**
     * glm::perspective makes a right hand coordinate system,
     * meaning that x is right, y is up and negative z is forward.
     * (z is flipped from world space to window space)
     */
    camera.proj = glm::perspective(glm::radians(camera.FOV), static_cast<float>(width) / height, 0.1f, 100.f);
    camera.view = component::trans::createViewMat(EM.get<component::trans>(entity));
    playerEntity = entity;



    // ------------- Axis: ------------------------------
    entity = EM.create();
    EM.emplace<component::mat>(entity, colorShader.get());
    EM.emplace<component::metadata>(entity, "axis");
    auto mesh = &EM.emplace<component::mesh>(entity);
    // EM.emplace<component::trans>(entity);

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &mesh->VAO);
    glBindVertexArray(mesh->VAO);

    glGenBuffers(1, &mesh->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(shapes::axis), shapes::axis.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    mesh->vertexCount = static_cast<unsigned int>(shapes::axis.size());
    mesh->drawMode = GL_LINES;




    // ----------- Cube: ------------------------------
    auto cubeEnt = entity = EM.create();
    EM.emplace<component::mat>(entity, colorShader.get());
    mesh = &EM.emplace<component::mesh>(entity);
    EM.emplace<component::trans>(entity);
    EM.emplace<component::metadata>(entity, "cube");
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------


    // auto obj = ModelLoader::load("src/models/samples/Cube.gltf");
    // std::cout << "Vertex count: " << obj.first.size() << ", index count: " << obj.second.size() << std::endl;
    // if (!ModelLoader::save(obj, "src/models/CopyCube.gltf")) {
    //     std::cout << "Saving failed" << std::endl;
    // }

    // obj = ModelLoader::load("src/models/CopyCube.gltf");
    // std::cout << "Vertex count: " << obj.first.size() << ", index count: " << obj.second.size() << std::endl;


    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->IBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(mesh->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(shapes::cube), shapes::cube.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(shapes::cubeIndices), shapes::cubeIndices.data(), GL_STATIC_DRAW);
    // GLint bufferSize;
    // glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
    // std::cout << "buffer size: " << bufferSize << std::endl;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    mesh->vertexCount = static_cast<unsigned int>(shapes::cube.size());
    mesh->bIndices = true;
    // std::tuple_size to get length of array. Surprisingly that it doesn't already exist in std::array
    // https://stackoverflow.com/questions/21936507/why-isnt-stdarraysize-static
    mesh->indexCount = static_cast<unsigned int>(shapes::cubeIndices.size());

    // // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    // glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);







    entity = EM.create();
    EM.emplace<component::mat>(entity, phongShader.get(), glm::vec3{0.7f, 0.2f, 0.2f});
    EM.emplace<component::trans>(entity) = {.pos{0.f, -1.f, 0.f}, .scale{2.f}};
    EM.emplace<component::metadata>(entity, "plane");
    mesh = &EM.emplace<component::mesh>(entity);
    glCreateVertexArrays(1, &mesh->VAO);
    glBindVertexArray(mesh->VAO);
    
    glCreateBuffers(1, &mesh->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(shapes::plane), shapes::plane.data(), GL_STATIC_DRAW);
    mesh->vertexCount = static_cast<unsigned int>(shapes::plane.size());

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);





    // --------------------------- Sphere ----------------------------
    entity = EM.create();
    auto sphereEnt = entity;
    EM.emplace<component::mat>(entity, colorShader.get(), glm::vec3{1.f, 0.f, 0.f});
    EM.emplace<component::trans>(entity);
    EM.emplace<component::metadata>(entity, "ball");
    mesh = &EM.emplace<component::mesh>(entity);
    glCreateVertexArrays(1, &mesh->VAO);
    glBindVertexArray(mesh->VAO);

    glCreateBuffers(1, &mesh->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    auto ballVertices = shapes::cubeSphere(3);
    // Have to use ballvertice.size() * sizeof(vertex) because it's a std::vector
    glBufferData(GL_ARRAY_BUFFER, ballVertices.size() * sizeof(vertex), ballVertices.data(), GL_STATIC_DRAW);
    mesh->vertexCount = static_cast<unsigned int>(ballVertices.size());
    mesh->drawMode = GL_TRIANGLES;
    mesh->bIndices = false;
    

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);






    auto getRandDeg = []() {
        return (std::rand() % 100) * 0.01f * 6.28f;
    };
    auto getRandPointInUnitSphere = [=]() {
        auto x{std::rand() % 100 * 0.01f * 6.28f}, y{std::acosf(std::rand() % 100 * 0.02f - 1)};
        // std::cout << "x, y: " << x << ", " << y << std::endl;
        return glm::vec3{std::sinf(y) * std::cosf(x), std::sinf(y) * std::sinf(x), std::cosf(y)};
    };
    auto getRandColor = []() {
        return glm::vec3{rand() % 100 * 0.01f, rand() % 100 * 0.01f, rand() % 100 * 0.01f};
    };
    auto getMassFromSize = [](const component::trans& trans) {
        float radius = (trans.flags & component::trans::SPHERE) ? trans.scale.x : glm::length(trans.scale);
        float vol = 12.57f * std::powf(radius, 3.f) / 3.f;
        return 10000.f * vol;
    };
    for (unsigned int i{0}, max{30}; i < max; ++i)
    {
        entity = EM.create();
        EM.emplace<component::mat>(entity, phongShader.get(), getRandColor());
        auto &trans = EM.emplace<component::trans>(entity);
        auto deg = getRandDeg();
        auto dir = getRandPointInUnitSphere();
        // std::cout << "Rand deg : " << deg << ", rand dir: " << dir.x << ", " << dir.y << ", " << dir.z << std::endl;
        trans.flags |= trans.SPHERE;
        trans.pos = dir * (std::rand() % 100 * 0.1f + 100.f);
        // std::cout << "Startpos: " << trans.pos.x << ", " << trans.pos.y << ", " << trans.pos.z << std::endl;
        trans.rot = glm::quat{std::cosf(deg * 0.5f), dir * std::sinf(deg * 0.5f)};
        trans.scale = glm::vec3{std::rand() % 40 * 0.1f};
        // Copy the mesh component (use same VAO)
        EM.emplace<component::mesh>(entity, EM.get<component::mesh>(sphereEnt));
        EM.emplace<component::metadata>(entity, std::string{"plane "}.append(std::to_string(i)));
        EM.emplace<component::phys>(entity, getMassFromSize(trans), getRandPointInUnitSphere() * (std::rand() % 100 * 0.01f));
    }


    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);
}

void App::cleanupScene()
{
    auto view = EM.view<component::mesh>();
    for (auto entity : view)
    {
        auto &mesh = view.get<component::mesh>(entity);

        if (mesh.bIndices)
            glDeleteBuffers(1, &mesh.IBO);
        
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteVertexArrays(1, &mesh.VAO);
    }
}

void App::framebuffer_size_callback(GLFWwindow *wp, int width, int height)
{
    // make sure the viewport matches the new wp dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);

    auto app = static_cast<App *>(glfwGetWindowUserPointer(wp));
    assert(app != nullptr);
    auto &camera = app->EM.get<component::camera>(app->playerEntity);
    /**
         * glm::perspective makes a right hand coordinate system,
         * meaning that x is right, y is up and negative z is forward.
         * (z is flipped from world space to window space)
         */
    camera.proj = glm::perspective(glm::radians(camera.FOV), static_cast<float>(width) / height, 1.f, 1000.f);
}

void App::errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    std::string sourceStr{enumToString(source,
                                        ESPair{GL_DEBUG_SOURCE_API, "GL_DEBUG_SOURCE_API"},
                                        ESPair{GL_DEBUG_SOURCE_WINDOW_SYSTEM, "GL_DEBUG_SOURCE_WINDOW_SYSTEM"},
                                        ESPair{GL_DEBUG_SOURCE_SHADER_COMPILER, "GL_DEBUG_SOURCE_SHADER_COMPILER"},
                                        ESPair{GL_DEBUG_SOURCE_THIRD_PARTY, "GL_DEBUG_SOURCE_THIRD_PARTY"},
                                        ESPair{GL_DEBUG_SOURCE_APPLICATION, "GL_DEBUG_SOURCE_APPLICATION"},
                                        ESPair{GL_DEBUG_SOURCE_OTHER, "GL_DEBUG_SOURCE_OTHER"})};
    std::string typeStr{enumToString(type,
                                        ESPair{GL_DEBUG_TYPE_ERROR, "GL_DEBUG_TYPE_ERROR"},
                                        ESPair{GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"},
                                        ESPair{GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"},
                                        ESPair{GL_DEBUG_TYPE_PORTABILITY, "GL_DEBUG_TYPE_PORTABILITY"},
                                        ESPair{GL_DEBUG_TYPE_PERFORMANCE, "GL_DEBUG_TYPE_PERFORMANCE"},
                                        ESPair{GL_DEBUG_TYPE_MARKER, "GL_DEBUG_TYPE_MARKER"},
                                        ESPair{GL_DEBUG_TYPE_PUSH_GROUP, "GL_DEBUG_TYPE_PUSH_GROUP"},
                                        ESPair{GL_DEBUG_TYPE_POP_GROUP, "GL_DEBUG_TYPE_POP_GROUP"},
                                        ESPair{GL_DEBUG_TYPE_OTHER, "GL_DEBUG_TYPE_OTHER"})};
    std::string severityStr{enumToString(severity,
                                            ESPair{GL_DEBUG_SEVERITY_HIGH, "GL_DEBUG_SEVERITY_HIGH"},
                                            ESPair{GL_DEBUG_SEVERITY_MEDIUM, "GL_DEBUG_SEVERITY_MEDIUM"},
                                            ESPair{GL_DEBUG_SEVERITY_LOW, "GL_DEBUG_SEVERITY_LOW"},
                                            ESPair{GL_DEBUG_SEVERITY_NOTIFICATION, "GL_DEBUG_SEVERITY_NOTIFICATION"})};

    // Close if severe error.
    if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM) {
        auto app = (App*)userParam;
        glfwSetWindowShouldClose(app->wp, true);
    }

    std::cout << "GL_ERROR: (source: " << sourceStr << ", type: " << typeStr << ", severity: " << severityStr << ", message: " << message << std::endl;
}
#include "particle.h"
#include "physics.h"
#include "seqnbody.h"
#include "octree.h"
#include "cosntlib.h"
#include "camera.h"
#include "generate.h"
#include "shader.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <chrono>
#include "menu.h"

// Debug function to check OpenGL errors
void checkGLError(const char* operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::string errorString;
        switch (error) {
            case GL_INVALID_ENUM: errorString = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: errorString = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: errorString = "GL_INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY: errorString = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default: errorString = "UNKNOWN_ERROR"; break;
        }
        std::cerr << "OpenGL error after " << operation << ": " << errorString << " (" << error << ")" << std::endl;
    }
}
void stabilizeOrbits(ParticleSystem& particleSystem, float damping = 0.9995f) {
    // fun maths to recalculate velocity so it stablizilizes and simulates friction (ignoring the blackhole)
    for (size_t j = 1; j < particleSystem.size(); j++) {
        
        glm::vec3 pos(particleSystem[j].position);
        glm::vec3 vel(particleSystem[j].velocity);
        glm::vec3 center(particleSystem[0].position); 
        
        glm::vec3 toCenter = center - pos;
        float dist = glm::length(toCenter);
        
        if (dist < 0.1f) continue; 
        
        glm::vec3 dirToCenter = toCenter / dist;
        
        float radialVelocity = glm::dot(vel, dirToCenter);
        
        glm::vec3 tangentialDir = glm::cross(glm::cross(dirToCenter, vel), dirToCenter);
        if (glm::length(tangentialDir) > 0.0001f) {
            tangentialDir = glm::normalize(tangentialDir);
        }
        
        glm::vec3 tangentialVelocity = vel - (radialVelocity * dirToCenter);
        
        glm::vec3 newVel = tangentialVelocity + radialVelocity * dirToCenter * 0.95f;
        
        newVel *= damping;
        
        particleSystem[j].velocity = glm::vec4(newVel, 0.0f);
    }
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

unsigned int createQuadVAO() {
    //creating a quad that basically makes it easier to apply shaders to the entire screen
    float quadVertices[] = {
        // positions        // texture coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    
    return quadVAO;
}

unsigned int createFramebuffer(unsigned int& textureColorBuffer, unsigned int width, unsigned int height) {
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);
    
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return framebuffer;
}

int main()
{

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "N-Body Simulation", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback); // Add this line
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE); 

    // ImGui initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    unsigned int galaxyShader = createShaderFromFiles("galaxy.vert", "galaxy.frag");
    unsigned int blurShader = createShaderFromFiles("blur.vert", "blur.frag");
    unsigned int postShader = createShaderFromFiles("post.vert", "post.frag");
    
    unsigned int quadVAO = createQuadVAO();
    
    unsigned int galaxyFBO1, galaxyFBO2, blurFBO1, blurFBO2;
    unsigned int galaxyColorBuffer1, galaxyColorBuffer2, blurColorBuffer1, blurColorBuffer2;
    galaxyFBO1 = createFramebuffer(galaxyColorBuffer1, SCR_WIDTH, SCR_HEIGHT);
    galaxyFBO2 = createFramebuffer(galaxyColorBuffer2, SCR_WIDTH, SCR_HEIGHT);
    blurFBO1 = createFramebuffer(blurColorBuffer1, SCR_WIDTH, SCR_HEIGHT);
    blurFBO2 = createFramebuffer(blurColorBuffer2, SCR_WIDTH, SCR_HEIGHT);

    unsigned int particleVAO, particleVBO;
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);
    
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * sizeof(Particle), NULL, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)(3 * sizeof(float) + sizeof(glm::vec4) * 2));
    glEnableVertexAttribArray(1);

    Particle* particles = new Particle[MAX_PARTICLES];
    generateRandomGalaxy(particles, numParticles);
    
    ParticleSystem particleSystem(particles, numParticles);
    SequentialNBodySimulator seqSimulator(particleSystem, physicsTimeStep);
    BarnesHutCPUSimulator bhSimulator(particleSystem, physicsTimeStep, theta);

    int colorType = 0; 
    bool enablePostProcessing = true;
    
    float fps = 0.0f;
    float frameTime = 0.0f;
    float simulationTime = 0.0f;
    int frameCount = 0;
    auto lastTime = std::chrono::high_resolution_clock::now();

    SimulationMenu menu;

    menu.initialize(
        [&](glm::vec3& pos, glm::vec3& front, glm::vec3& up, float& yawVal, float& pitchVal) {
            cameraPos = pos;
            cameraFront = front;
            cameraUp = up;
            yaw = yawVal;
            pitch = yawVal;
        },
        [&](bool enabled) {
            glfwSetInputMode(window, GLFW_CURSOR, enabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        },
        [&](const char* name, float value) {
            glUseProgram(postShader);
            glUniform1f(glGetUniformLocation(postShader, name), value);
        },
        [&](const char* name, int value) {
            glUseProgram(postShader);
            glUniform1i(glGetUniformLocation(postShader, name), value);
        }
    );

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(window);
        
        auto simStart = std::chrono::high_resolution_clock::now();
        
        if (!pauseSimulation) {
            for (int i = 0; i < simSpeed; i++) {
                
                stabilizeOrbits(particleSystem);
                
                if (simulationType == 0) {
                    seqSimulator.update();
                } else {
                    bhSimulator.update();
                }
            }
        }
        
        auto simEnd = std::chrono::high_resolution_clock::now();
        simulationTime = std::chrono::duration<float, std::milli>(simEnd - simStart).count();
        
        frameCount++;
        auto currentTime = std::chrono::high_resolution_clock::now();
        float timeDiff = std::chrono::duration<float>(currentTime - lastTime).count();
        if (timeDiff >= 1.0f) {
            fps = frameCount / timeDiff;
            frameTime = 1000.0f / fps;
            frameCount = 0;
            lastTime = currentTime;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(Particle), particles);
        
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp = projection * view * model;
        
        if (enablePostProcessing) {
            glBindFramebuffer(GL_FRAMEBUFFER, galaxyFBO1);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            glBindFramebuffer(GL_FRAMEBUFFER, galaxyFBO2);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            glBindFramebuffer(GL_FRAMEBUFFER, blurFBO1);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            glBindFramebuffer(GL_FRAMEBUFFER, blurFBO2);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            glBindFramebuffer(GL_FRAMEBUFFER, galaxyFBO1);
            glDisable(GL_POINT_SMOOTH);
            glDisable(GL_LINE_SMOOTH);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            
            glUseProgram(galaxyShader);
            glUniformMatrix4fv(glGetUniformLocation(galaxyShader, "u_mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
            glBindVertexArray(particleVAO);
            glDrawArrays(GL_POINTS, 0, numParticles);
            
            glBindFramebuffer(GL_FRAMEBUFFER, galaxyFBO2);
            glEnable(GL_POINT_SMOOTH);
            glEnable(GL_LINE_SMOOTH);
            
            glUseProgram(galaxyShader);
            glUniformMatrix4fv(glGetUniformLocation(galaxyShader, "u_mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
            glBindVertexArray(particleVAO);
            glDrawArrays(GL_POINTS, 0, numParticles);
            
            glBindFramebuffer(GL_FRAMEBUFFER, blurFBO1);
            glUseProgram(blurShader);
            glUniform1i(glGetUniformLocation(blurShader, "u_texture"), 0);
            glUniform1i(glGetUniformLocation(blurShader, "u_horizontal"), 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, galaxyColorBuffer2);
            
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            glBindFramebuffer(GL_FRAMEBUFFER, blurFBO2);
            glUseProgram(blurShader);
            glUniform1i(glGetUniformLocation(blurShader, "u_texture"), 0);
            glUniform1i(glGetUniformLocation(blurShader, "u_horizontal"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, blurColorBuffer1);
            
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            glUseProgram(postShader);
            glUniform1i(glGetUniformLocation(postShader, "u_color_type"), colorType);
            glUniform1i(glGetUniformLocation(postShader, "u_galaxy"), 0);
            glUniform1i(glGetUniformLocation(postShader, "u_blur"), 1);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, galaxyColorBuffer1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, blurColorBuffer2);
            
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            glUseProgram(galaxyShader);
            glUniformMatrix4fv(glGetUniformLocation(galaxyShader, "u_mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
            glBindVertexArray(particleVAO);
            glDrawArrays(GL_POINTS, 0, numParticles);
        }
        
        menu.updatePerformanceMetrics(fps, frameTime, simulationTime);

        bool galaxyRegenerated = menu.renderMenu(particles, particleSystem, seqSimulator, bhSimulator);

        if (galaxyRegenerated) {
            particleSystem = ParticleSystem(particles, numParticles);
            seqSimulator = SequentialNBodySimulator(particleSystem, physicsTimeStep);
            bhSimulator = BarnesHutCPUSimulator(particleSystem, physicsTimeStep, theta);
        }

        pauseSimulation = menu.isPaused();
        simulationType = menu.getSimulationType();
        simSpeed = menu.getSimSpeed();
        physicsTimeStep = menu.getTimeStep();
        theta = menu.getTheta();
        enablePostProcessing = menu.isPostProcessingEnabled();
        colorType = menu.getColorType();
        numParticles = menu.getNumParticles();
        cameraSpeed = menu.getCameraSpeed();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    delete[] particles;
    
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleVBO);
    glDeleteVertexArrays(1, &quadVAO);
    
    glDeleteFramebuffers(1, &galaxyFBO1);
    glDeleteFramebuffers(1, &galaxyFBO2);
    glDeleteFramebuffers(1, &blurFBO1);
    glDeleteFramebuffers(1, &blurFBO2);
    
    glDeleteTextures(1, &galaxyColorBuffer1);
    glDeleteTextures(1, &galaxyColorBuffer2);
    glDeleteTextures(1, &blurColorBuffer1);
    glDeleteTextures(1, &blurColorBuffer2);
    
    glDeleteProgram(galaxyShader);
    glDeleteProgram(blurShader);
    glDeleteProgram(postShader);
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwTerminate();
    return 0;
}


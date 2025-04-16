#ifndef MENU_H
#define MENU_H

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <string>
#include "particle.h"
#include "bhut.h"
#include "seqnbody.h"
#include "generate.h"
#include <functional>
#include "cosntlib.h"
#include "cuda_simulator.h"
class SimulationMenu
{
private:
    // Performance metrics
    float fps = 0.0f;
    float frameTime = 0.0f;
    float simulationTime = 0.0f;

    // Simulation settings
    bool pauseSimulation = false;
    int simulationType = 1;
    float simSpeed = 1.0f;
    float physicsTimeStep = 0.01f;
    float theta = 0.5f;

    // Visual settings
    bool enablePostProcessing = true;
    int colorType = 0;
    float exposureValue = 1.5f;
    bool chromaticAberration = true;
    float starDensity = 0.997f;

    // Galaxy settings
    int galaxyType = 0;
    int numParticles = 1000;

    // Camera settings
    bool cameraEnabled = false;
    float cameraSpeed = 5.0f;

    // Callbacks
    std::function<void(glm::vec3 &, glm::vec3 &, glm::vec3 &, float &, float &)> resetCameraFunc;
    std::function<void(bool)> toggleCameraFunc;

    // Uniforms callback
    std::function<void(const char *, float)> setUniformFloatFunc;
    std::function<void(const char *, int)> setUniformIntFunc;

public:
    SimulationMenu() {}

    void initialize(
        std::function<void(glm::vec3 &, glm::vec3 &, glm::vec3 &, float &, float &)> resetCamera,
        std::function<void(bool)> toggleCamera,
        std::function<void(const char *, float)> setUniformFloat,
        std::function<void(const char *, int)> setUniformInt)
    {
        resetCameraFunc = resetCamera;
        toggleCameraFunc = toggleCamera;
        setUniformFloatFunc = setUniformFloat;
        setUniformIntFunc = setUniformInt;
    }

    void updatePerformanceMetrics(float newFps, float newFrameTime, float newSimTime)
    {
        fps = newFps;
        frameTime = newFrameTime;
        simulationTime = newSimTime;
    }

    bool isPaused() const { return pauseSimulation; }
    int getSimulationType() const { return simulationType; }
    float getSimSpeed() const { return simSpeed; }
    float getTimeStep() const { return physicsTimeStep; }
    float getTheta() const { return theta; }
    bool isPostProcessingEnabled() const { return enablePostProcessing; }
    int getColorType() const { return colorType; }
    float getExposure() const { return exposureValue; }
    bool isChromaticAberrationEnabled() const { return chromaticAberration; }
    float getStarDensity() const { return starDensity; }
    int getGalaxyType() const { return galaxyType; }
    int getNumParticles() const { return numParticles; }
    bool isCameraEnabled() const { return cameraEnabled; }
    float getCameraSpeed() const { return cameraSpeed; }

    bool renderMenu(Particle *particles, ParticleSystem &particleSystem,
                    SequentialNBodySimulator &seqSimulator,
                    BarnesHutCPUSimulator &bhSimulator,
                    CUDANBodySimulator &cudaSimulator) // added cuda to the simulator
    {
        bool galaxyRegenerated = false;

        // ImGui new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui window
        ImGui::Begin("N-Body Simulation Controls");

        renderPerformanceSection();
        renderSimulationControls(bhSimulator, cudaSimulator);
        renderVisualSettings();
        galaxyRegenerated = renderGalaxySettings(particles, particleSystem, seqSimulator, bhSimulator, cudaSimulator);
        renderCameraControls();

        ImGui::End();

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        return galaxyRegenerated;
    }

private:
    void renderPerformanceSection()
    {
        ImGui::Text("Performance Metrics");
        ImGui::Text("FPS: %.1f (%.1f ms/frame)", fps, frameTime);
        ImGui::Text("Simulation Time: %.1f ms", simulationTime);
        ImGui::Text("Particles: %d", numParticles);
        ImGui::Separator();
    }

    void renderSimulationControls(BarnesHutCPUSimulator &bhSimulator, CUDANBodySimulator &cudaSimulator)
    {
        ImGui::Text("Simulation Controls");
        if (ImGui::Button(pauseSimulation ? "Resume" : "Pause"))
        {
            pauseSimulation = !pauseSimulation;
        }

        const char *simTypes[] = {"Sequential", "Barnes-Hut", "CUDA"};
        ImGui::Combo("Simulation Type", &simulationType, simTypes, IM_ARRAYSIZE(simTypes));

        ImGui::SliderFloat("Speed", &simSpeed, 0.1f, 10.0f, "%.1f");
        ImGui::SliderFloat("Time Step", &physicsTimeStep, 0.001f, 0.1f, "%.3f");

        if (simulationType == 1)
        {
            ImGui::SliderFloat("Theta", &theta, 0.1f, 1.0f, "%.2f");
            ImGui::Text("Barnes-Hut Optimizations:");

            static bool adaptiveTheta = true;
            if (ImGui::Checkbox("Adaptive Theta", &adaptiveTheta))
            {
                bhSimulator.setAdaptiveTheta(adaptiveTheta);
            }

            static int rebuildFrequency = 1;
            if (ImGui::SliderInt("Tree Rebuild Frequency", &rebuildFrequency, 1, 10))
            {
                bhSimulator.setRebuildFrequency(rebuildFrequency);
            }

            static bool showProfiling = false;
            if (ImGui::Checkbox("Show Performance Metrics", &showProfiling))
            {
                bhSimulator.enableProfilingOutput(showProfiling);
            }
        }
        else if (simulationTime == 2)
        {
            ImGui::Text("CUDA Optimizations:");

            static bool showCudaProfiling = false;
            if (ImGui::Checkbox("Show CUDA Performance Metrics", &showCudaProfiling))
            {
                cudaSimulator.enableProfilingOutput(showCudaProfiling);
            }

            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                               "CUDA acceleration active");
        }

        static float blackHoleMass = 1000.0f;
        if (ImGui::SliderFloat("Black Hole Mass", &blackHoleMass, 100.0f, 5000.0f, "%.0f"))
        {
            // This will be handled externally when regenerating the galaxy
        }

        ImGui::Separator();
    }

    void renderVisualSettings()
    {
        ImGui::Text("Visual Settings");
        ImGui::Checkbox("Enable Post-Processing", &enablePostProcessing);

        const char *colorSchemes[] = {"Blue", "Red", "Purple"};
        if (ImGui::Combo("Color Scheme", &colorType, colorSchemes, IM_ARRAYSIZE(colorSchemes)))
        {
            setUniformIntFunc("u_color_type", colorType);
        }

        if (ImGui::SliderFloat("Exposure", &exposureValue, 0.5f, 3.0f, "%.1f"))
        {
            setUniformFloatFunc("u_exposure", exposureValue);
        }

        if (ImGui::Checkbox("Chromatic Aberration", &chromaticAberration))
        {
            setUniformIntFunc("u_chromatic_aberration", chromaticAberration ? 1 : 0);
        }

        if (ImGui::SliderFloat("Star Density", &starDensity, 0.99f, 0.999f, "%.3f"))
        {
            setUniformFloatFunc("u_star_density", starDensity);
        }

        ImGui::Separator();
    }

    bool renderGalaxySettings(Particle *particles, ParticleSystem &particleSystem,
                              SequentialNBodySimulator &seqSimulator,
                              BarnesHutCPUSimulator &bhSimulator,
                              CUDANBodySimulator &cudaSimulator)
    {
        bool galaxyRegenerated = false;

        ImGui::Text("Galaxy Configuration");
        const char *galaxyTypes[] = {"Random", "Disk", "Spiral", "Collision", "Dense"};
        bool galaxyChanged = ImGui::Combo("Galaxy Type", &galaxyType, galaxyTypes, IM_ARRAYSIZE(galaxyTypes));

        bool particleCountChanged = ImGui::SliderInt("Particle Count", &numParticles, 100, MAX_PARTICLES);

        if (galaxyChanged || particleCountChanged || ImGui::Button("Generate New Galaxy"))
        {
            switch (galaxyType)
            {
            case 0:
                generateRandomGalaxy(particles, numParticles);
                break;
            case 1:
                generateDiskGalaxy(particles, numParticles);
                break;
            case 2:
                generateSpiralGalaxy(particles, numParticles);
                break;
            case 3:
                generateCollisionGalaxy(particles, numParticles);
                break;
            case 4:
                generateDenseDiskGalaxy(particles, numParticles);
                break;
            }

            // The main renderer will update the simulation with the new particles
            galaxyRegenerated = true;
        }

        ImGui::Separator();

        return galaxyRegenerated;
    }

    void renderCameraControls()
    {
        ImGui::Text("Camera Controls");
        if (ImGui::Button(cameraEnabled ? "Disable Camera" : "Enable Camera"))
        {
            cameraEnabled = !cameraEnabled;
            toggleCameraFunc(cameraEnabled);
        }

        ImGui::SliderFloat("Camera Speed", &cameraSpeed, 1.0f, 20.0f, "%.1f");

        if (ImGui::Button("Reset Camera"))
        {
            // Call the reset camera function provided during initialization
            glm::vec3 cameraPos(0.0f, 0.0f, 50.0f);
            glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
            glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
            float yaw = -90.0f;
            float pitch = 0.0f;
            resetCameraFunc(cameraPos, cameraFront, cameraUp, yaw, pitch);
        }

        ImGui::Text("WASD: Move camera");
        ImGui::Text("QE: Move up/down");
        ImGui::Text("Mouse: Look around (when camera enabled)");
        ImGui::Text("Space: Pause/Resume simulation");
        ImGui::Text("Esc: Exit");
    }
};

#endif // MENU_H
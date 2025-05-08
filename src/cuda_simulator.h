#ifndef CUDA_SIMULATOR_H
#define CUDA_SIMULATOR_H

#include "particle.h"
#include "physics.h"
#include "cuda_nbody.h"
#include <memory>
#include <chrono>
#include <iostream>

// file to make sure cuda works in the menu
class CUDANBodySimulator
{
private:
    std::shared_ptr<ParticleSystem> particles;
    float timeStep;
    float G;
    float softening;

    ParticleGPU *devParticles;
    bool gpuInitialized;

    bool enableProfiling;

    std::chrono::high_resolution_clock::time_point startTime;
    float copyToGPUTime;
    float computeTime;
    float copyFromGPUTime;
    float totalTime;

public:
    CUDANBodySimulator(ParticleSystem &particleSystem, float dt,
                       float G = Physics::G, float softening = Physics::SOFTENING)
        : particles(std::make_shared<ParticleSystem>(particleSystem)),
          timeStep(dt), G(G), softening(softening),
          devParticles(nullptr), gpuInitialized(false),
          enableProfiling(false),
          copyToGPUTime(0.0f), computeTime(0.0f), copyFromGPUTime(0.0f), totalTime(0.0f)
    {
        initializeGPU();
    }

    ~CUDANBodySimulator()
    {
        cleanupGPU();
    }

    CUDANBodySimulator(const CUDANBodySimulator &) = delete;
    CUDANBodySimulator &operator=(const CUDANBodySimulator &) = delete;

    CUDANBodySimulator(CUDANBodySimulator &&other) noexcept
        : particles(std::move(other.particles)),
          timeStep(other.timeStep),
          G(other.G),
          softening(other.softening),
          devParticles(other.devParticles),
          gpuInitialized(other.gpuInitialized),
          enableProfiling(other.enableProfiling),
          copyToGPUTime(other.copyToGPUTime),
          computeTime(other.computeTime),
          copyFromGPUTime(other.copyFromGPUTime),
          totalTime(other.totalTime)
    {
        other.devParticles = nullptr;
        other.gpuInitialized = false;
    }

    CUDANBodySimulator &operator=(CUDANBodySimulator &&other) noexcept
    {
        if (this != &other)
        {
            cleanupGPU();

            particles = std::move(other.particles);
            timeStep = other.timeStep;
            G = other.G;
            softening = other.softening;
            devParticles = other.devParticles;
            gpuInitialized = other.gpuInitialized;
            enableProfiling = other.enableProfiling;
            copyToGPUTime = other.copyToGPUTime;
            computeTime = other.computeTime;
            copyFromGPUTime = other.copyFromGPUTime;
            totalTime = other.totalTime;

            other.devParticles = nullptr;
            other.gpuInitialized = false;
        }
        return *this;
    }

    void update()
{
    if (!particles || particles->size() == 0 || !gpuInitialized)
    {
        return;
    }

    startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = startTime;

    copyParticlesToGPU(devParticles, particles->data(), particles->size());

    auto afterCopyToGPU = std::chrono::high_resolution_clock::now();
    copyToGPUTime = std::chrono::duration<float, std::milli>(afterCopyToGPU - currentTime).count();
    currentTime = afterCopyToGPU;

 
    computeGravitationalForcesGPU(devParticles, particles->size(), G, softening);
    integrateParticlesGPU(devParticles, particles->size(), timeStep);

    auto afterCompute = std::chrono::high_resolution_clock::now();
    computeTime = std::chrono::duration<float, std::milli>(afterCompute - currentTime).count();
    currentTime = afterCompute;

    copyParticlesFromGPU(particles->data(), devParticles, particles->size());

    auto endTime = std::chrono::high_resolution_clock::now();
    copyFromGPUTime = std::chrono::duration<float, std::milli>(endTime - currentTime).count();
    totalTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    if (enableProfiling)
    {
        std::cout << "CUDA Profiling [" << particles->size() << " particles]:"
                << " Total: " << totalTime << "ms,"
                << " Copy to GPU: " << copyToGPUTime << "ms,"
                << " Compute: " << computeTime << "ms,"
                << " Copy from GPU: " << copyFromGPUTime << "ms"
                << std::endl;
    }
}

    void enableProfilingOutput(bool enable)
    {
        enableProfiling = enable;
    }

    void resetSimulation(ParticleSystem &newParticles)
    {
        particles = std::make_shared<ParticleSystem>(newParticles);

        if (particles->size() != newParticles.size())
        {
            cleanupGPU();
            initializeGPU();
        }
    }

private:
    void initializeGPU()
    {
        if (!particles)
            return;

        try
        {
            // Allocate mem on gpu
            allocateParticlesGPU(&devParticles, particles->size());
            gpuInitialized = true;

            // Initial copy of particles to gpu
            copyParticlesToGPU(devParticles, particles->data(), particles->size());
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error initializing CUDA: " << e.what() << std::endl;
            cleanupGPU();
        }
    }

    void cleanupGPU()
    {
        if (gpuInitialized && devParticles)
        {
            try
            {
                freeParticlesGPU(devParticles);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error cleaning up CUDA resources: " << e.what() << std::endl;
            }
            devParticles = nullptr;
            gpuInitialized = false;
        }
    }
};

#endif // CUDA_SIMULATOR_H
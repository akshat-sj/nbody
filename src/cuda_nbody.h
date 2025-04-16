#ifndef CUDA_NBODY_H
#define CUDA_NBODY_H

#include <glm/glm.hpp>
#include "particle.h"
#include "physics.h"

// Include CUDA headers only when compiling with NVCC
#ifdef __CUDACC__
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#endif

// error check macro
#ifdef __CUDACC__
#define CUDA_CHECK_ERROR(call)                                                                           \
    do                                                                                                   \
    {                                                                                                    \
        cudaError_t error = call;                                                                        \
        if (error != cudaSuccess)                                                                        \
        {                                                                                                \
            fprintf(stderr, "CUDA error: %s at %s:%d\n", cudaGetErrorString(error), __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                                                          \
        }                                                                                                \
    } while (0)
#else
#define CUDA_CHECK_ERROR(call) call
#endif

// make a particle struct for cuda only (use compatible types for C++)
struct ParticleGPU
{
#ifdef __CUDACC__
    float4 position;
    float4 velocity;
    float4 acceleration;
#else
    // Use glm::vec4 for C++ code
    glm::vec4 position;
    glm::vec4 velocity;
    glm::vec4 acceleration;
#endif
    float mass;
};

// Forward declarations of CUDA functions - couldnt really do the cmake stuff without it
#ifdef __CUDACC__
__host__
#endif
    ParticleGPU
    particleToGPU(const Particle &p);

#ifdef __CUDACC__
__host__
#endif
    void
    gpuToParticle(const ParticleGPU &gpuP, Particle &p);

void allocateParticlesGPU(ParticleGPU **devParticles, int numParticles);
void freeParticlesGPU(ParticleGPU *devParticles);
void copyParticlesToGPU(ParticleGPU *devParticles, const Particle *hostParticles, int numParticles);
void copyParticlesFromGPU(Particle *hostParticles, const ParticleGPU *devParticles, int numParticles);
void computeGravitationalForcesGPU(ParticleGPU *devParticles, int numParticles, float G, float softening);
void integrateParticlesGPU(ParticleGPU *devParticles, int numParticles, float dt);

#endif // CUDA_NBODY_H
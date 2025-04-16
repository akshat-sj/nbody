#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include "cuda_nbody.h"
#include <stdio.h>

// functions to move info from cpu to gpu and vice versa
__host__ ParticleGPU particleToGPU(const Particle &p)
{
    ParticleGPU gpuP;
    gpuP.position = make_float4(p.position.x, p.position.y, p.position.z, p.position.w);
    gpuP.velocity = make_float4(p.velocity.x, p.velocity.y, p.velocity.z, p.velocity.w);
    gpuP.acceleration = make_float4(p.acceleration.x, p.acceleration.y, p.acceleration.z, p.acceleration.w);
    gpuP.mass = p.mass;
    return gpuP;
}

__host__ void gpuToParticle(const ParticleGPU &gpuP, Particle &p)
{
    p.position = glm::vec4(gpuP.position.x, gpuP.position.y, gpuP.position.z, gpuP.position.w);
    p.velocity = glm::vec4(gpuP.velocity.x, gpuP.velocity.y, gpuP.velocity.z, gpuP.velocity.w);
    p.acceleration = glm::vec4(gpuP.acceleration.x, gpuP.acceleration.y, gpuP.acceleration.z, gpuP.acceleration.w);
    p.mass = gpuP.mass;
}

// same stuff as physics file but using kernels

__global__ void resetAccelerationKernel(ParticleGPU *particles, int numParticles)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < numParticles)
    {
        particles[idx].acceleration = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

__global__ void computeForcesKernel(ParticleGPU *particles, int numParticles, float G, float softening)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < numParticles)
    {
        float4 myPos = particles[idx].position;
        float myMass = particles[idx].mass;
        float4 acc = make_float4(0.0f, 0.0f, 0.0f, 0.0f);

        if (idx == 0)
        {
            particles[idx].acceleration = make_float4(0.0f, 0.0f, 0.0f, 0.0f);
            return;
        }

        // black holes the first particle
        float4 bhPos = particles[0].position;
        float bhMass = particles[0].mass;

        float3 direction;
        direction.x = bhPos.x - myPos.x;
        direction.y = bhPos.y - myPos.y;
        direction.z = bhPos.z - myPos.z;

        float distSquared = direction.x * direction.x +
                            direction.y * direction.y +
                            direction.z * direction.z +
                            softening;

        float dist = sqrtf(distSquared);
        float invDist = 1.0f / dist;

        direction.x *= invDist;
        direction.y *= invDist;
        direction.z *= invDist;

        float forceMag = G * myMass * bhMass / distSquared;

        acc.x += direction.x * forceMag / myMass;
        acc.y += direction.y * forceMag / myMass;
        acc.z += direction.z * forceMag / myMass;

        // Calculate forces from all other particles
        for (int j = 1; j < numParticles; j++)
        {
            if (j == idx)
                continue;

            float4 otherPos = particles[j].position;
            float otherMass = particles[j].mass;

            float3 dirOther;
            dirOther.x = otherPos.x - myPos.x;
            dirOther.y = otherPos.y - myPos.y;
            dirOther.z = otherPos.z - myPos.z;

            float distSqOther = dirOther.x * dirOther.x +
                                dirOther.y * dirOther.y +
                                dirOther.z * dirOther.z +
                                softening;

            float distOther = sqrtf(distSqOther);
            float invDistOther = 1.0f / distOther;

            dirOther.x *= invDistOther;
            dirOther.y *= invDistOther;
            dirOther.z *= invDistOther;

            float forceMagOther = G * myMass * otherMass / distSqOther;

            acc.x += dirOther.x * forceMagOther / myMass;
            acc.y += dirOther.y * forceMagOther / myMass;
            acc.z += dirOther.z * forceMagOther / myMass;
        }

        // left remaining implementation same
        float maxAcc = 1000.0f;
        float accMag = sqrtf(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);

        if (accMag > maxAcc)
        {
            float scale = maxAcc / accMag;
            acc.x *= scale;
            acc.y *= scale;
            acc.z *= scale;
        }

        particles[idx].acceleration = acc;
    }
}

// CUDA part 1 for integrate leap frog
__global__ void integrateLeapFrogKernel1(ParticleGPU *particles, int numParticles, float dt)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    // implementation practically is the same

    if (idx < numParticles)
    {
        if (idx == 0)
            return;

        float4 vel = particles[idx].velocity;
        float4 acc = particles[idx].acceleration;

        vel.x += acc.x * dt * 0.5f;
        vel.y += acc.y * dt * 0.5f;
        vel.z += acc.z * dt * 0.5f;

        particles[idx].position.x += vel.x * dt;
        particles[idx].position.y += vel.y * dt;
        particles[idx].position.z += vel.z * dt;

        particles[idx].velocity = vel;
    }
}

// CUDA part 2 for leap-frog integration
__global__ void integrateLeapFrogKernel2(ParticleGPU *particles, int numParticles, float dt)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < numParticles)
    {
        // pretty much the same as before
        if (idx == 0)
            return;

        float4 vel = particles[idx].velocity;
        float4 acc = particles[idx].acceleration;

        vel.x += acc.x * dt * 0.5f;
        vel.y += acc.y * dt * 0.5f;
        vel.z += acc.z * dt * 0.5f;

        float3 pos;
        pos.x = particles[idx].position.x;
        pos.y = particles[idx].position.y;
        pos.z = particles[idx].position.z;

        float distFromCenter = sqrtf(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
        if (distFromCenter > 30.0f)
        {
            vel.x *= 0.998f;
            vel.y *= 0.998f;
            vel.z *= 0.998f;
        }

        particles[idx].velocity = vel;
    }
}

void allocateParticlesGPU(ParticleGPU **devParticles, int numParticles)
{
    CUDA_CHECK_ERROR(cudaMalloc((void **)devParticles, numParticles * sizeof(ParticleGPU)));
}

void freeParticlesGPU(ParticleGPU *devParticles)
{
    if (devParticles)
    {
        CUDA_CHECK_ERROR(cudaFree(devParticles));
    }
}

void copyParticlesToGPU(ParticleGPU *devParticles, const Particle *hostParticles, int numParticles)
{
    // temp particles to fill in real info
    ParticleGPU *tempGPUParticles = new ParticleGPU[numParticles];

    for (int i = 0; i < numParticles; i++)
    {
        tempGPUParticles[i] = particleToGPU(hostParticles[i]);
    }

    // Copy to device
    CUDA_CHECK_ERROR(cudaMemcpy(devParticles, tempGPUParticles,
                                numParticles * sizeof(ParticleGPU),
                                cudaMemcpyHostToDevice));

    delete[] tempGPUParticles;
}

void copyParticlesFromGPU(Particle *hostParticles, const ParticleGPU *devParticles, int numParticles)
{
    // temp particles
    ParticleGPU *tempGPUParticles = new ParticleGPU[numParticles];

    // Copy from device
    CUDA_CHECK_ERROR(cudaMemcpy(tempGPUParticles, devParticles,
                                numParticles * sizeof(ParticleGPU),
                                cudaMemcpyDeviceToHost));

    for (int i = 0; i < numParticles; i++)
    {
        gpuToParticle(tempGPUParticles[i], hostParticles[i]);
    }

    delete[] tempGPUParticles;
}

void computeGravitationalForcesGPU(ParticleGPU *devParticles, int numParticles, float G, float softening)
{
    int blockSize = 512;
    int numBlocks = (numParticles + blockSize - 1) / blockSize;

    resetAccelerationKernel<<<numBlocks, blockSize>>>(devParticles, numParticles);

    computeForcesKernel<<<numBlocks, blockSize>>>(devParticles, numParticles, G, softening);

    // Check for kernel errors
    CUDA_CHECK_ERROR(cudaGetLastError());
    CUDA_CHECK_ERROR(cudaDeviceSynchronize());
}

void integrateParticlesGPU(ParticleGPU *devParticles, int numParticles, float dt)
{
    // Set grid and block dimensions
    int blockSize = 256;
    int numBlocks = (numParticles + blockSize - 1) / blockSize;

    // First half of leap-frog integration
    integrateLeapFrogKernel1<<<numBlocks, blockSize>>>(devParticles, numParticles, dt);

    // Compute forces (this will be called separately)

    // Second half of leap-frog integration
    integrateLeapFrogKernel2<<<numBlocks, blockSize>>>(devParticles, numParticles, dt);

    // Check for kernel errors
    CUDA_CHECK_ERROR(cudaGetLastError());
    CUDA_CHECK_ERROR(cudaDeviceSynchronize());
}

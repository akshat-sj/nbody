#include "octree.h"
#include "particle.h"
#include "physics.h"
#include <memory>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <vector>

class BarnesHutCPUSimulator
{
private:
    std::shared_ptr<ParticleSystem> particles;
    float timeStep;
    float theta;
    Octree octree;
    float G;
    float softening;
    
    bool enableProfiling = false;
    int rebuildFrequency = 1;
    int frameCounter = 0;

public:
    BarnesHutCPUSimulator(ParticleSystem& particleSystem, float dt, float theta = 0.5f, 
                         float G = Physics::G, float softening = Physics::SOFTENING)
        : particles(std::make_shared<ParticleSystem>(particleSystem)),
          timeStep(dt), theta(theta), octree(theta), 
          G(G), softening(softening) {}

    BarnesHutCPUSimulator(BarnesHutCPUSimulator&&) = default;
    BarnesHutCPUSimulator& operator=(BarnesHutCPUSimulator&&) = default;

    void update()
    {
        if (!particles || particles->size() == 0) return;
        
        size_t n = particles->size();
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < n; i++) {
            Physics::integrateLeapFrog((*particles)[i], timeStep);
        }
        
        auto afterIntegrate1 = std::chrono::high_resolution_clock::now();
        
        bool rebuildTree = (frameCounter % rebuildFrequency == 0);
        if (rebuildTree) {
            try {
                octree.buildTree(*particles);
            } catch (const std::exception& e) {
                std::cerr << "Error building octree: " << e.what() << std::endl;
                calculateForcesDirectly();
                return;
            }
        }
        
        auto afterTreeBuild = std::chrono::high_resolution_clock::now();
        
        try {
            calculateForcesSafely();
        } catch (const std::exception& e) {
            std::cerr << "Error calculating forces: " << e.what() << std::endl;
            calculateForcesDirectly();
        }
        
        auto afterForces = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < n; i++) {
            Physics::finalizeLeapFrog((*particles)[i], timeStep);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        
        if (enableProfiling) {
            float integrateTime1 = std::chrono::duration<float, std::milli>(afterIntegrate1 - startTime).count();
            float treeBuildTime = std::chrono::duration<float, std::milli>(afterTreeBuild - afterIntegrate1).count();
            float forcesTime = std::chrono::duration<float, std::milli>(afterForces - afterTreeBuild).count();
            float integrateTime2 = std::chrono::duration<float, std::milli>(endTime - afterForces).count();
            float totalTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
            
            std::cout << "BH Profiling [" << n << " particles]:" 
                      << " Total: " << totalTime << "ms,"
                      << " Tree: " << treeBuildTime << "ms," 
                      << " Forces: " << forcesTime << "ms," 
                      << " Integrate: " << (integrateTime1 + integrateTime2) << "ms" 
                      << std::endl;
        }
        
        frameCounter++;
    }
    
    void setRebuildFrequency(int freq) {
        rebuildFrequency = std::max(1, freq);
    }
    
    void enableProfilingOutput(bool enable) {
        enableProfiling = enable;
    }
    
    void setAdaptiveTheta(bool enable) {
        if (enable) {
            size_t n = particles->size();
            theta = std::min(0.8f, std::max(0.3f, 0.4f + static_cast<float>(n) / 50000.0f));
            octree.setTheta(theta);
        }
    }

private:
    void calculateForcesSafely() {
        if (!particles) return;
        
        size_t n = particles->size();
        
        for (size_t i = 0; i < n; i++) {
            (*particles)[i].acceleration = glm::vec4(0.0f);
        }

        for (size_t i = 0; i < n; i++) {
            if ((*particles)[i].mass <= 0.0f) continue;
            
            float adaptiveSoftening = softening;
            if ((*particles)[i].mass > 10.0f) {
                adaptiveSoftening = softening * 1.5f;
            }
            
            glm::vec3 force(0.0f);
            
            try {
                force = octree.calculateForce((*particles)[i], G, adaptiveSoftening);
            } catch (const std::exception& e) {
                std::cerr << "Error in octree force calc, using direct for particle " << i << std::endl;
                force = calculateDirectForce(i);
                continue;
            }
            
            if (i > 0 && n > 1 && (*particles)[0].mass > 100.0f) {
                glm::vec3 pos1((*particles)[i].position);
                glm::vec3 pos2((*particles)[0].position);
                
                glm::vec3 direction = pos2 - pos1;
                float distSquared = glm::dot(direction, direction) + adaptiveSoftening;
                
                if (distSquared > 0.0001f) {
                    float dist = sqrt(distSquared);
                    direction /= dist;
                    
                    float forceMag = G * (*particles)[i].mass * (*particles)[0].mass / distSquared;
                    force += direction * forceMag;
                }
            }
            
            glm::vec3 acceleration = force / std::max(0.001f, (*particles)[i].mass);
            
            float maxAcc = 1000.0f; 
            float accMag = glm::length(acceleration);
            if (accMag > maxAcc) {
                acceleration = acceleration * (maxAcc / accMag);
            }
            
            glm::vec3 pos((*particles)[i].position);
            float distFromCenter = glm::length(pos);
            if (distFromCenter > 30.0f) {
                glm::vec3 vel((*particles)[i].velocity);
                vel *= 0.998f;
                (*particles)[i].velocity = glm::vec4(vel, 0.0f);
            }
            
            (*particles)[i].acceleration = glm::vec4(acceleration, 0.0f);
        }
    }
    
    void calculateForcesDirectly() {
        if (!particles) return;
        
        size_t n = particles->size();
        
        for (size_t i = 0; i < n; i++) {
            (*particles)[i].acceleration = glm::vec4(0.0f);
        }
        
        for (size_t i = 0; i < n; i++) {
            glm::vec3 force(0.0f);
            
            force = calculateDirectForce(i);
            
            glm::vec3 acceleration = force / std::max(0.001f, (*particles)[i].mass);
            
            float maxAcc = 1000.0f;
            float accMag = glm::length(acceleration);
            if (accMag > maxAcc) {
                acceleration = acceleration * (maxAcc / accMag);
            }
            
            (*particles)[i].acceleration = glm::vec4(acceleration, 0.0f);
        }
        
        if (enableProfiling) {
            std::cerr << "Using direct force calculation as fallback for " << n << " particles" << std::endl;
        }
    }
    
    glm::vec3 calculateDirectForce(size_t index) {
        if (!particles || index >= particles->size()) return glm::vec3(0.0f);
        
        glm::vec3 force(0.0f);
        size_t n = particles->size();
        
        for (size_t j = 0; j < n; j++) {
            if (j == index) continue;
            
            glm::vec3 pos1((*particles)[index].position);
            glm::vec3 pos2((*particles)[j].position);
            
            glm::vec3 direction = pos2 - pos1;
            float distSquared = glm::dot(direction, direction) + softening;
            
            if (distSquared > 0.0001f) {
                float dist = sqrt(distSquared);
                direction /= dist;
                
                float forceMag = G * (*particles)[index].mass * (*particles)[j].mass / distSquared;
                force += direction * forceMag;
            }
        }
        
        return force;
    }
};
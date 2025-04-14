#ifndef SEQNBODY_H
#define SEQNBODY_H

#include "particle.h"
#include "physics.h"

class SequentialNBodySimulator {
private:
    ParticleSystem* particleSystem; 
    float dt;
    float blackHoleMass; 

public:
    SequentialNBodySimulator() : particleSystem(nullptr), dt(0.0f), blackHoleMass(1000.0f) {}
    
    SequentialNBodySimulator(ParticleSystem& ps, float timeStep, float bhMass = 1000.0f) 
        : particleSystem(&ps), dt(timeStep), blackHoleMass(bhMass) {}

    void update() {
        if (!particleSystem) return; 
        
        for (size_t i = 0; i < particleSystem->size(); i++) {
            Physics::integrateLeapFrog((*particleSystem)[i], dt);
        }

        for (size_t i = 0; i < particleSystem->size(); i++) {
            (*particleSystem)[i].acceleration = glm::vec4(0.0f);
        }

        for (size_t i = 1; i < particleSystem->size(); i++) {
            glm::vec3 bhForce = Physics::calculateBlackHoleForce((*particleSystem)[i], (*particleSystem)[0].mass);
            glm::vec3 acc = bhForce / (*particleSystem)[i].mass;
            
            for (size_t j = 1; j < particleSystem->size(); j++) {
                if (i != j) {
                    glm::vec3 force = Physics::calculateForce((*particleSystem)[i], (*particleSystem)[j]);
                    acc += force / (*particleSystem)[i].mass;
                }
            }
            
            (*particleSystem)[i].acceleration = glm::vec4(acc, 0.0f);
        }

        for (size_t i = 1; i < particleSystem->size(); i++) {
            Physics::finalizeLeapFrog((*particleSystem)[i], dt);
        }
    }
};

#endif // SEQNBODY_H
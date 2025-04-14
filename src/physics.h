#ifndef PHYSICS_H
#define PHYSICS_H

#include "particle.h"
#include <glm/glm.hpp>
#include <cmath>

namespace Physics
{
    constexpr float G = 1.0f;
    constexpr float SOFTENING = 0.1f;

    inline glm::vec3 calculateForce(const Particle &p1, const Particle &p2)
    {
        glm::vec3 pos1(p1.position);
        glm::vec3 pos2(p2.position);
        
        glm::vec3 direction = pos2 - pos1;
        
        float distSquared = glm::dot(direction, direction) + SOFTENING;
        
        float dist = sqrt(distSquared);
        if (dist > 0.0001f) {
            direction /= dist;
        }
        
        float forceMagnitude = G * p1.mass * p2.mass / distSquared;
        
        return direction * forceMagnitude;
    }

    inline glm::vec3 calculateBlackHoleForce(const Particle &p, float blackHoleMass = 1000.0f)
    {
        glm::vec3 pos(p.position);
        glm::vec3 center(0.0f); 
        
        glm::vec3 direction = center - pos;
        
        float distSquared = glm::dot(direction, direction) + SOFTENING;
        
        float dist = sqrt(distSquared);
        if (dist > 0.0001f) {
            direction /= dist;
        }
        
        float forceMagnitude = G * blackHoleMass * p.mass / distSquared;
        
        return direction * forceMagnitude;
    }

    inline void integrateLeapFrog(Particle &p, float dt)
    {
        glm::vec3 velocity(p.velocity);
        glm::vec3 acceleration(p.acceleration);

        velocity += acceleration * dt * 0.5f;
        
        glm::vec3 position(p.position);
        position += velocity * dt;
        p.position = glm::vec4(position, 0.0f);
        
        p.velocity = glm::vec4(velocity, 0.0f);
    }

    inline void finalizeLeapFrog(Particle &p, float dt)
    {
        glm::vec3 velocity(p.velocity);
        glm::vec3 acceleration(p.acceleration);
        
        velocity += acceleration * dt * 0.5f;
        p.velocity = glm::vec4(velocity, 0.0f);
    }
}

#endif // PHYSICS_H
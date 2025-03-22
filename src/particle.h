#ifndef PARTICLE_H
#define PARTICLE_h
#include <glm/glm.hpp>

struct Particle
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 accelearion;
    float mass;
};


#endif
#ifndef PARTICLE_H
#define PARTICLE_H 
#include "library.h"

struct Particle
{
    glm::vec4 position;
    glm::vec4 velocity;
    glm::vec4 acceleration;
    float mass;

    Particle(glm::vec3 pos = glm::vec3(0.0f),
             glm::vec3 vel = glm::vec3(0.0f),
             glm::vec3 acc = glm::vec3(0.0f),
             float m = 1.0f)
        : position(glm::vec4(pos, 0.0f)),
          velocity(glm::vec4(vel, 0.0f)),
          acceleration(glm::vec4(acc, 0.0f)),
          mass(m) {}

};

class ParticleSystem
{
private:
    Particle *particles;
    size_t numParticles;
    bool ownmemory;

public:
    ParticleSystem(Particle *existingParticles, size_t n)
        : particles(existingParticles), numParticles(n), ownmemory(false) {}

    ~ParticleSystem()
    {
        if (ownmemory && particles)
        {
            delete[] particles;
        }
    }
    size_t size() const { return numParticles; }

    Particle &operator[](size_t i) { return particles[i]; }
    const Particle &operator[](size_t i) const { return particles[i]; }
    Particle *data() { return particles; }
    const Particle *data() const { return particles; }

};


#endif
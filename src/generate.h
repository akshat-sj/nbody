#ifndef GENERATE_H
#define GENERATE_H
#include "particle.h"
#include "library.h"
#include "physics.h"


glm::vec3 randomSphere(float radius) {
    // use rejection sampling to get points in a unit cube into a unit sphere to get points uniformly
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    glm::vec3 point;
    float length;
    
    do {
        point.x = dist(gen);
        point.y = dist(gen);
        point.z = dist(gen);
        length = glm::length(point);
    } while (length > 1.0f);
    
    return point * radius;
}

void generateDiskGalaxy(Particle* particles, int count)
{
    const float galaxy_diameter = 20.0f;
    const float galaxy_thickness = 1.0f;
    const float stars_speed = 5.0f;
    const float black_hole_mass = 1000.0f;
    // placing central black hole 
    particles[0] = Particle(
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        black_hole_mass
    );
    
    for (int i = 1; i < count; i++) {
        // places stars in disk by putting more stars closer to center like real galaxy
        glm::vec3 pos = randomSphere(galaxy_diameter / 2.0f);
        float radius = glm::length(glm::vec2(pos.x, pos.z));
        float scaledRadius = pow(radius / (galaxy_diameter / 2.0f), 5.0f) * (galaxy_diameter / 2.0f);
        
        if (radius > 0.0f) {
            float scale = scaledRadius / radius;
            pos.x *= scale;
            pos.z *= scale;
        }
        
        // flattening the galaxy 
        pos.y *= galaxy_thickness / galaxy_diameter;
        // calculating force direction and velocity direction
        glm::vec3 direction = glm::normalize(glm::cross(glm::vec3(pos.x, 0.0f, pos.z), glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 vel = direction * stars_speed;
        particles[i] = Particle(glm::vec4(pos, 0.0f), glm::vec4(vel, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), 1.0f);
    }
}

void generateSpiralGalaxy(Particle* particles, int count)
{
    const float galaxy_diameter = 20.0f;
    const float galaxy_thickness = 1.0f;
    const float stars_speed = 5.0f;
    const float black_hole_mass = 1000.0f;
    
    // placing central black hole 
    particles[0] = Particle(
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        black_hole_mass
    );
    
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDist(0.1f, galaxy_diameter / 2.0f);
    std::uniform_real_distribution<float> armPhase(0.0f, 2.0f * 3.14159f);
    std::normal_distribution<float> heightDist(0.0f, 0.2f);
    
    const int arms = 2; 
    const float arm_tightness = 0.5f;
    
    for (int i = 1; i < count; i++) {
        float baseRadius = radiusDist(gen);
        float arm = static_cast<int>(gen() % arms);
        float armOffset = arm * (2.0f * 3.14159f / arms);
        float angle = armOffset + (arm_tightness * baseRadius);
        
        angle += angleDist(gen) * 0.2f;
        
        float height = heightDist(gen) * (0.1f + baseRadius * 0.03f);
        glm::vec3 pos(
            baseRadius * cos(angle),
            baseRadius * sin(angle),
            height * galaxy_thickness
        );
        
        glm::vec3 direction = glm::normalize(glm::cross(glm::vec3(pos.x, pos.y, 0.0f),glm::vec3(0.0f, 0.0f, 1.0f)     ));
        float speed = stars_speed;
        glm::vec3 vel = direction * speed;
        particles[i] = Particle(glm::vec4(pos, 0.0f),glm::vec4(vel, 0.0f),glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),1.0f);
    }
}

void generateCollisionGalaxy(Particle* particles, int count)
{
    const float galaxy_separation = 15.0f;
    const float galaxy_diameter = 15.0f;
    const float galaxy_thickness = 1.0f;
    const float stars_speed = 3.0f;
    const float collision_speed = 1.0f;
    const float black_hole_mass = 800.0f;
    
    int half = count / 2;
    
    particles[0] = Particle(
        glm::vec4(-galaxy_separation/2.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(collision_speed, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        black_hole_mass
    );
    
    particles[half] = Particle(
        glm::vec4(galaxy_separation/2.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(-collision_speed, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        black_hole_mass
    );
    
    for (int i = 1; i < half; i++) {
        glm::vec3 pos = randomSphere(galaxy_diameter / 2.0f);
        
        float radius = glm::length(glm::vec2(pos.x, pos.z));
        float scaledRadius = pow(radius / (galaxy_diameter / 2.0f), 5.0f) * (galaxy_diameter / 2.0f);
        
        if (radius > 0.0f) {
            float scale = scaledRadius / radius;
            pos.x *= scale;
            pos.z *= scale;
        }
        
        pos.y *= galaxy_thickness / galaxy_diameter;
        
        pos.x -= galaxy_separation / 2.0f;
        
        glm::vec3 radiusVec = pos - glm::vec3(-galaxy_separation/2.0f, 0.0f, 0.0f);
        glm::vec3 direction = glm::normalize(glm::cross(glm::vec3(radiusVec.x, 0.0f, radiusVec.z), glm::vec3(0.0f, 1.0f, 0.0f)));
        
        glm::vec3 vel = direction * stars_speed + glm::vec3(collision_speed, 0.0f, 0.0f);
        
        particles[i] = Particle(
            glm::vec4(pos, 0.0f),
            glm::vec4(vel, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            1.0f
        );
    }
    
    for (int i = half + 1; i < count; i++) {
        glm::vec3 pos = randomSphere(galaxy_diameter / 2.0f);
        
        float radius = glm::length(glm::vec2(pos.x, pos.z));
        float scaledRadius = pow(radius / (galaxy_diameter / 2.0f), 5.0f) * (galaxy_diameter / 2.0f);
        
        if (radius > 0.0f) {
            float scale = scaledRadius / radius;
            pos.x *= scale;
            pos.z *= scale;
        }
        
        pos.y *= galaxy_thickness / galaxy_diameter;
        
        std::swap(pos.y, pos.z);
        
        pos.x += galaxy_separation / 2.0f;
        
        glm::vec3 radiusVec = pos - glm::vec3(galaxy_separation/2.0f, 0.0f, 0.0f);
        glm::vec3 direction = glm::normalize(glm::cross(glm::vec3(radiusVec.x, 0.0f, radiusVec.z), glm::vec3(0.0f, 1.0f, 0.0f)));
        
        std::swap(direction.y, direction.z);
        
        glm::vec3 vel = direction * stars_speed + glm::vec3(-collision_speed, 0.0f, 0.0f);
        
        particles[i] = Particle(
            glm::vec4(pos, 0.0f),
            glm::vec4(vel, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            1.0f
        );
    }
}

void generateRandomGalaxy(Particle* particles, int count)
{
    const float maxDistance = 20.0f;
    const float black_hole_mass = 1000.0f;
    
    particles[0] = Particle(
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        black_hole_mass
    );
    
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> posDist(-maxDistance, maxDistance);
    std::uniform_real_distribution<float> velDist(-1.0f, 1.0f);
    
    for (int i = 1; i < count; i++) {
        glm::vec3 pos(posDist(gen), posDist(gen), posDist(gen));
        glm::vec3 vel(velDist(gen), velDist(gen), velDist(gen));
        
        vel *= (1.0f - glm::length(pos) / maxDistance) * 2.0f;
        
        particles[i] = Particle(
            glm::vec4(pos, 0.0f),
            glm::vec4(vel, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            1.0f
        );
    }
}

void generateDenseDiskGalaxy(Particle* particles, int count)
{
    const float galaxy_diameter = 15.0f;
    const float galaxy_thickness = 0.5f;
    const float stars_speed = 6.0f;
    const float black_hole_mass = 1500.0f;
    
    particles[0] = Particle(
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
        black_hole_mass
    );
    
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> radiusDist(0.1f, galaxy_diameter / 2.0f);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::normal_distribution<float> heightDist(0.0f, 0.1f);
    
    for (int i = 1; i < count; i++) {
        float r = radiusDist(gen);
        r = pow(r, 1.5f) * pow(galaxy_diameter / 2.0f, -0.5f);
        
        float angle = angleDist(gen);
        float height = heightDist(gen) * galaxy_thickness;
        
        glm::vec3 pos(
            r * cos(angle),
            height,
            r * sin(angle)
        );
        
        float dist = glm::length(glm::vec2(pos.x, pos.z));
        if (dist < 0.1f) dist = 0.1f; 
        
        float speed = stars_speed * sqrt(black_hole_mass / (dist * 100.0f));
        
        glm::vec3 vel(
            -pos.z / dist * speed,
            0.0f,
            pos.x / dist * speed
        );
        
        particles[i] = Particle(
            glm::vec4(pos, 0.0f),
            glm::vec4(vel, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            1.0f
        );
    }
}
#endif
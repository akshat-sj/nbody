#ifndef OCTREE_H
#define OCTREE_H

#include "octree_node.h"
#include "particle.h"
#include <algorithm>
#include <limits>
#include <stack>
#include <exception>

class Octree
{
private:
    std::shared_ptr<OctreeNode> root;
    float theta;
    
    glm::vec3 cachedMinBound;
    glm::vec3 cachedMaxBound;
    bool boundsNeedUpdate;
    
    size_t maxTreeDepth;
    size_t nodeCount;

public:
    Octree(float theta = 0.5f) 
        : root(nullptr), theta(theta), boundsNeedUpdate(true), 
          maxTreeDepth(0), nodeCount(0) {}

    void setTheta(float newTheta) {
        theta = std::max(0.1f, std::min(1.0f, newTheta));
    }

    void buildTree(ParticleSystem &particles)
    {
        if (particles.size() == 0) {
            root = nullptr;
            return;
        }
        
        maxTreeDepth = 0;
        nodeCount = 0;
        
        root = nullptr;
        
        calculateBounds(particles);
        
        glm::vec3 center = (cachedMinBound + cachedMaxBound) * 0.5f;
        float halfWidth = std::max(std::max(
                              cachedMaxBound.x - center.x,
                              cachedMaxBound.y - center.y),
                          cachedMaxBound.z - center.z);
        
        root = std::make_shared<OctreeNode>(center, halfWidth);
        nodeCount++;
        
        const size_t MAX_TREE_DEPTH = 20;
        
        for (size_t i = 0; i < particles.size(); i++) {
            insertParticleSafely(&particles[i], root, 0, MAX_TREE_DEPTH);
        }
        
        calculateCenterOfMass(root);
    }
    
    glm::vec3 calculateForce(const Particle &particle, float G, float softening)
    {
        if (!root) return glm::vec3(0.0f);
        
        glm::vec3 force(0.0f);
        glm::vec3 particlePos(particle.position);
        
        std::stack<std::shared_ptr<OctreeNode>> nodeStack;
        nodeStack.push(root);
        
        while (!nodeStack.empty()) {
            std::shared_ptr<OctreeNode> node = nodeStack.top();
            nodeStack.pop();
            
            if (!node) continue;
            
            if (node->totalMass <= 0.0f) continue;
            
            if (node->isExternal() && node->particle == &particle) continue;
            
            glm::vec3 direction = node->centerOfMass - particlePos;
            float distSquared = glm::dot(direction, direction) + softening;
            
            if (node->isExternal() || 
                (node->halfWidth * node->halfWidth) / distSquared < theta * theta) {
                
                float distance = std::sqrt(distSquared);
                
                if (distance < 1e-5f) distance = 1e-5f;
                
                float forceMagnitude = G * particle.mass * node->totalMass / distSquared;
                force += direction * (forceMagnitude / distance);
            } 
            else {
                for (int i = 0; i < 8; i++) {
                    if (node->children[i]) {
                        nodeStack.push(node->children[i]);
                    }
                }
            }
        }
        
        return force;
    }

private:
    void calculateBounds(ParticleSystem &particles) {
        cachedMinBound = glm::vec3(std::numeric_limits<float>::max());
        cachedMaxBound = glm::vec3(std::numeric_limits<float>::lowest());

        for (size_t i = 0; i < particles.size(); i++) {
            const glm::vec3 pos(particles[i].position);
            cachedMaxBound = glm::max(cachedMaxBound, pos);
            cachedMinBound = glm::min(cachedMinBound, pos);
        }

        float padding = 0.1f * glm::length(cachedMaxBound - cachedMinBound);
        if (padding < 0.5f) padding = 0.5f; 
        
        cachedMaxBound += glm::vec3(padding);
        cachedMinBound -= glm::vec3(padding);
    }
    
    void insertParticleSafely(Particle *particle, std::shared_ptr<OctreeNode> node, 
                              size_t depth, size_t maxDepth) {
        
        if (!node || !particle || depth > maxDepth) {
            return;
        }
        
        maxTreeDepth = std::max(maxTreeDepth, depth);
        
        glm::vec3 pos(particle->position);
        if (pos.x < node->center.x - node->halfWidth || pos.x > node->center.x + node->halfWidth ||
            pos.y < node->center.y - node->halfWidth || pos.y > node->center.y + node->halfWidth ||
            pos.z < node->center.z - node->halfWidth || pos.z > node->center.z + node->halfWidth) {
            return;
        }
        
        if (!node->hasChildren() && !node->particle) {
            node->particle = particle;
            return;
        }
        
        if (node->isExternal()) {
            Particle *existingParticle = node->particle;
            
            node->particle = nullptr;
            
            int existingOctant = node->getOctantForPosition(glm::vec3(existingParticle->position));
            
            if (!node->children[existingOctant]) {
                glm::vec3 childCenter = node->getOctantCenter(existingOctant);
                node->children[existingOctant] = std::make_shared<OctreeNode>(
                    childCenter, node->halfWidth * 0.5f);
                nodeCount++;
            }
            
            insertParticleSafely(existingParticle, node->children[existingOctant], depth + 1, maxDepth);
        }
        
        int octant = node->getOctantForPosition(pos);
        
        if (!node->children[octant]) {
            glm::vec3 childCenter = node->getOctantCenter(octant);
            node->children[octant] = std::make_shared<OctreeNode>(
                childCenter, node->halfWidth * 0.5f);
            nodeCount++;
        }
        
        insertParticleSafely(particle, node->children[octant], depth + 1, maxDepth);
    }

    void calculateCenterOfMass(std::shared_ptr<OctreeNode> node) {
        if (!node) return;
        
        node->centerOfMass = glm::vec3(0.0f);
        node->totalMass = 0.0f;

        if (node->isExternal() && node->particle) {
            node->centerOfMass = glm::vec3(node->particle->position);
            node->totalMass = node->particle->mass;
            return;
        }
        
        for (int i = 0; i < 8; i++) {
            if (node->children[i]) {
                calculateCenterOfMass(node->children[i]);

                if (node->children[i]->totalMass > 0.0f) {
                    node->totalMass += node->children[i]->totalMass;
                    node->centerOfMass += node->children[i]->totalMass * node->children[i]->centerOfMass;
                }
            }
        }
        
        if (node->totalMass > 0.0f) {
            node->centerOfMass /= node->totalMass;
        }
    }
};

#endif // OCTREE_H
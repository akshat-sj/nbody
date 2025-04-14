#ifndef OCTREE_NODE_H
#define OCTREE_NODE_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "particle.h"

class OctreeNode {
public:
    glm::vec3 center;
    float halfWidth;

    glm::vec3 centerOfMass;
    float totalMass;

    Particle *particle;
    std::shared_ptr<OctreeNode> children[8];

    OctreeNode(const glm::vec3 &center, float halfWidth)
        : center(center), halfWidth(halfWidth), 
          centerOfMass(0.0f), totalMass(0.0f), particle(nullptr) {
        for (int i = 0; i < 8; i++) {
            children[i] = nullptr;
        }
    }

    bool isExternal() const {
        for (int i = 0; i < 8; i++) {
            if (children[i]) return false;
        }
        return true;
    }

    bool hasChildren() const {
        for (int i = 0; i < 8; i++) {
            if (children[i]) return true;
        }
        return false;
    }

    int getOctantForPosition(const glm::vec3 &position) const {
        int octant = 0;
        if (position.x >= center.x) octant |= 1;
        if (position.y >= center.y) octant |= 2;
        if (position.z >= center.z) octant |= 4;
        return octant;
    }

    glm::vec3 getOctantCenter(int octant) const {
        glm::vec3 offset(
            (octant & 1) ? halfWidth * 0.5f : -halfWidth * 0.5f,
            (octant & 2) ? halfWidth * 0.5f : -halfWidth * 0.5f,
            (octant & 4) ? halfWidth * 0.5f : -halfWidth * 0.5f
        );
        return center + offset;
    }
};

#endif // OCTREE_NODE_H
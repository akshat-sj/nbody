#ifndef CONSTLIB_H
#define CONSTLIB_H

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
const int MAX_PARTICLES = 10000;
int numParticles = 1000;
float simSpeed = 1.0f;
float physicsTimeStep = 0.01f;
bool pauseSimulation = false;
int simulationType = 1;
float theta = 0.5f;
int galaxyType = 0; 



#endif
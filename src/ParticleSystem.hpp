#pragma once

#include "Particle.hpp"
#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"
#include <glm/glm.hpp>

class ParticleSystem {
public:
	ParticleSystem();
	int findUnusedParticle();
	void generateParticles(int mode, int amount, glm::vec3 center, glm::vec3 initVel, float size, float spread, int life);
	void sortByCameraDistance();
	void killAll();
	
	int maxParticles;
	Particle particlesContainer[100000];
	int lastUsedParticle;
	int particlesCount;
};
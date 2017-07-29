#include "ParticleSystem.hpp"
#include "scene_lua.hpp"
using namespace std;

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"
#include "GeometryNode.hpp"
#include "JointNode.hpp"
#include "grid.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

ParticleSystem::ParticleSystem(): maxParticles(100000), lastUsedParticle(0), particlesCount(0) {
};

int ParticleSystem::findUnusedParticle() {
	for(int i=lastUsedParticle; i<maxParticles; i++){
		if (particlesContainer[i].life <= 0){
			lastUsedParticle = i;
			return i;
		}
	}
	for(int i=0; i<lastUsedParticle; i++){
		if (particlesContainer[i].life <= 0){
			lastUsedParticle = i;
			return i;
		}
	}
	return 0;
};

void ParticleSystem::generateParticles(int mode, int amount, vec3 center, vec3 initVel, float size, float spread, int life) {
	for (int i = 0; i < amount; i++) {
		int particle_i = findUnusedParticle();
		particlesContainer[particle_i].reset(mode, center, initVel, size, spread, life);
	}
}

void ParticleSystem::sortByCameraDistance() {
	sort(&particlesContainer[0], &particlesContainer[maxParticles]);
}

void ParticleSystem::killAll() {
	for (int i = 0; i < maxParticles; i++) {
		particlesContainer[i].life = -1.0f;
	}
	lastUsedParticle = 0;
}

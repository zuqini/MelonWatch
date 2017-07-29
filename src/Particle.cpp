#include "Particle.hpp"
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

Particle::Particle(): pos(0), speed(0), col(1.0f, 1.0f, 0.0f, 1.0f), life(-1), maxLife(-1), size(0.01), cameradistance(-1) {};

void Particle::reset(int mode, vec3 newPos, vec3 initVel, float newSize, float spread, int newLife) {
	pos = newPos;
	if (mode == 1) { // blood
		col = vec4(0.5f + rand() * 0.5f / RAND_MAX, 0.0f, 0.0f, 1.0f);
	} else if (mode == 2) { // snow
		col = vec4(0.8f + rand() * 0.2f / RAND_MAX, 0.8f + rand() * 0.2f / RAND_MAX, 0.8f + rand() * 0.2f / RAND_MAX, 1.0f);
	} else if (mode == 3) { // magic
		col = vec4(0.0f, 0.3f + rand() * 0.7f / RAND_MAX, 0.3f + rand() * 0.7f / RAND_MAX, 1.0f);
	} else { // else
		col = vec4(1.0f, 0.6f + rand() * 0.4f / RAND_MAX, 0.0f, 1.0f);
	}
	float radius = rand() * spread / RAND_MAX;
	float angle = rand() * PI / RAND_MAX;
	float angle2 = rand() * 2 * PI / RAND_MAX;
	speed = radius * (vec3(sin(angle) * cos(angle2), sin(angle) * sin(angle2), cos(angle)) + initVel);
	life = newLife;
	maxLife = newLife;
	cameradistance = -1;
	size = newSize;
};

bool operator<(const Particle &p1, const Particle &p2) {
	return p1.cameradistance > p2.cameradistance;
}
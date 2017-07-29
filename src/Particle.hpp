#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"
#include <glm/glm.hpp>

class Particle {
public:
	Particle();
	void reset(int mode, glm::vec3 newPos, glm::vec3 initVel, float newSize, float spread, int newLife);
	friend bool operator< (const Particle &p1, const Particle &p2);
	glm::vec3 pos, speed;
	glm::vec4 col;
	float size;
	float life;
	float maxLife;
	float cameradistance;
};
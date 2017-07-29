#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"
#include <glm/glm.hpp>

class Projectile {
public:
	Projectile();
	void reset(glm::vec3 pos, glm::vec3 speed);
	glm::vec3 pos, speed;
	glm::vec4 col;
	float size;
	float life;
};
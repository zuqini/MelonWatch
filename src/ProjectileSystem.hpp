#pragma once

#include "Projectile.hpp"
#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"
#include <glm/glm.hpp>

class ProjectileSystem {
public:
	ProjectileSystem();
	int findUnusedProjectile();
	void generateProjectile(glm::vec3 pos, glm::vec3 speed);
	void killAll();

	int maxProjectile;
	Projectile projectileContainer[30];
	int lastUsedProjectile;
};
#include "ProjectileSystem.hpp"
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

ProjectileSystem::ProjectileSystem(): maxProjectile(30), lastUsedProjectile(0) {
};

int ProjectileSystem::findUnusedProjectile() {
	for(int i=lastUsedProjectile; i<maxProjectile; i++){
		if (projectileContainer[i].life <= 0){
			lastUsedProjectile = i;
			return i;
		}
	}
	for(int i=0; i<lastUsedProjectile; i++){
		if (projectileContainer[i].life <= 0){
			lastUsedProjectile = i;
			return i;
		}
	}
	return 0;
};

void ProjectileSystem::generateProjectile(vec3 pos, vec3 speed) {
	int projectileIndex = findUnusedProjectile();
	projectileContainer[projectileIndex].reset(pos, speed);
}

void ProjectileSystem::killAll() {
	for (int i = 0; i < maxProjectile; i++) {
		projectileContainer[i].life = -1.0f;
	}
	lastUsedProjectile = 0;
}

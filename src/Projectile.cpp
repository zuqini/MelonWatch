#include "Projectile.hpp"
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

Projectile::Projectile():
	pos(0), speed(0), col(1.0f, 1.0f, 0.0f, 1.0f), size(1), life(-1) {};

void Projectile::reset(vec3 newPos, vec3 newSpeed) {
	pos = newPos;
	speed = newSpeed;
	col = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	size = 1;
	life = 150;
}

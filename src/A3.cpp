#include "A3.hpp"
#include "scene_lua.hpp"
using namespace std;

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"
#include "GeometryNode.hpp"
#include "JointNode.hpp"
#include "grid.hpp"
#include "Particle.hpp"
#include "Projectile.hpp"

#include <cstdlib>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <string.h>
#include <climits>
#include <queue>
#include <stack>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

typedef uint8_t  CHAR;
typedef uint16_t WORD;
typedef uint32_t DWORD;

typedef int8_t  BYTE;
typedef int16_t SHORT;
typedef int32_t LONG;

typedef LONG INT;
typedef INT BOOL;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace glm;

static bool show_gui = false;
static bool show_shadow_map = false;
static bool render_shadows = true;
static bool god_mode = false;
static bool render_textures = true;
static bool cursor_hidden = true;
static bool firstRun = true;

const unsigned int SHADOW_WIDTH = 1280, SHADOW_HEIGHT = 1280;
const glm::mat4 biasMatrix(
			0.5, 0.0, 0.0, 0.0,
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0
			);

static const size_t DIM_X = 255;
static const size_t DIM_Y = 255;
static const size_t DIM_Z = 255;

const float MAX_ACCEL_CAPACITY = 100.0f;
const float ACCEL_REPLENISH_RATE = 0.25f;
const float ACCEL_CONSUMPTION_RATE = 1.5f;

const int SNOW_PERIOD = 60;
const int FLAME_PERIOD = 10;
const size_t CIRCLE_PTS = 48;
const float AIR_RESISTANCE = 0.001f;
const float FRICTION = 0.005f;
const float MOVEMENT_ACCEL = 0.008 + FRICTION + AIR_RESISTANCE;
const float JUMP_FORCE = 0.2f;
const float ELEVATED_MOVEMENT_SPD = 0.24f;
const float NORMAL_MOVEMENT_SPD = 0.12f;
const float SENSITIVITY = 0.1f;
const float CAMERA_HEIGHT = 1.0f;
const float CROUCHED_HEIGHT = 0.35f;
const float CAMERA_RADIUS = 0.5f;
const int SHOOT_PERIOD = 5;
const vec3 GRAVITY(0.0f, -0.004f, 0.0f);
const float WALK_HOP = 0.04f;
const int MAX_JUMP_COUNT = 3;
const int MAX_LIFE = 12;

const float ZOOM_COEFFICIENT = 0.025f;
float currZoom = 0;

const float MAX_FOV = 60.0f;
const float MIN_FOV = 25.0f;

const vec3 OPT_1(14, 1, 15);
const vec3 OPT_2(14, 1, 17);
const vec3 OPT_3(14, 1, 19);

int ENEMY_FIRE_PERIOD = 180;
float ENEMY_FIRE_SPEED = 0.5f;

int numGrid = 16;
float gridLength = 1.0f / numGrid;

vec3 triangleNormals[] = {
	vec3(-1, 0, 0),
	vec3(0, -1, 0),
	vec3(0, 0, -1),
	vec3(1, 0, 0),
	vec3(0, 1, 0),
	vec3(0, 0, 1)
};

void planeIntersect(glm::vec3 origin, glm::vec3 destination, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2,
	float &b, float &a, float &t) {
	glm::vec3 R = origin - p0;
	glm::vec3 X(p1.x - p0.x, p2.x - p0.x, destination.x - origin.x);
	glm::vec3 Y(p1.y - p0.y, p2.y - p0.y, destination.y - origin.y);
	glm::vec3 Z(p1.z - p0.z, p2.z - p0.z, destination.z - origin.z);
	float D = glm::determinant(
		glm::mat3(X.x, Y.x, Z.x,
		X.y, Y.y, Z.y,
		-1.0 * X.z, -1.0 * Y.z, -1.0 * Z.z));
	float D1 = glm::determinant(
		glm::mat3(R.x, R.y, R.z,
		X.y, Y.y, Z.y,
		-1.0 * X.z, -1.0 * Y.z, -1.0 * Z.z));
	float D2 = glm::determinant(
		glm::mat3(X.x, Y.x, Z.x,
		R.x, R.y, R.z,
		-1.0 * X.z, -1.0 * Y.z, -1.0 * Z.z));
	float D3 = glm::determinant(
		glm::mat3(X.x, Y.x, Z.x,
		X.y, Y.y, Z.y,
		R.x, R.y, R.z));
	b = D1 / D;
	a = D2 / D;
	t = D3 / D;
}

bool sameSign(float a, float b) {
	return a*b >= 0.0f;
}

//----------------------------------------------------------------------------------------
// Constructor
A3::A3(const std::string & luaSceneFile)
	: m_luaSceneFile(luaSceneFile),
	  m_positionAttribLocation(0),
	  m_normalAttribLocation(0),
	  m_vao_meshData(0),
	  m_vbo_vertexPositions(0),
	  m_vbo_vertexNormals(0),
	  snowCounter(0),
	  m_vao_arcCircle(0),
	  m_vbo_arcCircle(0),
	  m_particle_vao(0),
	  m_particle_vbo(0),
	  currMapIndex(0),
	  depthMapFBO(0),
	  accelFov(0),
	  m_particles_pos_vbo(0),
	  m_particles_col_vbo(0),
	  jumpCount(MAX_JUMP_COUNT),
	  light_translate(0.0f),
	  l_near(-150.0f),
	  l_far(150.0f),
	  life(MAX_LIFE),
	  prevXPos(0),
	  prevYPos(0),
	  monstersKilled(0),
	  timesDied(0),
	  bulletsFired(0),
	  bulletsHit(0),
	  enemiesHit(0),
	  levelsPassed(0),
	  playerHeight(CAMERA_HEIGHT),
	  initMouse(true),
	  crouchPressed(false),
	  flameCounter(0),
	  gun_ani_counter(0),
	  enemyFireCounter(0),
	  walkSourceSwap(true),
	  recoil(0.0f),
	  zoomPressed(false),
	  inAirDisplacement(0),
	  accelPressed(false),
	  accelCapacity(MAX_ACCEL_CAPACITY),
	  inAirVel(0),
	  charVel(0.0f),
	  fov(MAX_FOV),
	  shootCounter(SHOOT_PERIOD)
{
	m_block_vao = new GLuint[ DIM_X * DIM_Y * DIM_Z ];
	m_block_vbo = new GLuint[ DIM_X * DIM_Y * DIM_Z ];
	m_block_uv_vbo = new GLuint[ DIM_X * DIM_Y * DIM_Z ];
	for (int i = 0; i < 4; i++) {
		wasdPressed[i] = false;
	}
	Grid *grid = new Grid(DIM_X, DIM_Y, DIM_Z);
	buildHomeBase(grid);
	maps.push_back(grid);
	currMap = new Grid(DIM_X, DIM_Y, DIM_Z);
}

//----------------------------------------------------------------------------------------
// Destructor
A3::~A3()
{
	delete [] m_block_vao;
	delete [] m_block_vbo;
	delete [] m_block_uv_vbo;
	while(!maps.empty()) {
        delete maps.back();
        maps.pop_back();
    }
    delete currMap;

    alDeleteSources(1, &gunSource);
	alDeleteBuffers(1, &shootBuffer);
	alDeleteSources(1, &expSource);
	alDeleteBuffers(1, &expBuffer);
	alDeleteSources(1, &dieSource);
	alDeleteBuffers(1, &dieBuffer);
	alDeleteSources(1, &bgmSource);
	alDeleteBuffers(1, &bgmBuffer);
	alDeleteSources(1, &homeBgmSource);
	alDeleteBuffers(1, &homeBgmBuffer);
	alDeleteSources(1, &flameSource);
	alDeleteBuffers(1, &flameBuffer);
	alDeleteSources(1, &enemySource);
	alDeleteBuffers(1, &enemyBuffer);
	alDeleteSources(1, &tpSource);
	alDeleteBuffers(1, &tpBuffer);
	alDeleteSources(1, &accelEngineSource);
	alDeleteBuffers(1, &accelEngineBuffer);

	alDeleteSources(1, &walk1Source);
	alDeleteBuffers(1, &walk1Buffer);
	alDeleteSources(1, &walk2Source);
	alDeleteBuffers(1, &walk2Buffer);

	device = alcGetContextsDevice(context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

void A3::buildHomeBase(Grid *grid) {
	int translateX = 10;
	for (int i = 1; i < 23; i++) {
		for (int j = 1; j < 23; j++) {
			grid->setBlock(i + translateX, 0, j, 5);
		}
	}

	for (int i = 0; i < 24; i++) {
		grid->setBlock(i + translateX, 1, 0, 8);
		grid->setBlock(i + translateX, 1, 24 - 1, 8);
		grid->setBlock(i + translateX, 2, 0, 4 * 16 + 1);
		grid->setBlock(i + translateX, 2, 24 - 1, 4 * 16 + 1);
		grid->setBlock(i + translateX, 3, 0, 8);
		grid->setBlock(i + translateX, 3, 24 - 1, 8);
	}

	for (int i = 0; i < 24; i++) {
		grid->setBlock(0 + translateX, 1, i, 4 * 16 + 1);
		grid->setBlock(24 - 1 + translateX, 1, i, 8);
		grid->setBlock(0 + translateX, 2, i, 4 * 16 + 1);
		grid->setBlock(24 - 1 + translateX, 2, i, 4 * 16 + 1);
		grid->setBlock(0 + translateX, 3, i, 4 * 16 + 1);
		grid->setBlock(24 - 1 + translateX, 3, i, 8);
	}

	for (int i = 0; i < 10; i++) {
		grid->setBlock(0 + translateX, i, 23, 17);
		grid->setBlock(1 + translateX, i, 23, 17);
		grid->setBlock(0 + translateX, i, 22, 17);
		grid->setBlock(1 + translateX, i, 22, 17);

		grid->setBlock(23 + translateX, i, 0, 17);
		grid->setBlock(23 + translateX, i, 1, 17);
		grid->setBlock(22 + translateX, i, 0, 17);
		grid->setBlock(22 + translateX, i, 1, 17);

		grid->setBlock(0 + translateX, i, 0, 17);
		grid->setBlock(1 + translateX, i, 0, 17);
		grid->setBlock(0 + translateX, i, 1, 17);
		grid->setBlock(1 + translateX, i, 1, 17);

		grid->setBlock(23 + translateX, i, 23, 17);
		grid->setBlock(22 + translateX, i, 23, 17);
		grid->setBlock(23 + translateX, i, 22, 17);
		grid->setBlock(22 + translateX, i, 22, 17);
	}

	grid->setBlock(2 + translateX, 6, 22, 5);
	grid->setBlock(2 + translateX, 6, 21, 5);
	grid->setBlock(1 + translateX, 6, 21, 5);

	grid->setBlock(21 + translateX, 6, 22, 5);
	grid->setBlock(21 + translateX, 6, 21, 5);
	grid->setBlock(22 + translateX, 6, 21, 5);

	grid->setBlock(2 + translateX, 6, 1, 5);
	grid->setBlock(2 + translateX, 6, 2, 5);
	grid->setBlock(1 + translateX, 6, 2, 5);

	grid->setBlock(21 + translateX, 6, 1, 5);
	grid->setBlock(22 + translateX, 6, 2, 5);
	grid->setBlock(21 + translateX, 6, 2, 5);

	grid->setBlock(3 + translateX, 1, 19, 3 * 16 + 12);
	grid->setBlock(3 + translateX, 1, 17, 4 * 16 + 12);
	grid->setBlock(3 + translateX, 1, 15, 4 * 16 + 11);

	grid->setBlock(4 + translateX, 0, 19, 24);
	grid->setBlock(4 + translateX, 0, 17, 24);
	grid->setBlock(4 + translateX, 0, 15, 24);

	for (int i = 0; i < 11; i++) {
		grid->setBlock(translateX - i, 0, 10, 5);
		grid->setBlock(translateX - i, 0, 15, 5);
		grid->setBlock(translateX - i, 1, 10, 4 * 16 + 1);
		grid->setBlock(translateX - i, 1, 15, 4 * 16 + 1);
		grid->setBlock(translateX - i, 2, 10, 4 * 16 + 1);
		grid->setBlock(translateX - i, 2, 15, 4 * 16 + 1);
		grid->setBlock(translateX - i, 3, 10, 4 * 16 + 1);
		grid->setBlock(translateX - i, 3, 15, 4 * 16 + 1);

		for (int j = 0; j < 4; j++) {
			grid->setBlock(translateX - i, 0, 11 + j, 8 * 16 + 2);
		}
	}
	for (int i = 1; i < 5; i++) {
		grid->setBlock(0, i ,10 + 1, 145);
		grid->setBlock(0, i ,10 + 2, 13 * 16 + 16);
		grid->setBlock(0, i ,10 + 3, 13 * 16 + 16);
		grid->setBlock(0, i ,10 + 4, 145);
	}

	for (int i = 0; i < 4; i++) {
		grid->setBlock(0, 4 ,12, 145);
		grid->setBlock(0, 4, 13, 145);

		for (int j = 0; j < 6; j++) {
			grid->setBlock(translateX, i+1 ,10 + j, 0);
		}
	}

	for (int i = 1; i < 5; i++) {
		grid->setBlock(28, i ,3 + 1, 14 * 16 + 1);
		grid->setBlock(28, i ,3 + 2, 14 * 16 + 16);
		grid->setBlock(28, i ,3 + 3, 14 * 16 + 16);
		grid->setBlock(28, i ,3 + 4, 14 * 16 + 1);
	}

	for (int i = 0; i < 4; i++) {
		grid->setBlock(28, 4 ,5, 14 * 16 + 1);
		grid->setBlock(28, 4, 6, 14 * 16 + 1);
		grid->setBlock(28, 0 ,5, 14 * 16 + 1);
		grid->setBlock(28, 0, 6, 14 * 16 + 1);
	}
}

void A3::setMap(int mapIndex) {
	particleSystem.killAll();
	projectileSystem.killAll();
	enemyFireCounter = 1;
	if (mapIndex == 1 && currMapIndex == 0) {
		alSourcePlay(bgmSource);
		alSourceStop(homeBgmSource);

		levelsPassed = 0;
		bulletsFired = 0;
		bulletsHit = 0;
		enemiesHit = 0;
		timesDied = 0;
		monstersKilled = 0;
	}
	if (mapIndex < maps.size()) {
		for (int i = 0; i < DIM_X; i++) {
			for (int j = 0; j < DIM_Y; j++) {
				for (int k = 0; k < DIM_Z; k++) {
					currMap->setBlock(i, j, k, maps[mapIndex]->getBlock(i,j,k));
				}
			}
		}
	} else if (mapIndex > currMapIndex) {
		// randomly generate map
		// clear map
		for (int i = 0; i < DIM_X; i++) {
			for (int j = 0; j < DIM_Y; j++) {
				for (int k = 0; k < DIM_Z; k++) {
					currMap->setBlock(i, j, k, 0);
				}
			}
		}

		int numPlatforms = 0;
		float minX = DIM_X, maxZ = 0, maxY = 0;
		queue<vec4> q;
		stack<vec4> platformsToBuild;
		q.push(vec4(DIM_X - 6, 1, 0, 4));
		while(numPlatforms < 20) {
			numPlatforms++;
			vec4 curr = q.front();
			q.pop();
			platformsToBuild.push(curr);

			int distX = rand() % 12 + 8;
			int distZ = rand() % 3 + 3;
			int distY = rand() % 14 - 2;
			int size = rand() % 8 + 2;

			vec4 left(std::min(DIM_X * 1.0f, std::max(0.0f, curr.x - distX)),
				std::min(DIM_Y * 1.0f, std::max(1.0f, curr.y + distY)),
				std::min(DIM_Z * 1.0f, std::max(0.0f, curr.z + distZ)),
				size);

			distX = rand() % 3 + 3;
			distZ = rand() % 12 + 8;
			distY = rand() % 14 - 2;
			size = rand() % 8 + 2;

			vec4 right(std::min(DIM_X * 1.0f, std::max(0.0f, curr.x - distX)),
				std::min(DIM_Y * 1.0f, std::max(1.0f, curr.y + distY)),
				std::min(DIM_Z * 1.0f, std::max(0.0f, curr.z + distZ)),
				size);
			if (numPlatforms == 1) {
				q.push(left);
				q.push(right);
			} else {
				int mode = rand() % 11;
				if (mode < 5) {
					q.push(left);
				} else if (mode < 10) {
					q.push(right);
				} else {
					q.push(left);
					q.push(right);
				}
			}
		}
		bool doorBuilt = false;
		vec4 toDoorPos;
		int platformsBuilt = 0;
		while (platformsToBuild.size() > 1) {
			vec4 coords = platformsToBuild.top();
			platformsToBuild.pop();
			int size = !doorBuilt ? 6 : coords.w;
			if (coords.x < 0 || coords.x+size >= DIM_X ||
				coords.z < 0 || coords.z+size >= DIM_Z ||
				coords.y + 5 >= DIM_Y) {
				continue;
			} else {
				platformsBuilt++;
				if (!doorBuilt) {
					doorBuilt = true;
					toDoorPos = coords;
				} else {
					minX = std::min(minX, coords.x);
					maxY = std::max(maxY, coords.y);
					maxZ = std::max(maxZ, coords.z);
					buildPlatform(currMap, coords.x, coords.y, coords.z, coords.w, 17);
					int numBlocks = rand() % (int)((coords.w - 1) * (coords.w - 1));
					for (int i = 0; i < numBlocks; i++) {
						int height = rand() % 3 + 2;
						int newX = rand() % (int)coords.w + (int)coords.x;
						int newY = rand() % (int)coords.w + (int)coords.z;
						for (int j = 1; j < height; j++) {
							currMap->setBlock(newX, 
							(int)coords.y + j, newY, 17);
						}
					}
				}
			}
		}
		int numEnemiesToGenerate = 3 + currMapIndex;
		for (int i = 0; i < numEnemiesToGenerate; i++) {
			int x = rand() % (int)(DIM_X - minX) + (int)(minX);
			int y = rand() % (int)(maxY);
			int z = rand() % (int)(maxZ);
			currMap->setBlock(x, y, z, -121);
		}

		// build doors last to ensure it exists
		vec4 fromDoorPos = platformsToBuild.top();
		platformsToBuild.pop();
		buildFromDoor(currMap, fromDoorPos.x, fromDoorPos.y, fromDoorPos.z);
		buildToDoor(currMap, toDoorPos.x, toDoorPos.y, toDoorPos.z);
	}
	if (mapIndex > currMapIndex) {
		currMapIndex = mapIndex;
		levelsPassed++;
	}
	if (mapIndex == 0) {
		life = MAX_LIFE;
		currMapIndex = 0;

		levelsPassedDisplayed = levelsPassed;
		bulletsFiredDisplayed = bulletsFired;
		bulletsHitDisplayed = bulletsHit;
		enemiesHitDisplayed = enemiesHit;
		timesDiedDisplayed = timesDied;
		monstersKilledDisplayed = monstersKilled;

		alSourcePlay(homeBgmSource);
		alSourceStop(bgmSource);
	}
	setupMap();
	resetPlayer();
}

void A3::buildPlatform(Grid *grid, int x, int y, int z, int size, int block) {
	for (int i = x; i < x + size; i++) {
		for (int j = z; j < z + size; j++) {
			grid->setBlock(i, y ,j, block);
		}
	}
	grid->setBlock(x, y-1 ,z, 6*16+14);
	grid->setBlock(x + size - 1, y-1 ,z, 6*16+14);
	grid->setBlock(x, y-1 ,z + size - 1, 6*16+14);
	grid->setBlock(x + size - 1, y-1 ,z + size - 1, 6*16+14);
}

void A3::buildToDoor(Grid *grid, int x, int y, int z) {
	for (int i = x; i < x + 6; i++) {
		for (int j = z; j < z + 6; j++) {
			grid->setBlock(i, y ,j, 7 * 16 + 5);
		}
	}
	for (int i = y + 1; i < y + 4; i++) {
		grid->setBlock(x, i ,z + 1, 145);
		grid->setBlock(x, i ,z + 2, 13 * 16 + 16);
		grid->setBlock(x, i ,z + 3, 13 * 16 + 16);
		grid->setBlock(x, i ,z + 4, 145);
	}

	for (int i = 0; i < 4; i++) {
		grid->setBlock(x, y + 4 ,z + 1 + i, 145);
		grid->setBlock(x, y + 4, z + 1 + i, 145);
		grid->setBlock(x, y ,z + 1 + i, 145);
		grid->setBlock(x, y, z + 1 + i, 145);
	}
}

void A3::buildFromDoor(Grid *grid, int x, int y, int z) {
	for (int i = x; i < x + 6; i++) {
		for (int j = z; j < z + 6; j++) {
			grid->setBlock(i, y ,j, 7 * 16 + 5);
		}
	}
	for (int i = y + 1; i < y + 4; i++) {
		grid->setBlock(x + 5, i ,z + 1, 14 * 16 + 1);
		grid->setBlock(x + 5, i ,z + 2, 14 * 16 + 16);
		grid->setBlock(x + 5, i ,z + 3, 14 * 16 + 16);
		grid->setBlock(x + 5, i ,z + 4, 14 * 16 + 1);
	}

	for (int i = 0; i < 4; i++) {
		grid->setBlock(x + 5, y + 4 ,z + 1 + i, 14 * 16 + 1);
		grid->setBlock(x + 5, y + 4, z + 1 + i, 14 * 16 + 1);
		grid->setBlock(x + 5, y ,z + 1 + i, 14 * 16 + 1);
		grid->setBlock(x + 5, y, z + 1 + i, 14 * 16 + 1);
	}
}

void A3::setupLight() {
	light_translate = cameraPos;
	l_perspective = ortho(-60.0f, 60.0f, -60.0f, 60.0f, l_near, l_far);
	l_view = lookAt(glm::vec3(-2.0f, 64 / 2 + 8.0f, 64 - 16.0f),
					glm::vec3( 64 / 2, 64 / 2,  64 / 2),
					glm::vec3( 0.0f, 1.0f,  0.0f));
	// l_view = lookAt(glm::vec3(-2.0f, DIM_Y / 2 + 8.0f, DIM_Z - 16.0f),
	// 				glm::vec3( DIM_X / 2, DIM_Y / 2,  DIM_Z / 2),
	// 				glm::vec3( 0.0f, 1.0f,  0.0f));
}

void A3::stepLight() {
	if (distance(cameraPos, light_translate) > 5) {
		light_translate = cameraPos;
	}
	l_view = lookAt(light_translate,
					glm::vec3( 34.0f, -8.0f,  -16.0f) + light_translate,
					glm::vec3( 0.0f, 1.0f,  0.0f));
}

bool A3::isCrouching() {
	return playerHeight == CROUCHED_HEIGHT;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A3::init()
{
	// Set the background colour.
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glClearColor(0.35, 0.35, 0.35, 1.0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	createShaderProgram();

	glGenVertexArrays(1, &m_vao_arcCircle);
	glGenVertexArrays(1, &m_vao_meshData);
	enableVertexShaderInputSlots();

	processLuaSceneFile(m_luaSceneFile);

	// Load and decode all .obj files at once here.  You may add additional .obj files to
	// this list in order to support rendering additional mesh types.  All vertex
	// positions, and normals will be extracted and stored within the MeshConsolidator
	// class.
	unique_ptr<MeshConsolidator> meshConsolidator (new MeshConsolidator{
			getAssetFilePath("cube.obj"),
			getAssetFilePath("sphere.obj"),
			getAssetFilePath("suzanne.obj")
	});

	// Acquire the BatchInfoMap from the MeshConsolidator.
	meshConsolidator->getBatchInfoMap(m_batchInfoMap);

	// Take all vertex data within the MeshConsolidator and upload it to VBOs on the GPU.
	uploadVertexDataToVbos(*meshConsolidator);

	mapVboDataToVertexShaderInputLocations();

	setupLight();

	initPerspectiveMatrix();

	initViewMatrix();

	initLightSources();
	setupCubemap();
	setupParticles();
	depthMap = setupDepthTexture();
	atlasTextureId = loadMapTexture("Assets/textures/textureAtlas3.jpg");

	vector<std::string> facesSunSet = {
		"Assets/textures/rt.png",
		"Assets/textures/lf.png",
		"Assets/textures/up.png",
		"Assets/textures/dn.png",
		"Assets/textures/bk.png",
		"Assets/textures/ft.png"
	};
	vector<std::string> facesNight = {
		"Assets/textures/rt2.png",
		"Assets/textures/lf2.png",
		"Assets/textures/up2.png",
		"Assets/textures/dn2.png",
		"Assets/textures/bk2.png",
		"Assets/textures/ft2.png"
	};
	cubemapTextureId = loadCubemap(facesSunSet);

	initSound();

	setMap(currMapIndex);
}

GLuint A3::loadMapTexture(const char * filename) {
	int width, height, n;
	unsigned char * data = stbi_load(filename, &width, &height, &n, 0);
	//loadBMP(filename, width, height, data);

	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	return textureID;
}

GLuint A3::loadCubemap(vector<string> faces) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++) {
		int width, height, n;
		unsigned char * data = stbi_load(faces[i].c_str(), &width, &height, &n, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		} else {
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void A3::resetPlayer() {
	inAirDisplacement = 0;
	//accelCapacity = MAX_ACCEL_CAPACITY;
	jumpCount = MAX_JUMP_COUNT;
	charVel = vec3(-0.25f, 0.1f, 0.125f);
	if (currMapIndex == 0) {
		cameraPos = vec3(28.5f, 1.5f, 5.5f);
		cameraUp = vec3(0.0f, 1.0f,  0.0f);
		yaw = 155;
	} else {
		cameraPos = vec3(DIM_X-0.5, 2.5f, 2.5f);
		cameraUp = vec3(0.0f, 1.0f,  0.0f);
		yaw = 155;
	}
	pitch = 0;
	
	glm::vec3 front;
	front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	front.y = sin(glm::radians(pitch));
	front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	cameraFront = glm::normalize(front);

	particleSystem.generateParticles(3, 500, cameraPos + 1.0f * cameraFront, cameraFront, 0.1f, 0.5f, 120);

	alSourcePlay(tpSource);
}

void A3::setupParticles() {
	vec3 triangleVertices[] = {
		vec3(-0.5, -0.5, -0.5),
		vec3(-0.5, 0.5, -0.5),
		vec3(-0.5, 0.5, 0.5),
		vec3(-0.5, -0.5, -0.5),
		vec3(-0.5, -0.5, 0.5),
		vec3(-0.5, 0.5, 0.5),

		vec3(-0.5, -0.5, -0.5),
		vec3(0.5, -0.5, -0.5),
		vec3(0.5, -0.5, 0.5),
		vec3(-0.5, -0.5, -0.5),
		vec3(-0.5, -0.5, 0.5),
		vec3(0.5, -0.5, 0.5),

		vec3(-0.5, -0.5, -0.5),
		vec3(-0.5, 0.5, -0.5),
		vec3(0.5, 0.5, -0.5),
		vec3(-0.5, -0.5, -0.5),
		vec3(0.5, -0.5, -0.5),
		vec3(0.5, 0.5, -0.5),

		vec3(0.5, 0.5, 0.5),
		vec3(0.5, -0.5, 0.5),
		vec3(0.5, -0.5, -0.5),
		vec3(0.5, 0.5, 0.5),
		vec3(0.5, 0.5, -0.5),
		vec3(0.5, -0.5, -0.5),

		vec3(0.5, 0.5, 0.5),
		vec3(-0.5 , 0.5, 0.5),
		vec3(-0.5, 0.5, -0.5),
		vec3(0.5, 0.5, 0.5),
		vec3(0.5, 0.5, -0.5),
		vec3(-0.5, 0.5, -0.5),

		vec3(0.5, 0.5, 0.5),
		vec3(-0.5, 0.5, 0.5),
		vec3(-0.5, -0.5, 0.5),
		vec3(0.5, 0.5, 0.5),
		vec3(0.5, -0.5, 0.5),
		vec3(-0.5, -0.5, 0.5),
	};
	glGenVertexArrays( 1, &m_particle_vao );
	glBindVertexArray( m_particle_vao );

	GLint posAttrib = m_shader_particle.getAttribLocation( "position" );
	glEnableVertexAttribArray( posAttrib );

	GLint offsetAttrib = m_shader_particle.getAttribLocation( "offsetSize" );
	glEnableVertexAttribArray( offsetAttrib );

	GLint colourAttrib = m_shader_particle.getAttribLocation( "colour" );
	glEnableVertexAttribArray( colourAttrib );

	glGenBuffers( 1, &m_particle_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_particle_vbo );
	glBufferData( GL_ARRAY_BUFFER, sizeof(triangleVertices),
		triangleVertices, GL_STATIC_DRAW );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

	glGenBuffers(1, &m_particles_pos_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_particles_pos_vbo);
	glBufferData(GL_ARRAY_BUFFER, particleSystem.maxParticles * 4 * sizeof(GL_FLOAT), NULL, GL_STREAM_DRAW);
	glVertexAttribPointer( offsetAttrib, 4, GL_FLOAT, GL_TRUE, 0, nullptr );

	glGenBuffers(1, &m_particles_col_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_particles_col_vbo);
	glBufferData(GL_ARRAY_BUFFER, particleSystem.maxParticles * 4 * sizeof(GL_FLOAT), NULL, GL_STREAM_DRAW);
	glVertexAttribPointer( colourAttrib, 4, GL_FLOAT, GL_TRUE, 0, nullptr );

	// Reset state to prevent rogue code from messing with *my* 
	// stuff!
	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	CHECK_GL_ERRORS;
}

void A3::setupMap() {
	enemyBlocks.clear();
	transparentMapBlocks.clear();
	opaqueBlocks.clear();
	for (int x = 0; x < DIM_X; x++) {
		for (int y = 0; y < DIM_Y; y++) {
			for (int z = 0; z < DIM_Z; z++) {
				int block = currMap->getBlock(x,y,z);
				if (block != 0) {
					vec3 triangleVertices[] = {
						vec3(x, y-1, z),
						vec3(x, y, z),
						vec3(x, y, z + 1),
						vec3(x, y-1, z),
						vec3(x, y-1, z + 1),
						vec3(x, y, z + 1),

						vec3(x, y-1, z),
						vec3(x + 1, y-1, z),
						vec3(x + 1, y-1, z + 1),
						vec3(x, y-1, z),
						vec3(x, y-1, z + 1),
						vec3(x + 1, y-1, z + 1),

						vec3(x, y-1, z),
						vec3(x, y, z),
						vec3(x + 1, y, z),
						vec3(x, y-1, z),
						vec3(x + 1, y-1, z),
						vec3(x + 1, y, z),

						vec3(x + 1, y, z + 1),
						vec3(x + 1, y-1, z + 1),
						vec3(x + 1, y-1, z),
						vec3(x + 1, y, z + 1),
						vec3(x + 1, y, z),
						vec3(x + 1, y-1, z),

						vec3(x + 1, y, z + 1),
						vec3(x , y, z + 1),
						vec3(x, y, z),
						vec3(x + 1, y, z + 1),
						vec3(x + 1, y, z),
						vec3(x, y, z),

						vec3(x + 1, y, z + 1),
						vec3(x, y, z + 1),
						vec3(x, y-1, z + 1),
						vec3(x + 1, y, z + 1),
						vec3(x + 1, y-1, z + 1),
						vec3(x, y-1, z + 1),
					};

					vec2 UVVertices[36];
					uint gridId = abs(block) - 1;
					for (int i = 0; i < 12; i++) {
						for (int j = 0; j < 3; j++) {
							vec3 endToPos = triangleVertices[i*3] - triangleVertices[i*3+j];
							vec3 zxy = vec3(triangleNormals[i/2].z, triangleNormals[i/2].x, triangleNormals[i/2].y);
							vec3 yzx = vec3(triangleNormals[i/2].y, triangleNormals[i/2].z, triangleNormals[i/2].x);
							vec2 tileUV = vec2(dot(zxy, endToPos), dot(yzx, endToPos));
							vec2 tileOffset = gridLength * vec2(fmod(gridId, 16), floor(gridId * 1.0f / 16));
							UVVertices[i*3+j] = tileOffset + gridLength * tileUV;
						}
					}

					// Create the vertex array to record buffer assignments.
					glGenVertexArrays( 1, &(m_block_vao[x * DIM_Z * DIM_Y + y * DIM_Z + z]) );
					glBindVertexArray( m_block_vao[x * DIM_Z * DIM_Y + y * DIM_Z + z] );

					// Create the cube vertex buffer
					glGenBuffers( 1, &(m_block_vbo[z * DIM_Z * DIM_Y + y * DIM_Z + x]) );
					glBindBuffer( GL_ARRAY_BUFFER, m_block_vbo[z * DIM_Z * DIM_Y + y * DIM_Z + x] );
					glBufferData( GL_ARRAY_BUFFER, sizeof(triangleVertices),
						triangleVertices, GL_STATIC_DRAW );

					// Specify the means of extracting the position values properly.
					GLint posAttribEntity = m_shader_entity.getAttribLocation( "position" );
					glEnableVertexAttribArray( posAttribEntity );
					glVertexAttribPointer( posAttribEntity, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr );

					GLint posAttribDepth = m_shader_depth.getAttribLocation( "position" );
					glEnableVertexAttribArray( posAttribDepth );
					glVertexAttribPointer( posAttribDepth, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr );

					// Create the cube vertex buffer
					glGenBuffers( 1, &(m_block_uv_vbo[z * DIM_Z * DIM_Y + y * DIM_Z + x]) );
					glBindBuffer( GL_ARRAY_BUFFER, m_block_uv_vbo[z * DIM_Z * DIM_Y + y * DIM_Z + x] );
					glBufferData( GL_ARRAY_BUFFER, sizeof(UVVertices),
						UVVertices, GL_STATIC_DRAW );

					// Specify the means of extracting the normal values properly.
					GLint tileUVAttrib = m_shader_entity.getAttribLocation( "tileUV" );
					glEnableVertexAttribArray( tileUVAttrib );
					glVertexAttribPointer( tileUVAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), nullptr );

					// Reset state to prevent rogue code from messing with *my* 
					// stuff!
					glBindVertexArray( 0 );
					glBindBuffer( GL_ARRAY_BUFFER, 0 );
					glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

					CHECK_GL_ERRORS;

					if (!currMap->isOpaque(x,y,z)) {
						transparentMapBlocks.push_back(vec3(x,y,z));
					} else {
						opaqueBlocks.push_back(vec3(x,y,z));
					}

					if (block < 0) {
						enemyBlocks.push_back(vec3(x,y,z));
					}
				}
			}
		}
	}
}

GLuint A3::setupDepthTexture() {
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureID, 0);
	glDrawBuffer(GL_NONE);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "false" << endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return textureID;
}

void A3::setupCubemap() {
	vec3 triangleVertices[] = {
		vec3(-1.0f,  1.0f, -1.0f),
		vec3(-1.0f, -1.0f, -1.0f),
		vec3(1.0f, -1.0f, -1.0f),
		vec3(1.0f, -1.0f, -1.0f),
		vec3(1.0f,  1.0f, -1.0f),
		vec3(-1.0f,  1.0f, -1.0f),

		vec3(-1.0f, -1.0f,  1.0f),
		vec3(-1.0f, -1.0f, -1.0f),
		vec3(-1.0f,  1.0f, -1.0f),
		vec3(-1.0f,  1.0f, -1.0f),
		vec3(-1.0f,  1.0f,  1.0f),
		vec3(-1.0f, -1.0f,  1.0f),

		vec3(1.0f, -1.0f, -1.0f),
		vec3(1.0f, -1.0f,  1.0f),
		vec3(1.0f,  1.0f,  1.0f),
		vec3(1.0f,  1.0f,  1.0f),
		vec3(1.0f,  1.0f, -1.0f),
		vec3(1.0f, -1.0f, -1.0f),

		vec3(-1.0f, -1.0f,  1.0f),
		vec3(-1.0f,  1.0f,  1.0f),
		vec3(1.0f,  1.0f,  1.0f),
		vec3(1.0f,  1.0f,  1.0f),
		vec3(1.0f, -1.0f,  1.0f),
		vec3(-1.0f, -1.0f,  1.0f),

		vec3(-1.0f,  1.0f, -1.0f),
		vec3(1.0f,  1.0f, -1.0f),
		vec3(1.0f,  1.0f,  1.0f),
		vec3(1.0f,  1.0f,  1.0f),
		vec3(-1.0f,  1.0f,  1.0f),
		vec3(-1.0f,  1.0f, -1.0f),

		vec3(-1.0f, -1.0f, -1.0f),
		vec3(-1.0f, -1.0f,  1.0f),
		vec3(1.0f, -1.0f, -1.0f),
		vec3(1.0f, -1.0f, -1.0f),
		vec3(-1.0f, -1.0f,  1.0f),
		vec3(1.0f, -1.0f,  1.0f)
	};
	// Create the vertex array to record buffer assignments.
	glGenVertexArrays( 1, &m_vao_cubemap );
	glBindVertexArray( m_vao_cubemap );

	// Create the cube vertex buffer
	glGenBuffers( 1, &m_vbo_cubemap );
	glBindBuffer( GL_ARRAY_BUFFER, m_vbo_cubemap );
	glBufferData( GL_ARRAY_BUFFER, sizeof(triangleVertices),
		triangleVertices, GL_STATIC_DRAW );

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader_cubemap.getAttribLocation( "position" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr );

	// Create
	// Reset state to prevent rogue code from messing with *my* 
	// stuff!
	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	CHECK_GL_ERRORS;
}

void A3::removeEntity(vector<vec3> &vec, int i, int j, int k) {
	for (vector<vec3>::iterator it = vec.begin() ; it != vec.end(); ++it) {
		if (it->x == i && it->y == j && it->z == k) {
			vec.erase(it);
			break;
		}
	}
}

//----------------------------------------------------------------------------------------
void A3::processLuaSceneFile(const std::string & filename) {
	// This version of the code treats the Lua file as an Asset,
	// so that you'd launch the program with just the filename
	// of a puppet in the Assets/ directory.
	// std::string assetFilePath = getAssetFilePath(filename.c_str());
	// m_rootNode = std::shared_ptr<SceneNode>(import_lua(assetFilePath));

	// This version of the code treats the main program argument
	// as a straightforward pathname.
	m_rootNode = std::shared_ptr<SceneNode>(import_lua(filename));
	if (!m_rootNode) {
		std::cerr << "Could not open " << filename << std::endl;
	} else {
		m_rootNode->set_transform(translate(mat4(1.0f),
			vec3(cos( gun_ani_counter ) * 0.02f, sin(gun_ani_counter * 2) * 0.01f, 0)));
	}
}

//----------------------------------------------------------------------------------------
void A3::createShaderProgram()
{
	m_shader.generateProgramObject();
	m_shader.attachVertexShader( getAssetFilePath("VertexShader.vs").c_str() );
	m_shader.attachFragmentShader( getAssetFilePath("FragmentShader.fs").c_str() );
	m_shader.link();

	m_shader_arcCircle.generateProgramObject();
	m_shader_arcCircle.attachVertexShader( getAssetFilePath("arc_VertexShader.vs").c_str() );
	m_shader_arcCircle.attachFragmentShader( getAssetFilePath("arc_FragmentShader.fs").c_str() );
	m_shader_arcCircle.link();

	m_shader_entity.generateProgramObject();
	m_shader_entity.attachVertexShader( getAssetFilePath("entity_VertexShader.vs").c_str() );
	m_shader_entity.attachFragmentShader( getAssetFilePath("entity_FragmentShader.fs").c_str() );
	m_shader_entity.link();

	m_shader_particle.generateProgramObject();
	m_shader_particle.attachVertexShader( getAssetFilePath("particle_VertexShader.vs").c_str() );
	m_shader_particle.attachFragmentShader( getAssetFilePath("particle_FragmentShader.fs").c_str() );
	m_shader_particle.link();

	m_shader_cubemap.generateProgramObject();
	m_shader_cubemap.attachVertexShader( getAssetFilePath("cubemap_VertexShader.vs").c_str() );
	m_shader_cubemap.attachFragmentShader( getAssetFilePath("cubemap_FragmentShader.fs").c_str() );
	m_shader_cubemap.link();

	m_shader_depth.generateProgramObject();
	m_shader_depth.attachVertexShader( getAssetFilePath("depth_VertexShader.vs").c_str() );
	m_shader_depth.attachFragmentShader( getAssetFilePath("depth_FragmentShader.fs").c_str() );
	m_shader_depth.link();

	debug.generateProgramObject();
	debug.attachVertexShader( getAssetFilePath("debug_VertexShader.vs").c_str() );
	debug.attachFragmentShader( getAssetFilePath("debug_FragmentShader.fs").c_str() );
	debug.link();
}

//----------------------------------------------------------------------------------------
void A3::enableVertexShaderInputSlots()
{
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(m_vao_meshData);

		// Enable the vertex shader attribute location for "position" when rendering.
		m_positionAttribLocation = m_shader.getAttribLocation("position");
		glEnableVertexAttribArray(m_positionAttribLocation);

		// Enable the vertex shader attribute location for "normal" when rendering.
		m_normalAttribLocation = m_shader.getAttribLocation("normal");
		glEnableVertexAttribArray(m_normalAttribLocation);

		CHECK_GL_ERRORS;
	}


	//-- Enable input slots for m_vao_arcCircle:
	{
		glBindVertexArray(m_vao_arcCircle);

		// Enable the vertex shader attribute location for "position" when rendering.
		m_arc_positionAttribLocation = m_shader_arcCircle.getAttribLocation("position");
		glEnableVertexAttribArray(m_arc_positionAttribLocation);

		CHECK_GL_ERRORS;
	}

	// Restore defaults
	glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------
void A3::uploadVertexDataToVbos (
		const MeshConsolidator & meshConsolidator
) {
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &m_vbo_vertexPositions);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);

		glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexPositionBytes(),
				meshConsolidator.getVertexPositionDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// Generate VBO to store all vertex normal data
	{
		glGenBuffers(1, &m_vbo_vertexNormals);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);

		glBufferData(GL_ARRAY_BUFFER, meshConsolidator.getNumVertexNormalBytes(),
				meshConsolidator.getVertexNormalDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// Generate VBO to store the trackball circle.
	{
		glGenBuffers( 1, &m_vbo_arcCircle );
		glBindBuffer( GL_ARRAY_BUFFER, m_vbo_arcCircle );

		float *pts = new float[ 2 * CIRCLE_PTS ];
		for( size_t idx = 0; idx < CIRCLE_PTS; ++idx ) {
			float ang = 2.0 * M_PI * float(idx) / CIRCLE_PTS;
			pts[2*idx] = cos( ang );
			pts[2*idx+1] = sin( ang );
		}

		glBufferData(GL_ARRAY_BUFFER, 2*CIRCLE_PTS*sizeof(float), pts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
void A3::mapVboDataToVertexShaderInputLocations()
{
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao_meshData);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);
	glVertexAttribPointer(m_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexNormals" into the
	// "normal" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);
	glVertexAttribPointer(m_normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;

	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao_arcCircle);

	// Tell GL how to map data from the vertex buffer "m_vbo_arcCircle" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_arcCircle);
	glVertexAttribPointer(m_arc_positionAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
void A3::initPerspectiveMatrix()
{
	float aspect = ((float)m_windowWidth) / m_windowHeight;
	m_perspective = glm::perspective(degreesToRadians(fov + accelFov), aspect, 0.1f, 150.0f);
}


//----------------------------------------------------------------------------------------
void A3::initViewMatrix() {
	m_view = glm::lookAt(cameraPos, cameraPos + cameraFront,
			cameraUp);
}

//----------------------------------------------------------------------------------------
void A3::initLightSources() {
	// World-space position
	m_light.position = vec3(-2.0f, 5.0f, 0.5f);
	m_light.rgbIntensity = vec3(0.8f); // White light
}

//----------------------------------------------------------------------------------------
void A3::uploadCommonSceneUniforms() {
	m_shader.enable();
	{
		//-- Set perspective matrix uniform for the scene:
		GLint location = m_shader.getUniformLocation("Perspective");
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perspective));
		CHECK_GL_ERRORS;


		//-- Set LightSource uniform for the scene:
		{
			location = m_shader.getUniformLocation("light.position");
			glUniform3fv(location, 1, value_ptr(m_light.position));
			location = m_shader.getUniformLocation("light.rgbIntensity");
			glUniform3fv(location, 1, value_ptr(m_light.rgbIntensity));
			CHECK_GL_ERRORS;
		}

		//-- Set background light ambient intensity
		{
			location = m_shader.getUniformLocation("ambientIntensity");
			vec3 ambientIntensity(0.05f);
			glUniform3fv(location, 1, value_ptr(ambientIntensity));
			CHECK_GL_ERRORS;
		}
	}
	m_shader.disable();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A3::appLogic()
{
	//Place per frame, application logic here ...
	step();
	initPerspectiveMatrix();
	stepLight();
	stepSound();
	uploadCommonSceneUniforms();
}

// loading wav file without using alut.h
// taken from tutorial at: https://www.youtube.com/watch?v=V83Ja4FmrqE
void A3::loadWav(const char * soundPath, ALuint source, ALuint buffer) {
	FILE *fp = NULL;
	fp = fopen(soundPath, "rb");
	char type[4];
	DWORD size, chunkSize;
	short formatType, channels;
	DWORD sampleRate, avgBytesPerSec;
	short bytesPerSample,bitsPerSample;
	DWORD dataSize;

	fread(type, sizeof(char), 4, fp);
	if (type[0]!='R' || type[1] != 'I' || type[2] != 'F' || type[3] != 'F')
		cout << "No RIFF" << endl;

	fread(&size, sizeof(DWORD), 1, fp);
	fread(type, sizeof(char), 4, fp);
	if (type[0]!='W' || type[1] != 'A' || type[2] != 'V' || type[3] != 'E')
		cout << "Not WAVE" << endl;

	fread(type, sizeof(char), 4, fp);
	if (type[0]!='f' || type[1] != 'm' || type[2] != 't' || type[3] != ' ')
		cout << "Not fmt" << endl;

	fread(&chunkSize, sizeof(DWORD), 1, fp);
	fread(&formatType, sizeof(short), 1, fp);
	fread(&channels, sizeof(short), 1, fp);
	fread(&sampleRate, sizeof(DWORD), 1, fp);
	fread(&avgBytesPerSec, sizeof(DWORD), 1, fp);
	fread(&bytesPerSample, sizeof(short), 1, fp);
	fread(&bitsPerSample, sizeof(short), 1, fp);

	fread(&type, sizeof(char), 4, fp);
	if (type[0]!='d' || type[1] != 'a' || type[2] != 't' || type[3] != 'a')
		cout << "Missing DATA" << endl;

	fread(&dataSize, sizeof(DWORD), 1, fp);

	unsigned char* buf = new unsigned char[dataSize];
	fread(buf, sizeof(BYTE), dataSize, fp);

	ALuint frequency=sampleRate;
	ALenum format=0;
	if (bitsPerSample == 8) {
		if (channels == 1)
			format = AL_FORMAT_MONO8;
		else if (channels == 2)
			format = AL_FORMAT_STEREO8;
	} else if (bitsPerSample == 16) {
		if (channels == 1)
			format = AL_FORMAT_MONO16;
		else if (channels == 2)
			format = AL_FORMAT_STEREO16;
	}
	alBufferData(buffer, format, buf, dataSize, frequency);
	alSourcei(source, AL_BUFFER, buffer);
	fclose(fp);
}

void A3::initSound() {
	device = alcOpenDevice(NULL);
	if (!device) cout << "No device" << endl;
	context = alcCreateContext(device, NULL);
	if (!alcMakeContextCurrent(context)) cout << "No context" << endl;
	ALfloat listenerOri[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	alListener3f(AL_POSITION, cameraPos.x, cameraPos.y, cameraPos.z);
	alListener3f(AL_VELOCITY, 0, 0, 0);
	alListenerfv(AL_ORIENTATION, listenerOri);

	alGenSources((ALuint)1, &gunSource);
	alSourcei(gunSource, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcef(gunSource, AL_PITCH, 1);
	alSourcef(gunSource, AL_GAIN, 0.4f);
	alSource3f(gunSource, AL_POSITION, 0, 0, 0);
	alSource3f(gunSource, AL_VELOCITY, 0, 0, 0);
	alSourcei(gunSource, AL_LOOPING, AL_FALSE);

	alGenBuffers((ALuint)1, &shootBuffer);

	loadWav("Assets/sounds/wpn_rifleassaultg3_fire_3d.wav", gunSource, shootBuffer);

	alGenSources((ALuint)1, &walk1Source);
	alSourcef(walk1Source, AL_PITCH, 1);
	alSourcef(walk1Source, AL_GAIN, 1);
	alSource3f(walk1Source, AL_POSITION, cameraPos.x, cameraPos.y - 1.0f, cameraPos.z);
	alSource3f(walk1Source, AL_VELOCITY, 0, 0, 0);
	alSourcei(walk1Source, AL_LOOPING, AL_FALSE);

	alGenBuffers((ALuint)1, &walk1Buffer);

	loadWav("Assets/sounds/walk_01.wav", walk1Source, walk1Buffer);

	alGenSources((ALuint)1, &walk2Source);
	alSourcef(walk2Source, AL_PITCH, 1);
	alSourcef(walk2Source, AL_GAIN, 1);
	alSource3f(walk2Source, AL_POSITION, cameraPos.x, cameraPos.y - 1.0f, cameraPos.z);
	alSource3f(walk2Source, AL_VELOCITY, 0, 0, 0);
	alSourcei(walk2Source, AL_LOOPING, AL_FALSE);

	alGenBuffers((ALuint)1, &walk2Buffer);

	loadWav("Assets/sounds/walk_02.wav", walk2Source, walk2Buffer);

	alGenSources((ALuint)1, &launcherSource);
	alSourcei(launcherSource, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcef(launcherSource, AL_PITCH, 1);
	alSourcef(launcherSource, AL_GAIN, 1);
	alSource3f(launcherSource, AL_POSITION, 0, 0, 0);
	alSource3f(launcherSource, AL_VELOCITY, 0, 0, 0);
	alSourcei(launcherSource, AL_LOOPING, AL_FALSE);

	alGenBuffers((ALuint)1, &launcherBuffer);

	loadWav("Assets/sounds/launcher.wav", launcherSource, launcherBuffer);

	alGenSources((ALuint)1, &expSource);
	alSourcef(expSource, AL_PITCH, 1);
	alSourcef(expSource, AL_GAIN, 1);
	alSource3f(expSource, AL_POSITION, cameraPos.x, cameraPos.y - 1.0f, cameraPos.z);
	alSource3f(expSource, AL_VELOCITY, 0, 0, 0);
	alSourcei(expSource, AL_LOOPING, AL_FALSE);

	alGenBuffers((ALuint)1, &expBuffer);

	loadWav("Assets/sounds/DSBAREXP.wav", expSource, expBuffer);

	alGenSources((ALuint)1, &dieSource);
	alSourcei(dieSource, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcef(dieSource, AL_PITCH, 1);
	alSourcef(dieSource, AL_GAIN, 0.6f);
	alSource3f(dieSource, AL_POSITION, 0, 0, 0);
	alSource3f(dieSource, AL_VELOCITY, 0, 0, 0);
	alSourcei(dieSource, AL_LOOPING, AL_FALSE);

	alGenBuffers((ALuint)1, &dieBuffer);

	loadWav("Assets/sounds/DSPDIEHI.wav", dieSource, dieBuffer);

	alGenSources((ALuint)1, &flameSource);
	alSourcef(flameSource, AL_PITCH, 1);
	alSourcef(flameSource, AL_GAIN, 1);
	alSource3f(flameSource, AL_POSITION, cameraPos.x, cameraPos.y - 1.0f, cameraPos.z);
	alSource3f(flameSource, AL_VELOCITY, 0, 0, 0);
	alSourcei(flameSource, AL_LOOPING, AL_FALSE);

	alGenBuffers((ALuint)1, &flameBuffer);

	loadWav("Assets/sounds/DSBDOPN.wav", flameSource, flameBuffer);

	alGenSources((ALuint)1, &enemySource);
	alSourcef(enemySource, AL_PITCH, 1);
	alSourcef(enemySource, AL_GAIN, 1);
	alSource3f(enemySource, AL_POSITION, cameraPos.x, cameraPos.y - 1.0f, cameraPos.z);
	alSource3f(enemySource, AL_VELOCITY, 0, 0, 0);
	alSourcei(enemySource, AL_LOOPING, AL_FALSE);

	alGenBuffers((ALuint)1, &enemyBuffer);

	loadWav("Assets/sounds/DSBOSPIT.wav", enemySource, enemyBuffer);

	alGenSources((ALuint)1, &tpSource);
	alSourcei(tpSource, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcef(tpSource, AL_PITCH, 1);
	alSourcef(tpSource, AL_GAIN, 1);
	alSource3f(tpSource, AL_POSITION, 0, 0, 0);
	alSource3f(tpSource, AL_VELOCITY, 0, 0, 0);
	alSourcei(tpSource, AL_LOOPING, AL_FALSE);

	alGenBuffers((ALuint)1, &tpBuffer);

	loadWav("Assets/sounds/DSDOROPN.wav", tpSource, tpBuffer);

	alGenSources((ALuint)1, &bgmSource);
	alSourcei(bgmSource, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcef(bgmSource, AL_PITCH, 1);
	alSourcef(bgmSource, AL_GAIN, 0.25f);
	alSource3f(bgmSource, AL_POSITION, 0, 0, 0);
	alSource3f(bgmSource, AL_VELOCITY, 0, 0, 0);
	alSourcei(bgmSource, AL_LOOPING, AL_TRUE);

	alGenBuffers((ALuint)1, &bgmBuffer);

	loadWav("Assets/sounds/bgm_02.wav", bgmSource, bgmBuffer);

	alGenSources((ALuint)1, &homeBgmSource);
	alSourcei(homeBgmSource, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcef(homeBgmSource, AL_PITCH, 1);
	alSourcef(homeBgmSource, AL_GAIN, 1.0f);
	alSource3f(homeBgmSource, AL_POSITION, 0, 0, 0);
	alSource3f(homeBgmSource, AL_VELOCITY, 0, 0, 0);
	alSourcei(homeBgmSource, AL_LOOPING, AL_TRUE);

	alGenBuffers((ALuint)1, &homeBgmBuffer);

	loadWav("Assets/sounds/bgm_01.wav", homeBgmSource, homeBgmBuffer);

	alGenSources((ALuint)1, &accelEngineSource);
	alSourcei(accelEngineSource, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcef(accelEngineSource, AL_PITCH, 1);
	alSourcef(accelEngineSource, AL_GAIN, 0.75f);
	alSource3f(accelEngineSource, AL_POSITION, 0, 0, 0);
	alSource3f(accelEngineSource, AL_VELOCITY, 0, 0, 0);
	alSourcei(accelEngineSource, AL_LOOPING, AL_FALSE);

	alGenBuffers((ALuint)1, &accelEngineBuffer);

	loadWav("Assets/sounds/engine_beep.wav", accelEngineSource, accelEngineBuffer);
}

bool A3::isPlaying(ALuint source) {
	ALenum state;
	alGetSourcei(source, AL_SOURCE_STATE, &state);
	return (state == AL_PLAYING);
}

void A3::stepSound() {
	alListener3f(AL_POSITION, cameraPos.x, cameraPos.y, cameraPos.z);
	alSourcef(accelEngineSource, AL_PITCH, 0.75 - accelCapacity / MAX_ACCEL_CAPACITY * 0.75f);
	alSourcef(accelEngineSource, AL_GAIN, 0.25f + accelCapacity * 0.5f / MAX_ACCEL_CAPACITY);
}

void A3::step() {
	float maxMovementSpd = NORMAL_MOVEMENT_SPD;
	if (accelCapacity < 100.0f && !isPlaying(accelEngineSource)) {
		alSourcePlay(accelEngineSource);
	}
	if (accelPressed && accelCapacity > 0) {
		maxMovementSpd = ELEVATED_MOVEMENT_SPD;
		accelCapacity = std::max(0.0f, accelCapacity - ACCEL_CONSUMPTION_RATE);
	} else if (!accelPressed && accelCapacity < 100.0f) {
		accelCapacity = std::min(MAX_ACCEL_CAPACITY, accelCapacity + ACCEL_REPLENISH_RATE);
	}

	if (maxMovementSpd == ELEVATED_MOVEMENT_SPD) {
		accelFov = std::min(30.0f, accelFov + 0.5f);
	} else {
		accelFov = std::max(0.0f, accelFov - 0.5f);
	}

	if (cameraPos.y < -25) {
		// life-=2;
		// if (life <= 0) {
		// 	setMap(0);
		// 	alSourcePlay(dieSource);
		// } else {
			resetPlayer();
			// alSourcePlay(dieSource);
		// }
		return;
	}

	shootCounter = std::max(0, shootCounter - 1);
	if (recoil != 0.0f) {
		recoil = std::max(0.0f, recoil - 0.01f);
	}

	// Simulate all particles
	particleSystem.particlesCount = 0;
	for(int i=0; i<particleSystem.maxParticles; i++){
		Particle& p = particleSystem.particlesContainer[i]; // shortcut
		if(p.life > 0.0f){
			// Decrease life
			p.life -= 1.0f;
			if (p.life > 0.0f){
				// Simulate simple physics : gravity only, no collisions
				p.speed += 0.1f * GRAVITY;
				p.pos += p.speed;
				p.col.a = p.life / p.maxLife;
				p.cameradistance = glm::distance( p.pos, cameraPos );
				//ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;
				// Fill the GPU buffer
				if (currMapIndex != 0 || p.pos.y <= 22) {
					g_particule_position_size_data[4*particleSystem.particlesCount+0] = p.pos.x;
					g_particule_position_size_data[4*particleSystem.particlesCount+1] = p.pos.y;
					g_particule_position_size_data[4*particleSystem.particlesCount+2] = p.pos.z;
					g_particule_position_size_data[4*particleSystem.particlesCount+3] = p.size;

					g_particule_color_data[4*particleSystem.particlesCount+0] = p.col.r;
					g_particule_color_data[4*particleSystem.particlesCount+1] = p.col.g;
					g_particule_color_data[4*particleSystem.particlesCount+2] = p.col.b;
					g_particule_color_data[4*particleSystem.particlesCount+3] = p.col.a;
					particleSystem.particlesCount++;
				}
			} else {
				// Particles that just died will be put at the end of the buffer in SortParticles();
				p.cameradistance = -1.0f;
			}
		}
	}
	/*
	commented out for performance
	particleSystem.sortByCameraDistance();
	*/

	// controls
	if (zoomPressed && (currZoom <= ZOOM_COEFFICIENT || fov >= MIN_FOV)) {
		currZoom = std::min(ZOOM_COEFFICIENT, currZoom + 0.0015f);
		fov = std::max(MIN_FOV, fov - 2);
	} else if (!zoomPressed && (currZoom >= 0 || fov <= MAX_FOV)) {
		currZoom = std::max(0.0f, currZoom - 0.0015f);
		fov = std::min(MAX_FOV, fov + 2);
	}

	// MOVEMENT
	if ( wasdPressed[0] ) {
		charVel += MOVEMENT_ACCEL * glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
	}
	if ( wasdPressed[1] ) {
		charVel -= glm::normalize(glm::cross(cameraFront, cameraUp)) * MOVEMENT_ACCEL;
	}
	if ( wasdPressed[2] ) {
		charVel -= MOVEMENT_ACCEL * glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
	}
	if ( wasdPressed[3] ) {
		charVel += glm::normalize(glm::cross(cameraFront, cameraUp)) * MOVEMENT_ACCEL;
	}
	
	if (crouchPressed) {
		playerHeight = CROUCHED_HEIGHT;
	} else {
		int ceiling = getCeiling();
		if (cameraPos.y + CAMERA_HEIGHT < ceiling) {
			 playerHeight = CAMERA_HEIGHT;
		}
	}

	charVel += GRAVITY;
	if (length(charVel) > 0) {
		charVel -= AIR_RESISTANCE * normalize(charVel);
	}

	int ground = getGround();
	vec3 horizontalVelocity(charVel.x, 0.0f, charVel.z);
	if (length(horizontalVelocity) > 0 && ground > -1 &&
		cameraPos.y - playerHeight <= ground + 1) {

		vec3 newcharVel = charVel - FRICTION * normalize(horizontalVelocity);
		if (!sameSign(newcharVel.x, charVel.x)) {
			newcharVel.x = 0;
		}
		if (!sameSign(newcharVel.z, charVel.z)) {
			newcharVel.z = 0;
		}
		charVel = newcharVel;
	}

	horizontalVelocity = vec3(charVel.x, 0.0f, charVel.z);
	if (length(horizontalVelocity) >= maxMovementSpd) {
		vec3 maxV = maxMovementSpd * normalize(horizontalVelocity);
		charVel.x = maxV.x;
		charVel.z = maxV.z;
	}

	initViewMatrix();

	// gravity
	float prevPos = cameraPos.y;
	cameraPos.y += charVel.y;
	if (ground > -1) {
		cameraPos.y = std::max(float(ground + playerHeight), cameraPos.y);
		if (cameraPos.y - playerHeight == ground) {
			charVel.y = 0.0f;
			jumpCount = MAX_JUMP_COUNT;
			if (prevPos != cameraPos.y) {
				if (walkSourceSwap) {
					alSource3f(walk1Source, AL_POSITION, cameraPos.x, cameraPos.y - 1.0f, cameraPos.z);
					alSourcePlay(walk1Source);
				} else {
					alSource3f(walk2Source, AL_POSITION, cameraPos.x, cameraPos.y - 1.0f, cameraPos.z);
					alSourcePlay(walk2Source);
				}
				walkSourceSwap = !walkSourceSwap;
			}
		}
	}
	int ceiling = getCeiling();
	if (ceiling < DIM_Y -1) {
		cameraPos.y = std::min(float(ceiling - CAMERA_RADIUS), cameraPos.y);
		if (cameraPos.y + CAMERA_RADIUS == ceiling) {
			charVel.y = 0.0f;
		}
	}

	// horizontal collision
	float newX = cameraPos.x + charVel.x;
	float newZ = cameraPos.z + charVel.z;
	int floorX = std::floor(cameraPos.x);
	int ceilingY = std::ceil(cameraPos.y);
	int floorY = std::floor(cameraPos.y);
	int floorZ = std::floor(cameraPos.z);
	bool toRight = cameraPos.x <= newX;
	bool toUp = cameraPos.z <= newZ;
	int newFloorX = toRight ? std::floor(newX + CAMERA_RADIUS) : std::floor(newX - CAMERA_RADIUS);
	int newFloorZ = toUp ? std::floor(newZ + CAMERA_RADIUS) : std::floor(newZ - CAMERA_RADIUS);

	bool xBlocked(false), yBlocked(false);
	if (currMap->isSolid(newFloorX, ceilingY, floorZ) ||
		(!isCrouching() && currMap->isSolid(newFloorX, floorY, floorZ))) {
		xBlocked = true;
		cameraPos.x = toRight ? newFloorX - CAMERA_RADIUS : newFloorX + 1 + CAMERA_RADIUS;
		charVel.x = 0;
	} else {
		cameraPos.x = newX;
	}
	if (currMap->isSolid(floorX, ceilingY, newFloorZ) ||
		(!isCrouching() && currMap->isSolid(floorX, floorY, newFloorZ))) {
		yBlocked = true;
		cameraPos.z = toUp ? newFloorZ - CAMERA_RADIUS : newFloorZ + 1 + CAMERA_RADIUS;
		charVel.z = 0;
	} else {
		cameraPos.z = newZ;
	}

	if (!xBlocked && !yBlocked) {
		// diagonal
		if (currMap->isSolid(newFloorX, ceilingY, newFloorZ) ||
			(!isCrouching() && currMap->isSolid(newFloorX, floorY, newFloorZ))) {
			cameraPos.x = toRight ? newFloorX - CAMERA_RADIUS : newFloorX + 1 + CAMERA_RADIUS;
			charVel.x = 0;
		}
	}

	// gun animation
	float horizontalSpeed = length(vec2(charVel.x, charVel.z));
	if (ground > -1 && cameraPos.y - playerHeight <= ground + 1 && horizontalSpeed > 0) {
		gun_ani_counter = fmod(gun_ani_counter + horizontalSpeed, 2.0f * PI);
	}

	if (ground > -1 && cameraPos.y - playerHeight <= ground + 1){
		inAirDisplacement = std::max(0.0f, inAirDisplacement - 0.015f);
		inAirVel = 0;
	} else {
		inAirDisplacement += inAirVel;
		inAirVel = 0.001f;
	}

	if (zoomPressed) {
		inAirDisplacement = 0;
	}
	float oscillation_x = cos( PI / 2 + gun_ani_counter ) * 0.03f;
	float oscillation_y = sin(gun_ani_counter * 2) * 0.01f;
	float zoom_x = -currZoom * 2;
	float zoom_y = currZoom;
	float zoom_z = currZoom / ZOOM_COEFFICIENT * 0.015;

	// enemy projectile
	bool projectileExists = false;
	vec3 posClosestToCamera;
	vec3 projSpeed;
	for(int i=0; i<projectileSystem.maxProjectile; i++){
		Projectile& p = projectileSystem.projectileContainer[i];
		if(p.life > 0.0f){
			p.life -= 1.0f;
			if (p.life > 0.0f){
				if (currMap->getBlock(floor(p.pos.x), std::ceil(p.pos.y), floor(p.pos.z)) > 0
					&& currMap->isSolid(floor(p.pos.x), std::ceil(p.pos.y), floor(p.pos.z))) {
					p.life = -1.0f;
					particleSystem.generateParticles(0, 250, p.pos, -p.speed, 0.2f, 0.15f, 60);
					playFarSoundClose(expSource, p.pos);
					continue;
				} else if (!god_mode && distance(p.pos, cameraPos) < 1.0f) {
					timesDied++;
					playFarSoundClose(expSource, p.pos);
					alSourcePlay(dieSource);
					p.life = -1.0f;
					life-=2;
					charVel = charVel + 0.2f * normalize(cameraPos - p.pos) + 0.1f * vec3(0.0f, 1.0f, 0.0f);
					if (life <= 0) {
						setMap(0);
						break;
					}
					particleSystem.generateParticles(1, 500, cameraPos + 1.0f * cameraFront, cameraFront, 0.1f, 0.5f, 120);
					continue;
				}
				p.pos += p.speed;
				if (!projectileExists) {
					projectileExists = true;
					posClosestToCamera = p.pos;
					projSpeed = p.speed;
				} else if (distance(p.pos, cameraPos) < distance(posClosestToCamera, cameraPos)) {
					posClosestToCamera = p.pos;
				}
				particleSystem.generateParticles(0, 50, p.pos, -p.speed, 0.2f, 0.3f, 10);
			}
		}
	}
	if (projectileExists && flameCounter == 0) {
		alSource3f(flameSource, AL_VELOCITY, projSpeed.x, projSpeed.y, projSpeed.z);
		playFarSoundClose(flameSource, posClosestToCamera);
	}
	if (projectileExists) {
		flameCounter = (flameCounter + 1) % FLAME_PERIOD;
	}

	if (enemyFireCounter == 0) {
		bool enemyExists = false;
		vec3 enemyClosestToCamera;
		for(vec3 block : enemyBlocks) {
			vec3 centerBlock(block.x + 0.5f, block.y - 0.5f, block.z + 0.5f);
			projectileSystem.generateProjectile(centerBlock,
				ENEMY_FIRE_SPEED * normalize(cameraPos + 20.0f * charVel - centerBlock));
			if (!enemyExists) {
				enemyExists = true;
				enemyClosestToCamera = centerBlock;
			} else if (distance(centerBlock, cameraPos) < distance(enemyClosestToCamera, cameraPos)) {
				enemyClosestToCamera = centerBlock;
			}
		}
		if (enemyExists) {
			playFarSoundClose(enemySource, enemyClosestToCamera);
		}
	}
	enemyFireCounter = (enemyFireCounter + 1) % ENEMY_FIRE_PERIOD;

	// SHOOTING
	if (shootingPressed && shootCounter == 0) {
		//fakeAmmoCount--;
		alSourcePlay(gunSource);
		shootCounter = SHOOT_PERIOD;
		recoil = 0.03;

		// setting up camera coordinate system
		vec3 cameraRight = normalize(cross(cameraFront, cameraUp));
		vec3 cameraVertical = normalize(cross(cameraRight, cameraFront));

		// initial pos and velocity
		vec3 pos = cameraPos + 0.40f * cameraFront;
		vec3 initVel = 5.0f * (pos - cameraPos + cameraFront);

		// shift to gun
		pos = pos + 0.05f * cameraRight - 0.05f * cameraVertical;

		// add oscillation
		pos = pos + oscillation_x * cameraRight + oscillation_y * cameraVertical;

		// in air displacement?
		if (!zoomPressed) {
			pos = pos - inAirDisplacement * cameraVertical;
		}

		// add zoom
		pos = pos + zoom_x * cameraRight + zoom_y * cameraVertical + zoom_z * cameraFront;
		particleSystem.generateParticles(0, 250, pos, initVel, 0.005f, 0.01f, 5);

		collide();

		if (!zoomPressed) {
			pitch = std::min(89.0f, std::max(-89.0f, pitch + 0.5f));
			yaw   = glm::mod( yaw + ((rand() % 2) ? 0.3f : -0.3f), 360.0f );
		} else {
			pitch = std::min(89.0f, std::max(-89.0f, pitch + 0.1f));
			yaw   = glm::mod( yaw + ((rand() % 2) ? 0.05f : -0.05f), 360.0f );
		}
		

		glm::vec3 front;
		front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		front.y = sin(glm::radians(pitch));
		front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		cameraFront = glm::normalize(front);
	}

	m_rootNode->set_transform(translate(mat4(1.0f),
			vec3(oscillation_x, oscillation_y, 0)) *
			translate(mat4(1.0f), vec3(0, 0, recoil)) *
			translate(mat4(1.0f), vec3(zoom_x, zoom_y, zoom_z)) *
			translate(mat4(1.0f), vec3(0, zoomPressed ? 0 : -inAirDisplacement, 0)));
	if (ground != -1 && cameraPos.y - playerHeight == ground && horizontalSpeed > 0.05) {
		charVel.y += WALK_HOP;
	}
	sort(transparentMapBlocks.begin(), transparentMapBlocks.end(), CompareCameraDistance(cameraPos));

	// generate snow
	if (currMapIndex == 0) {
		if (snowCounter == 0) {
			particleSystem.generateParticles(2, 250, vec3(82, 32, 82), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 250, vec3(72, 32, 82), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 250, vec3(82, 32, 72), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 250, vec3(72, 32, 72), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 250, vec3(62, 32, 62), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 200, vec3(62, 32, 52), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 200, vec3(52, 32, 42), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 200, vec3(42, 32, 32), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 200, vec3(42, 32, 42), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 200, vec3(32, 32, 22), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 200, vec3(32, 32, 32), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
			particleSystem.generateParticles(2, 200, vec3(22, 32, 32), vec3(-1, 0, -1), 0.1f, 0.5f, 600);
		}
		snowCounter = (snowCounter + 1) % SNOW_PERIOD;
	}

	vec3 nextPos = cameraPos + charVel;
	if (currMap->getBlock(floor(nextPos.x), ceil(nextPos.y), floor(nextPos.z)) == 13 * 16 + 16) {
		setMap(currMapIndex + 1);
	}
}

bool A3::collide() {
	static const glm::vec3 b_p[6][3] = {
		{
			vec3(0,0,0),
			vec3(1,0,0),
			vec3(0,-1,0),
		},
		{
			vec3(0,0,0),
			vec3(0,0,1),
			vec3(1,0,0),
		},
		{
			vec3(0,0,0),
			vec3(0,-1,0),
			vec3(0,0,1),
		},
		{
			vec3(1,-1,1),
			vec3(0,-1,1),
			vec3(1,0,1),
		},
		{
			vec3(1,-1,0),
			vec3(1,-1,1),
			vec3(1,0,0),
		},
		{
			vec3(1,-1,0),
			vec3(0,-1,0),
			vec3(1,-1,1),
		}
	};
	bulletsFired++;
	float t = 99999;
	bool isHit = false;
	vec3 curr = cameraPos + 0.5f * cameraFront;
	int i, j, k;
	while (distance(curr, cameraPos) < DIM_X) {
		i = floor(curr.x);
		j = ceil(curr.y);
		k = floor(curr.z);
		if (currMap->isSolid(i, j, k)) {
			for (int w = 0; w < 6; w++) {
				float b;
				float a;
				float candidate;
				planeIntersect(cameraPos, cameraPos + cameraFront, vec3(i, j, k) + b_p[w][0],
					vec3(i, j, k) + b_p[w][1], vec3(i, j, k) + b_p[w][2], b, a, candidate);
				if (b >= 0 && b <= 1 && a >= 0 && a <= 1) {
					if (candidate >= 0.001 && candidate < t) {
						t = candidate;
						isHit = true;
					}
				}
			}
			if (isHit) break;
		}
		curr += 0.25f * cameraFront;
	}
	/*
	slower version but more robust
	for (int i = 0; i < DIM_X; i++) {
		for (int j = 0; j < DIM_Y; j++) {
			for (int k = 0; k < DIM_Z; k++) {
				int block = currMap->getBlock(i, j, k);
				if (block != 0) {
					for (int w = 0; w < 6; w++) {
						float b;
						float a;
						float candidate;
						planeIntersect(cameraPos, cameraPos + cameraFront, vec3(i, j, k) + b_p[w][0],
							vec3(i, j, k) + b_p[w][1], vec3(i, j, k) + b_p[w][2], b, a, candidate);
						if (b >= 0 && b <= 1 && a >= 0 && a <= 1) {
							if (candidate >= 0.001 && candidate < t) {
								t = candidate;
								isHit = true;
							}
						}
					}
				}
			}
		}
	}
	*/
	if (isHit) {
		bulletsHit++;
		if (currMap->getBlock(i, j, k) < 0) {
			enemiesHit++;
		}
		currMap->hitBlock(i, j, k);
		if (currMap->getHealth(i, j, k) < 0) {
			if (currMap->getBlock(i, j, k) < 0) {
				monstersKilled++;
				life = std::min(MAX_LIFE, life + 1);
			}
			currMap->setBlock(i, j, k, 0);
			vec3 center(i + 0.5f, j - 0.5f, k + 0.5f);
			particleSystem.generateParticles(0, 250, center, vec3(0), 0.2f, 0.5f, 120);
			removeEntity(enemyBlocks, i, j, k);
			removeEntity(transparentMapBlocks, i, j, k);
			removeEntity(opaqueBlocks, i, j, k);

			playFarSoundClose(expSource, center);
		}
		particleSystem.generateParticles(0, 100, cameraPos + t * cameraFront,
			vec3(0, 0.1, 0), 0.05f, 0.05f, 90);
	}
	return isHit;
}

void A3::playFarSoundClose(ALuint source, vec3 pos) {
	vec3 dist = 0.3f * (pos - cameraPos);
	vec3 closerPos = cameraPos + dist;
	alSource3f(source, AL_POSITION, closerPos.x, closerPos.y, closerPos.z);
	alSourcePlay(source);
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A3::guiLogic()
{
	static bool showDebugWindow(true);
	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::SetNextWindowPos(ImVec2(50, show_gui ? m_windowHeight - 315 : m_windowHeight - 125));
	ImGui::Begin("Player", &showDebugWindow, ImVec2(100,100), 0.5f,
			windowFlags);

		char barString[16];
		barString[0] = '[';
		barString[1] = '-';
		barString[14] = '-';
		barString[15] = ']';
		for (int i = 0; i < 12; i++) {
			if (i < life) {
				barString[2+i] = '|';
			} else {
				barString[2+i] = ' ';
			}
		}
		ImGui::Text( "                Health: %s (%.1f%%)", barString, life * 100.0f / MAX_LIFE );
		float numJumpsBar = jumpCount * 12.0f / MAX_JUMP_COUNT;
		for (int i = 0; i < 12; i++) {
			if (i < numJumpsBar) {
				barString[2+i] = '|';
			} else {
				barString[2+i] = ' ';
			}
		}
		ImGui::Text( "Jump-Assist Compressor: %s (%.1f%%)", barString, jumpCount * 100.0f / MAX_JUMP_COUNT );
		float numAccelBar = accelCapacity * 12.0f / MAX_ACCEL_CAPACITY;
		for (int i = 0; i < 12; i++) {
			if (i < numAccelBar) {
				barString[2+i] = '|';
			} else {
				barString[2+i] = ' ';
			}
		}
		ImGui::Text( "    Accelerator Engine: %s (%.1f%%)", barString, accelCapacity * 100.0f / MAX_ACCEL_CAPACITY );

		ImGui::Separator();
		if (currMapIndex == 0) {
			ImGui::Text( "Location: Secret Base" );
		} else {
			ImGui::Text( "Location: Inter-Dimentional Space Lvl %d", currMapIndex );
		}
		if (show_gui) {
			ImGui::Text( "Speed: %.2fm/s", length(charVel) * 60 );
			ImGui::Text( "Coordinate: (x: %.2f, y: %.2f, z: %.2f)", cameraPos.x, cameraPos.y, cameraPos.z );
			ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );
			ImGui::Separator();
			ImGui::Text( "Invincibility (G): %s", god_mode ? "ON" : "OFF" );
			ImGui::Text( "Render Shadows (H): %s", render_shadows ? "ON" : "OFF" );
			ImGui::Text( "Render Textures (J): %s", render_textures ? "ON" : "OFF" );
			ImGui::Text( "Show Shadow Map Texture (K): %s", show_shadow_map ? "ON" : "OFF" );
			ImGui::Text( "Skip Level (V)" );
			ImGui::Text( "Return To Base (B)" );
			ImGui::Text( "Restart Level (N)" );
			ImGui::Text( "Reset Player (M)" );
		}

	ImGui::End();

	if (firstRun) {
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		ImGui::Begin("Beginning", &showDebugWindow, ImVec2(100,100), 0.8f, windowFlags);
			ImGui::SetWindowFontScale(1.5f);
			ImGui::Text( "Message From Sergeant Toot O'Real:" );
			ImGui::Separator();
			ImGui::Text( "- Hi there Private. Welcome to our secret base!" );
			ImGui::Text( "You can rest here and prepare for your mission." );
			ImGui::Text( "\n- If you need any information from the HQ," );
			ImGui::Text( "come have a chat with us by standing in front of our TELECOMMUNICATORS." );
			ImGui::Text( "They're the high-tech looking boxes to the left of the portal." );
			ImGui::Text( "\n(Press Enter to Acknowledge this message)" );
		ImGui::End();
	} else if (currMapIndex == 0 && floor(cameraPos.x) == OPT_1.x && floor(cameraPos.z) == OPT_1.z) {
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		ImGui::Begin("Instructions", &showDebugWindow, ImVec2(100,100), 0.8f, windowFlags);
			ImGui::SetWindowFontScale(1.5f);
			ImGui::Text( "Sergeant Toot O'Real:" );
			ImGui::Separator();
			ImGui::Text( "- Private, did you forget your mission already?\n\n- Go through our inter-dimentional portal to your right to\neradicate the evil pumpkin aliens." );
			ImGui::Text( "\n- Don't worry, if you're severely injured, we will teleport you right back\nto home base where you can rest and check your statistics." );

		ImGui::End();
	} else if (currMapIndex == 0 && floor(cameraPos.x) == OPT_2.x && floor(cameraPos.z) == OPT_2.z) {
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		ImGui::Begin("Instructions2", &showDebugWindow, ImVec2(100,100), 0.8f, windowFlags);
			ImGui::SetWindowFontScale(1.5f);
			ImGui::Text( "Captain T. Cherr:" );
			ImGui::Separator();
			ImGui::Text( "- Private, You forgot your basic bootcamp skills!? Ugh, don't forget them this time:");
			ImGui::Text( "\n- WASD: Movement");
			ImGui::Text( "- SHIFT: Speed Boost");
			ImGui::Text( "- SPACE: Jump");
			ImGui::Text( "- CTRL: Crouch");
			ImGui::Text( "- MOUSE: Aim");
			ImGui::Text( "- LEFT CLICK: Shoot");
			ImGui::Text( "- RIGHT CLICK: Zoom");
			ImGui::Text( "\n- L:  Show debug info");
		ImGui::End();
	} else if (currMapIndex == 0 && floor(cameraPos.x) == OPT_3.x && floor(cameraPos.z) == OPT_3.z) {
		ImGui::SetNextWindowPos(ImVec2(50, 50));
		ImGui::Begin("Instructions3", &showDebugWindow, ImVec2(100,100), 0.8f, windowFlags);
			ImGui::SetWindowFontScale(1.5f);
			ImGui::Text( "Lieutenant Stah T. Istik:" );
			ImGui::Separator();
			ImGui::Text( "- You want to know how well you did last mission? Here: ");
			ImGui::Text( "\n- Levels Passed: %d", levelsPassedDisplayed);
			ImGui::Text( "- Number of Shots Taken: %d", timesDiedDisplayed);
			ImGui::Text( "- Monsters Killed: %d", monstersKilledDisplayed);
			ImGui::Text( "- Bullets Fired: %d", bulletsFiredDisplayed);
			ImGui::Text( "- All Blocks Hit: %d", bulletsHitDisplayed);
			ImGui::Text( "- Enemy Blocks Hit: %d", enemiesHitDisplayed);
			ImGui::Text( "- Overall Accuracy: %.1f%%", enemiesHitDisplayed * 100.0f / bulletsFiredDisplayed);
		ImGui::End();
	}
}

//----------------------------------------------------------------------------------------
// Update mesh specific shader uniforms:
static void updateShaderUniforms(
		const ShaderProgram & shader,
		const GeometryNode & node,
		const SceneNode & parentNode,
		const glm::mat4 & viewMatrix
) {

	shader.enable();
	{
		//-- Set ModelView matrix:
		GLint location = shader.getUniformLocation("ModelView");
		mat4 modelView = parentNode.trans * node.trans;
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(modelView));
		CHECK_GL_ERRORS;

		//-- Set NormMatrix:
		location = shader.getUniformLocation("NormalMatrix");
		mat3 normalMatrix = glm::transpose(glm::inverse(mat3(modelView)));
		glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(normalMatrix));
		CHECK_GL_ERRORS;


		//-- Set Material values:
		location = shader.getUniformLocation("material.kd");
		vec3 kd = node.material.kd;
		glUniform3fv(location, 1, value_ptr(kd));
		CHECK_GL_ERRORS;
		location = shader.getUniformLocation("material.ks");
		vec3 ks = node.material.ks;
		glUniform3fv(location, 1, value_ptr(ks));
		CHECK_GL_ERRORS;
		location = shader.getUniformLocation("material.shininess");
		glUniform1f(location, node.material.shininess);
		CHECK_GL_ERRORS;

	}
	shader.disable();

}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A3::draw() {
	renderDepthMap();
	if (show_shadow_map) {
		renderQuad();
	} else {
		glEnable( GL_DEPTH_TEST );
		if (m_rootNode) renderSceneGraph(*m_rootNode);
		glDisable( GL_DEPTH_TEST );
		renderParticles();
		renderEntities();
		renderCubemap();
		renderTransparentEntities();
		renderArcCircle();
	}
}

//----------------------------------------------------------------------------------------
void A3::renderSceneGraph(const SceneNode & root) {

	// Bind the VAO once here, and reuse for all GeometryNode rendering below.
	glBindVertexArray(m_vao_meshData);

	// This is emphatically *not* how you should be drawing the scene graph in
	// your final implementation.  This is a non-hierarchical demonstration
	// in which we assume that there is a list of GeometryNodes living directly
	// underneath the root node, and that we can draw them in a loop.  It's
	// just enough to demonstrate how to get geometry and materials out of
	// a GeometryNode and onto the screen.

	// You'll want to turn this into recursive code that walks over the tree.
	// You can do that by putting a method in SceneNode, overridden in its
	// subclasses, that renders the subtree rooted at every node.  Or you
	// could put a set of mutually recursive functions in this class, which
	// walk down the tree from nodes of different types.

	for (const SceneNode * node : root.children) {

		if (node->m_nodeType != NodeType::GeometryNode)
			continue;

		const GeometryNode * geometryNode = static_cast<const GeometryNode *>(node);

		updateShaderUniforms(m_shader, *geometryNode, *m_rootNode, m_view);


		// Get the BatchInfo corresponding to the GeometryNode's unique MeshId.
		BatchInfo batchInfo = m_batchInfoMap[geometryNode->meshId];

		//-- Now render the mesh:
		m_shader.enable();
		glDrawArrays(GL_TRIANGLES, batchInfo.startIndex, batchInfo.numIndices);
		m_shader.disable();
	}

	glBindVertexArray(0);
	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
// Draw the trackball circle.
void A3::renderArcCircle() {
	glBindVertexArray(m_vao_arcCircle);

	m_shader_arcCircle.enable();
		GLint m_location = m_shader_arcCircle.getUniformLocation( "M" );
		float aspect = float(m_framebufferWidth)/float(m_framebufferHeight);
		glm::mat4 M;
		float size = 0.01f + (MAX_FOV - fov) * 0.0005f;
		if( aspect > 1.0 ) {
			M = glm::scale( glm::mat4(), glm::vec3( size/aspect, size, 1.0 ) );
		} else {
			M = glm::scale( glm::mat4(), glm::vec3( size, size*aspect, 1.0 ) );
		}
		glUniformMatrix4fv( m_location, 1, GL_FALSE, value_ptr( M ) );
		glDrawArrays( GL_LINE_LOOP, 0, CIRCLE_PTS );
	m_shader_arcCircle.disable();

	glBindVertexArray(0);
	CHECK_GL_ERRORS;
}

void A3::renderDepthMap() {
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		m_shader_depth.enable();
			glEnable( GL_DEPTH_TEST );

			// Draw the cubes
			// @TODO: instance the cubes
			GLint P_uni = m_shader_depth.getUniformLocation( "P" );
			GLint V_uni = m_shader_depth.getUniformLocation( "V" );

			glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( l_perspective ) );
			glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( l_view ) );
			for (vector<vec3>::iterator it = opaqueBlocks.begin(); it != opaqueBlocks.end(); ++it) {
				int x = it->x, y = it->y, z = it->z;
				int block = currMap->getBlock(x, y, z);
				if (block != 0 && currMap->isOpaque(x, y, z)) {
					glBindVertexArray( m_block_vao[x * DIM_Z * DIM_Y + y * DIM_Z + z] );
					if (render_shadows) glDrawArrays( GL_TRIANGLES, 0, 36 );
				}
			}
		m_shader_depth.disable();

	// Restore defaults
	glBindVertexArray( 0 );
	CHECK_GL_ERRORS;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, m_framebufferWidth, m_framebufferHeight);
}

//----------------------------------------------------------------------------------------
// Draw the map.
void A3::renderEntities() {
	m_shader_entity.enable();
		glEnable( GL_DEPTH_TEST );

		glUniform1i(m_shader_entity.getUniformLocation( "textureSampler" ), 0);
		glUniform1i(m_shader_entity.getUniformLocation( "shadowMap" ), 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlasTextureId);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);

		// Draw the cubes
		// @TODO: instance the cubes
		GLint col_uni = m_shader_entity.getUniformLocation( "colour" );
		GLint light_uni = m_shader_entity.getUniformLocation( "lightSpaceMatrix" );
		GLint P_uni = m_shader_entity.getUniformLocation( "P" );
		GLint V_uni = m_shader_entity.getUniformLocation( "V" );
		GLint render_textures_uni = m_shader_entity.getUniformLocation( "render_textures" );

		glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( m_perspective ) );
		glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( m_view ) );
		glUniformMatrix4fv( light_uni, 1, GL_FALSE, value_ptr( biasMatrix * l_perspective * l_view ) );
		glUniform1i( render_textures_uni, render_textures );
		for (vector<vec3>::iterator it = opaqueBlocks.begin(); it != opaqueBlocks.end(); ++it) {
			int x = it->x, y = it->y, z = it->z;
			int block = currMap->getBlock(x, y, z);
			if (distance(vec3(x,y,z), cameraPos) < 96) {
				if (block == 6*16+14) {
					particleSystem.generateParticles(0, 15, vec3(x + 0.5f, y - 1, z + 0.5f), vec3(0, -1.5f, 0), 0.15f, 0.2f, 5);
				}
				if (block == -121) {
					particleSystem.generateParticles(1, 15, vec3(x + 0.5f, y - 1, z + 0.5f), vec3(0, -0.3f, 0), 0.25f, 0.2f, 5);
				}
			}

			if (block != 0 && currMap->isOpaque(x, y, z)) {
				vec4 col = currMap->getCol(x, y, z);
				glBindVertexArray( m_block_vao[x * DIM_Z * DIM_Y + y * DIM_Z + z] );
				glUniform4f( col_uni, col.r, col.g, col.b, col.a);
				glDrawArrays( GL_TRIANGLES, 0, 36 );
			}
		}
	m_shader_entity.disable();

	// Restore defaults
	glBindVertexArray( 0 );

	CHECK_GL_ERRORS;
}

void A3::renderTransparentEntities() {
	m_shader_entity.enable();
		glEnable( GL_DEPTH_TEST );

		glUniform1i(m_shader_entity.getUniformLocation( "textureSampler" ), 0);
		glUniform1i(m_shader_entity.getUniformLocation( "shadowMap" ), 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlasTextureId);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);

		/// Draw the cubes
		GLint col_uni = m_shader_entity.getUniformLocation( "colour" );
		GLint light_uni = m_shader_entity.getUniformLocation( "lightSpaceMatrix" );
		GLint P_uni = m_shader_entity.getUniformLocation( "P" );
		GLint V_uni = m_shader_entity.getUniformLocation( "V" );
		GLint render_textures_uni = m_shader_entity.getUniformLocation( "render_textures" );

		glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( m_perspective ) );
		glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( m_view ) );
		glUniformMatrix4fv( light_uni, 1, GL_FALSE, value_ptr( biasMatrix * l_perspective * l_view ) );
		glUniform1i( render_textures_uni, render_textures );
		for (vector<vec3>::iterator it = transparentMapBlocks.begin(); it != transparentMapBlocks.end(); ++it) {
			int i = it->x, j = it->y, k = it->z;
			int block = currMap->getBlock(i, j, k);
			if (block != 0) {
				vec4 col = currMap->getCol(i, j, k);
				glBindVertexArray( m_block_vao[i * DIM_Z * DIM_Y + j * DIM_Z + k] );
				glUniform4f( col_uni, col.r, col.g, col.b, col.a);
				glDrawArrays( GL_TRIANGLES, 0, 36 );
			}
		}
	m_shader_entity.disable();

	// Restore defaults
	glBindVertexArray( 0 );

	CHECK_GL_ERRORS;
}

void A3::renderParticles() {
	m_shader_particle.enable();
		glEnable( GL_DEPTH_TEST );

		GLint P_uni = m_shader_particle.getUniformLocation( "P" );
		GLint V_uni = m_shader_particle.getUniformLocation( "V" );
		GLint M_uni = m_shader_particle.getUniformLocation( "M" );

		glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( m_perspective ) );
		glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( m_view ) );
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( mat4(1.0f) ) );
		glBindVertexArray( m_particle_vao );

		glBindBuffer(GL_ARRAY_BUFFER, m_particles_pos_vbo);
		glBufferData(GL_ARRAY_BUFFER, particleSystem.maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		glBufferSubData(GL_ARRAY_BUFFER, 0, particleSystem.particlesCount * 4 * sizeof(GLfloat), g_particule_position_size_data);

		glBindBuffer(GL_ARRAY_BUFFER, m_particles_col_vbo);
		glBufferData(GL_ARRAY_BUFFER, particleSystem.maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
		glBufferSubData(GL_ARRAY_BUFFER, 0, particleSystem.particlesCount * sizeof(GLfloat) * 4, g_particule_color_data);

		GLint posAttrib = m_shader_particle.getAttribLocation( "position" );
		GLint offsetAttrib = m_shader_particle.getAttribLocation( "offsetSize" );
		GLint colourAttrib = m_shader_particle.getAttribLocation( "colour" );

		glVertexAttribDivisor(posAttrib, 0); // particles vertices : always reuse the same 4 vertices -> 0
		glVertexAttribDivisor(offsetAttrib, 1); // positions : one per quad (its center) -> 1
		glVertexAttribDivisor(colourAttrib, 1);

		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, particleSystem.particlesCount);
	m_shader_particle.disable();

	// Restore defaults
	glBindVertexArray( 0 );

	CHECK_GL_ERRORS;
}

void A3::renderCubemap() {
	m_shader_cubemap.enable();
		glEnable( GL_DEPTH_TEST );
		glDepthFunc(GL_LEQUAL);

		GLint P_uni = m_shader_cubemap.getUniformLocation( "P" );
		GLint V_uni = m_shader_cubemap.getUniformLocation( "V" );
		GLint render_textures_uni = m_shader_cubemap.getUniformLocation( "render_textures" );

		glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( m_perspective ) );
		glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( m_view ) );
		glUniform1i( render_textures_uni, render_textures );
		glBindVertexArray( m_vao_cubemap );

		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureId);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthFunc(GL_LESS);

	m_shader_cubemap.disable();

	// Restore defaults
	glBindVertexArray( 0 );

	CHECK_GL_ERRORS;
}

int A3::getGround() {
	int x = std::floor(cameraPos.x);
	int y;
	int z = std::floor(cameraPos.z);
	for (y = std::floor(cameraPos.y); y >= 0; y--) {
		if (currMap->isSolid(x, y, z)) {
			break;
		}
	}
	return y;
}

int A3::getCeiling() {
	int x = std::floor(cameraPos.x);
	int y;
	int z = std::floor(cameraPos.z);
	for (y = std::ceil(cameraPos.y); y < DIM_Y; y++) {
		if (currMap->isSolid(x, y, z)) {
			break;
		}
	}
	return y-1;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A3::cleanup()
{

}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A3::cursorEnterWindowEvent (
		int entered
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool A3::mouseMoveEvent (
		double xPos,
		double yPos
) {
	bool eventHandled(true);

	if(initMouse) {
		prevXPos = xPos;
		prevYPos = yPos;
		initMouse = false;
		return true;
	}

	float xoffset = xPos - prevXPos;
	float yoffset = prevYPos - yPos; // reversed since y-coordinates range from bottom to top
	prevXPos = xPos;
	prevYPos = yPos;

	xoffset *= zoomPressed ? 0.35f * SENSITIVITY : SENSITIVITY;
	yoffset *= zoomPressed ? 0.35f * SENSITIVITY : SENSITIVITY;;

	yaw   = glm::mod( yaw + xoffset, 360.0f );
	pitch += yoffset;
	pitch = std::min(89.0f, std::max(-89.0f, pitch));

	glm::vec3 front;
	front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	front.y = sin(glm::radians(pitch));
	front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	cameraFront = glm::normalize(front);

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A3::mouseButtonInputEvent (
		int button,
		int actions,
		int mods
) {
	bool eventHandled(false);

	// Fill in with event handling code...
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (actions == GLFW_PRESS && !firstRun) {
			shootingPressed = true;
			eventHandled = true;
		}
		if (actions == GLFW_RELEASE) {
			shootingPressed = false;
			eventHandled = true;
		}
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (actions == GLFW_PRESS && !firstRun) {
			zoomPressed = true;
			eventHandled = true;
		}
		if (actions == GLFW_RELEASE) {
			zoomPressed = false;
			eventHandled = true;
		}
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A3::mouseScrollEvent (
		double xOffSet,
		double yOffSet
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A3::windowResizeEvent (
		int width,
		int height
) {
	bool eventHandled(false);
	initPerspectiveMatrix();
	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A3::keyInputEvent (
		int key,
		int action,
		int mods
) {
	bool eventHandled(false);

	if( action == GLFW_PRESS ) {
		if ( key == GLFW_KEY_P ) {
			if (cursor_hidden) glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			else glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			cursor_hidden = !cursor_hidden;
		}
		if ( key == GLFW_KEY_ENTER ) {
			firstRun = false;
		}
		if (!firstRun) {
			if( key == GLFW_KEY_L ) {
				show_gui = !show_gui;
				eventHandled = true;
			}
			if( key == GLFW_KEY_M ) {
				resetPlayer();
				eventHandled = true;
			}
			if( key == GLFW_KEY_N ) {
				setMap(currMapIndex);
				eventHandled = true;
			}
			if( key == GLFW_KEY_V ) {
				setMap(currMapIndex + 1);
				resetPlayer();
				eventHandled = true;
			}
			if( key == GLFW_KEY_B ) {
				setMap(0);
				eventHandled = true;
			}
			if( key == GLFW_KEY_K ) {
				show_shadow_map = !show_shadow_map;
				eventHandled = true;
			}
			if( key == GLFW_KEY_H ) {
				render_shadows = !render_shadows;
				eventHandled = true;
			}
			if( key == GLFW_KEY_J ) {
				render_textures = !render_textures;
				eventHandled = true;
			}
			if( key == GLFW_KEY_G ) {
				god_mode = !god_mode;
				eventHandled = true;
			}

			if( key == GLFW_KEY_SPACE ) {
				int ground = getGround();
				if (jumpCount > 0) {
					jumpCount--;
					charVel.y = JUMP_FORCE;
					alSourcePlay(launcherSource);
				}
				eventHandled = true;
			}
			if( key == GLFW_KEY_LEFT_CONTROL ) {
				crouchPressed = true;
				eventHandled = true;
			}
			if( key == GLFW_KEY_W ) {
				wasdPressed[0] = true;
				eventHandled = true;
			}
			if( key == GLFW_KEY_A ) {
				wasdPressed[1] = true;
				eventHandled = true;
			}
			if( key == GLFW_KEY_S ) {
				wasdPressed[2] = true;
				eventHandled = true;
			}
			if( key == GLFW_KEY_D ) {
				wasdPressed[3] = true;
				eventHandled = true;
			}
			if( key == GLFW_KEY_LEFT_SHIFT) {
				accelPressed = true;
				eventHandled = true;
			}
		}
	}
	if( action == GLFW_RELEASE ) {
		if( key == GLFW_KEY_LEFT_CONTROL ) {
			crouchPressed = false;
			eventHandled = true;
		}
		if( key == GLFW_KEY_W ) {
			wasdPressed[0] = false;
			eventHandled = true;
		}
		if( key == GLFW_KEY_A ) {
			wasdPressed[1] = false;
			eventHandled = true;
		}
		if( key == GLFW_KEY_S ) {
			wasdPressed[2] = false;
			eventHandled = true;
		}
		if( key == GLFW_KEY_D ) {
			wasdPressed[3] = false;
			eventHandled = true;
		}
		if( key == GLFW_KEY_LEFT_SHIFT) {
			accelPressed = false;
			eventHandled = true;
		}
	}
	// Fill in with event handling code...

	return eventHandled;
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void A3::renderQuad()
{
	debug.enable();
		glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D, depthMap);
	    if (quadVAO == 0)
	    {
	        float quadVertices[] = {
	            // positions        // texture Coords
	            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
	            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
	             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	        };
	        // setup plane VAO
	        glGenVertexArrays(1, &quadVAO);
	        glGenBuffers(1, &quadVBO);
	        glBindVertexArray(quadVAO);
	        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	        glEnableVertexAttribArray(0);
	        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	        glEnableVertexAttribArray(1);
	        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	    }
	    glBindVertexArray(quadVAO);
	    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    debug.disable();
    glBindVertexArray(0);
}

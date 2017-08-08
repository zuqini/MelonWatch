#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"

#include "SceneNode.hpp"
#include "grid.hpp"
#include "ProjectileSystem.hpp"
#include "ParticleSystem.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

struct LightSource {
	glm::vec3 position;
	glm::vec3 rgbIntensity;
};

struct CompareCameraDistance {
    CompareCameraDistance(glm::vec3 cameraPos): cameraPos(cameraPos) { }
    bool operator () (glm::vec3 a, glm::vec3 b) {
		return (distance(cameraPos, a) > distance(cameraPos, b));
    }
    glm::vec3 cameraPos;
};

class A3 : public CS488Window {
public:
	A3(const std::string & luaSceneFile);
	virtual ~A3();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	//-- Virtual callback methods
	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

	//-- One time initialization methods:
	void processLuaSceneFile(const std::string & filename);
	void createShaderProgram();
	void enableVertexShaderInputSlots();
	void uploadVertexDataToVbos(const MeshConsolidator & meshConsolidator);
	void mapVboDataToVertexShaderInputLocations();
	void initViewMatrix();
	void initLightSources();
	void buildHomeBase(Grid *grid);
	void buildPlatform(Grid *grid, int x, int y, int z, int size, int block);
	void buildToDoor(Grid *grid, int x, int y, int z);
	void buildFromDoor(Grid *grid, int x, int y, int z);
	void removeEntity(std::vector<glm::vec3> &vec, int i, int j, int k);

	void initPerspectiveMatrix();
	void uploadCommonSceneUniforms();
	void renderSceneGraph(const SceneNode &node);
	void renderArcCircle();
	void setupMap();
	void setupCubemap();
	void setupParticles();
	void renderParticles();
	void renderEntities();
	void renderTransparentEntities();
	void renderCubemap();
	void resetPlayer();
	int getGround();
	int getCeiling();
	bool isCrouching();
	bool collide();
	void step();

	void renderQuad();
	void setupLight();
	void stepLight();
	GLuint setupDepthTexture();
	void renderDepthMap();
	bool isDepthMapDirty;
	GLuint depthMapFBO;
	GLuint depthMap;
	glm::vec3 light_translate;
	glm::mat4 l_perspective;
	glm::mat4 l_view;
	float l_near;
	float l_far;
	ShaderProgram m_shader_depth;
	ShaderProgram debug;

	GLuint loadCubemap(std::vector<std::string> faces);
	GLuint loadMapTexture(const char * imagepath);

	std::vector<glm::vec3> opaqueBlocks;
	std::vector<glm::vec3> transparentMapBlocks;
	std::vector<glm::vec3> enemyBlocks;
	int enemyFireCounter;
	ProjectileSystem projectileSystem;

	glm::mat4 m_perspective;
	glm::mat4 m_view;

	LightSource m_light;
	ParticleSystem particleSystem;

	//-- GL resources for mesh geometry data:
	GLuint m_particle_vao; // Vertex Array Object
	GLuint m_particle_vbo; // Vertex Buffer Object
	GLuint m_particles_pos_vbo; // Vertex Buffer Object
	GLuint m_particles_col_vbo; // Vertex Buffer Object
	float g_particule_position_size_data[4 * 100000];
	float g_particule_color_data[4 * 100000];
	ShaderProgram m_shader_particle;

	ShaderProgram m_shader_entity;
	GLuint *m_block_vao; // Vertex Array Object
	GLuint *m_block_vbo; // Vertex Buffer Object
	GLuint *m_block_uv_vbo; // Vertex Buffer Object
	GLuint atlasTextureId;
	GLuint cubemapTextureId;

	GLuint m_vbo_cubemap;
	GLuint m_vao_cubemap;
	GLint m_cubemapAttribLocation;
	ShaderProgram m_shader_cubemap;

	GLuint m_vao_meshData;
	GLuint m_vbo_vertexPositions;
	GLuint m_vbo_vertexNormals;
	GLint m_positionAttribLocation;
	GLint m_normalAttribLocation;
	ShaderProgram m_shader;

	//-- GL resources for trackball circle geometry:
	GLuint m_vbo_arcCircle;
	GLuint m_vao_arcCircle;
	GLint m_arc_positionAttribLocation;
	ShaderProgram m_shader_arcCircle;

	// BatchInfoMap is an associative container that maps a unique MeshId to a BatchInfo
	// object. Each BatchInfo object contains an index offset and the number of indices
	// required to render the mesh with identifier MeshId.
	BatchInfoMap m_batchInfoMap;

	std::string m_luaSceneFile;

	std::shared_ptr<SceneNode> m_rootNode;

	glm::vec3 cameraPos;
	glm::vec3 cameraFront;
	glm::vec3 cameraUp;

	std::vector<Grid*> maps;
	Grid* currMap;
	int currMapIndex;
	void setMap(int mapIndex);

	bool initMouse;
	bool wasdPressed[4];
	bool accelPressed;
	float accelCapacity;
	bool crouchPressed;
	bool shootingPressed;
	bool zoomPressed;
	float prevXPos;
	float prevYPos;
	float yaw;
	float pitch;
	float playerHeight;
	float fov;
	float accelFov;

	float gun_ani_counter;

	int life;
	int monstersKilled;
	int timesDied;
	int bulletsFired;
	int bulletsHit;
	int enemiesHit;
	int levelsPassed;

	int monstersKilledDisplayed;
	int timesDiedDisplayed;
	int bulletsFiredDisplayed;
	int bulletsHitDisplayed;
	int enemiesHitDisplayed;
	int levelsPassedDisplayed;

	glm::vec3 charVel;
	float recoil;
	int shootCounter;
	int snowCounter;
	float inAirDisplacement;
	float inAirVel;
	int jumpCount;

	bool isPlaying(ALuint source);
	void playFarSoundClose(ALuint source, glm::vec3 pos);
	void initSound();
	void stepSound();
	void loadWav(const char * soundPath, ALuint source, ALuint buffer);
	ALCdevice *device;
	ALCcontext *context;
	ALuint gunSource;
	ALuint shootBuffer;

	ALuint expSource;
	ALuint expBuffer;

	ALuint launcherSource;
	ALuint launcherBuffer;

	ALuint dieSource;
	ALuint dieBuffer;

	ALuint bgmSource;
	ALuint bgmBuffer;

	ALuint accelEngineSource;
	ALuint accelEngineBuffer;

	ALuint homeBgmSource;
	ALuint homeBgmBuffer;

	ALuint flameSource;
	ALuint flameBuffer;
	int flameCounter;

	ALuint enemySource;
	ALuint enemyBuffer;

	ALuint tpSource;
	ALuint tpBuffer;

	ALuint walk1Source;
	ALuint walk2Source;
	ALuint walk1Buffer;
	ALuint walk2Buffer;

	bool walkSourceSwap;
};

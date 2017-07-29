#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/MeshConsolidator.hpp"
#include <glm/glm.hpp>

class Grid
{
public:
	Grid( size_t m_dim_x, size_t m_dim_y, size_t m_dim_z );
	~Grid();
	void reset();
	size_t getDimX() const;
	size_t getDimY() const;
	size_t getDimZ() const;
	int getBlock( int x, int y, int z ) const;
	glm::vec4 getCol( int x, int y, int z ) const;
	bool isOpaque( int x, int y, int z ) const;
	void setBlock( int x, int y, int z, int block );
	void setBlock( int x, int y, int z, int block, int health );
	void hitBlock( int x, int y, int z);
	int getHealth( int x, int y, int z);
	bool isSolid( int x, int y, int z ) const;
private:
	size_t m_dim_x;
	size_t m_dim_y;
	size_t m_dim_z;
	int *m_block;
	int *m_health;
};

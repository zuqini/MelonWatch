#include <algorithm>

#include "grid.hpp"
using namespace glm;

Grid::Grid( size_t dx, size_t dy, size_t dz )
	: m_dim_x( dx ), m_dim_y( dy ), m_dim_z( dz )
{
	m_block = new int[ dx * dy * dz ];
	m_health = new int[ dx * dy * dz ];
	reset();
}

void Grid::reset()
{
	size_t sz = m_dim_x*m_dim_y*m_dim_z;
	std::fill( m_block, m_block + sz, 0 );
	std::fill( m_health, m_health + sz, -1 );
}

Grid::~Grid()
{
	delete [] m_block;
	delete [] m_health;
}

size_t Grid::getDimX() const
{
	return m_dim_x;
}

size_t Grid::getDimY() const
{
	return m_dim_y;
}

size_t Grid::getDimZ() const
{
	return m_dim_z;
}

int Grid::getBlock( int x, int y, int z ) const
{
	if ( x < 0 || x >= m_dim_x || y < 0 || y >= m_dim_y || z < 0 || z >= m_dim_z) return 0;
	return m_block[ x * m_dim_z * m_dim_y + y * m_dim_z + z ];
}

bool Grid::isSolid( int x, int y, int z ) const
{
	if ( x < 0 || x >= m_dim_x || y < 0 || y >= m_dim_y || z < 0 || z >= m_dim_z) return 0;
	int block = abs(m_block[ x * m_dim_z * m_dim_y + y * m_dim_z + z ]);
	return (block != 0 && block != 13 * 16 + 16 && block != 14 * 16 + 16);
}

glm::vec4 Grid::getCol( int x, int y, int z ) const
{
	if ( x < 0 || x >= m_dim_x || y < 0 || y >= m_dim_y || z < 0 || z >= m_dim_z) return vec4(0.0f, 0.0f, 0.0f, 1.0f);
	int block = abs(m_block[ x * m_dim_z * m_dim_y + y * m_dim_z + z ]);
	if (block == 13 * 16 + 16) {
		return vec4(0.7f, 0.7f, 0.7f, 0.3f);
	} else if (block == 14 * 16 + 16) {
		return vec4(0.7f, 0.7f, 0.7f, 0.3f);
	} else if (block == 4 * 16 + 1) {
		return vec4(1.0f, 1.0f, 1.0f, 0.1f);
	} else {
		return vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

bool Grid::isOpaque( int x, int y, int z ) const
{
	if ( x < 0 || x >= m_dim_x || y < 0 || y >= m_dim_y || z < 0 || z >= m_dim_z) return false;
	int block = abs(m_block[ x * m_dim_z * m_dim_y + y * m_dim_z + z ]);
	return (getCol(x, y, z).w == 1.0f);
}

void Grid::setBlock( int x, int y, int z, int block)
{
	if (block != 0)
		setBlock(x, y, z, block, 5);
	else
		setBlock(x, y, z, block, -1);
}

void Grid::setBlock( int x, int y, int z, int block, int health)
{
	if ( x >= 0 && x < m_dim_x && y >= 0 && y < m_dim_y && z >= 0 && z < m_dim_z) {
		m_block[ x * m_dim_z * m_dim_y + y * m_dim_z + z ] = block;
		m_health[ x * m_dim_z * m_dim_y + y * m_dim_z + z ] = health;
	}
}

void Grid::hitBlock( int x, int y, int z) {
	if ( x >= 0 && x < m_dim_x && y >= 0 && y < m_dim_y && z >= 0 && z < m_dim_z) {
		m_health[ x * m_dim_z * m_dim_y + y * m_dim_z + z ]--;
	}
}

int Grid::getHealth( int x, int y, int z) {
	if ( x < 0 || x >= m_dim_x || y < 0 || y >= m_dim_y || z < 0 || z >= m_dim_z) return -1;
	return m_health[ x * m_dim_z * m_dim_y + y * m_dim_z + z ];
}

#include "SquareTile.h"
#include <iostream>
#include <omp.h>
#include <sstream>

#include <cstdio>
#include <ctime>

using namespace honeycomb;
using namespace gl;
using namespace glm;
using namespace globjects;

SquareTile::SquareTile(Renderer* renderer) :Tile(renderer)
{
	// create shader programs
	renderer->createShaderProgram("square-acc", {
		{GL_VERTEX_SHADER,"./res/tiles/square/square-acc-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/acc-fs.glsl"}
		});

	renderer->createShaderProgram("square-tiles", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/bounding-quad-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/square/square-tiles-fs.glsl"}
		});

	renderer->createShaderProgram("square-grid", {
		{GL_VERTEX_SHADER,"./res/tiles/square/square-grid-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/square/square-grid-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/square/square-grid-fs.glsl"}
		});

	renderer->createShaderProgram("square-normals", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/bounding-quad-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/square/square-normals-fs.glsl"}
		});
}


// --------------------------------------------------------------------------------------
// ###########################   FETCH SHADER PROGRAMS ##################################
// --------------------------------------------------------------------------------------

Program * SquareTile::getAccumulationProgram()
{
	auto shaderProgram_square_acc = renderer->shaderProgram("square-acc");

	shaderProgram_square_acc->setUniform("maxBounds_Off", maxBounds_Offset);
	shaderProgram_square_acc->setUniform("minBounds_Off", minBounds_Offset);

	shaderProgram_square_acc->setUniform("maxTexCoordX", m_tileMaxX);
	shaderProgram_square_acc->setUniform("maxTexCoordY", m_tileMaxY);

	return shaderProgram_square_acc;
}

Program * SquareTile::getTileProgram()
{
	auto shaderProgram_square_tiles = renderer->shaderProgram("square-tiles");

	//geometry shader
	shaderProgram_square_tiles->setUniform("maxBounds_acc", maxBounds_Offset);
	shaderProgram_square_tiles->setUniform("minBounds_acc", minBounds_Offset);


	return shaderProgram_square_tiles;
}

Program * SquareTile::getTileNormalsProgram()
{
	auto shaderProgram_square_normals = renderer->shaderProgram("square-normals");

	//geometry shader
	shaderProgram_square_normals->setUniform("maxBounds_acc", maxBounds_Offset);
	shaderProgram_square_normals->setUniform("minBounds_acc", minBounds_Offset);


	return shaderProgram_square_normals;
}

globjects::Program * honeycomb::SquareTile::getGridProgram()
{
	
	auto shaderProgram_square_grid = renderer->shaderProgram("square-grid");

	//vertex shader
	shaderProgram_square_grid->setUniform("numCols", m_tile_cols);
	shaderProgram_square_grid->setUniform("numRows", m_tile_rows);

	//geometry shader
	shaderProgram_square_grid->setUniform("maxBounds_Off", maxBounds_Offset);
	shaderProgram_square_grid->setUniform("minBounds_Off", minBounds_Offset);

	return shaderProgram_square_grid;
}

// --------------------------------------------------------------------------------------
// ########################### TILE CALC ###############################################
// --------------------------------------------------------------------------------------


void SquareTile::calculateNumberOfTiles(vec3 boundingBoxSize, vec3 minBounds)
{

	// get maximum value of X,Y in accumulateTexture-Space
	m_tile_cols = ceil(boundingBoxSize.x / tileSizeWS);
	m_tile_rows = ceil(boundingBoxSize.y / tileSizeWS);
	numTiles = m_tile_rows * m_tile_cols;
	m_tileMaxX = m_tile_cols - 1;
	m_tileMaxY = m_tile_rows - 1;

	//The squares on the maximum sides of the bounding box, will not fit into the box perfectly most of the time
	//therefore we calculate new maximum bounds that fit them perfectly
	//this way we can perform a mapping using the set square size
	maxBounds_Offset = vec2(m_tile_cols * tileSizeWS + minBounds.x, m_tile_rows * tileSizeWS + minBounds.y);
	minBounds_Offset = minBounds;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DISCREPANCY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SquareTile::mapPointToTile1D(vec2 p) {

	// to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
	// If the current value = maxValue, we take the maxTexCoord instead
	int squareX = min(m_tileMaxX, mapInterval(p.x, minBounds_Offset[0], maxBounds_Offset[0], m_tileMaxX + 1));
	int squareY = min(m_tileMaxY, mapInterval(p.y, minBounds_Offset[1], maxBounds_Offset[1], m_tileMaxY + 1));

	// compute 1D coordinates
	return squareX + m_tile_cols * squareY;
}
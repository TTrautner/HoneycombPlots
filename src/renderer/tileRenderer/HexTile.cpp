#include "HexTile.h"
#include <iostream>
#include <omp.h>
#include <sstream>

#include <cstdio>
#include <ctime>

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

HexTile::HexTile(Renderer* renderer) :Tile(renderer)
{
	// create shader programs
	renderer->createShaderProgram("hex-acc", {
		{GL_VERTEX_SHADER,"./res/tiles/hexagon/hexagon-acc-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/acc-fs.glsl"}
		});

	renderer->createShaderProgram("hex-tiles", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/bounding-quad-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/hexagon/hexagon-tiles-fs.glsl"}
		});

	renderer->createShaderProgram("hex-grid", {
		{GL_VERTEX_SHADER,"./res/tiles/hexagon/hexagon-grid-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/hexagon/hexagon-grid-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/hexagon/hexagon-grid-fs.glsl"}
		});

	renderer->createShaderProgram("hex-normals", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/bounding-quad-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/hexagon/hexagon-normals-fs.glsl"}
		});
}


// --------------------------------------------------------------------------------------
// ###########################   FETCH SHADER PROGRAMS ##################################
// --------------------------------------------------------------------------------------

Program * HexTile::getAccumulationProgram()
{
	auto shaderProgram_hex_acc = renderer->shaderProgram("hex-acc");

	shaderProgram_hex_acc->setUniform("maxBounds_rect", maxBounds_rect);
	shaderProgram_hex_acc->setUniform("minBounds_Offset", minBounds_Offset);

	shaderProgram_hex_acc->setUniform("maxTexCoordX", m_tileMaxX);
	shaderProgram_hex_acc->setUniform("maxTexCoordY", m_tileMaxY);

	shaderProgram_hex_acc->setUniform("max_rect_col", max_rect_col);
	shaderProgram_hex_acc->setUniform("max_rect_row", max_rect_row);

	shaderProgram_hex_acc->setUniform("rectHeight", rect_height);
	shaderProgram_hex_acc->setUniform("rectWidth", rect_width);

	return shaderProgram_hex_acc;
}

Program * HexTile::getTileProgram()
{
	auto shaderProgram_hex_tiles = renderer->shaderProgram("hex-tiles");

	//geometry shader
	shaderProgram_hex_tiles->setUniform("maxBounds_acc", maxBounds_rect);
	shaderProgram_hex_tiles->setUniform("minBounds_acc", minBounds_Offset);

	//geometry & fragment shader
	shaderProgram_hex_tiles->setUniform("rectSize", vec2(rect_width, rect_height));

	//fragment Shader
	shaderProgram_hex_tiles->setUniform("max_rect_col", max_rect_col);
	shaderProgram_hex_tiles->setUniform("max_rect_row", max_rect_row);

	return shaderProgram_hex_tiles;
}

Program * HexTile::getTileNormalsProgram()
{
	auto shaderProgram_hex_normals = renderer->shaderProgram("hex-normals");

	//geometry shader
	shaderProgram_hex_normals->setUniform("maxBounds_acc", maxBounds_rect);
	shaderProgram_hex_normals->setUniform("minBounds_acc", minBounds_Offset);

	//geometry & fragment shader
	shaderProgram_hex_normals->setUniform("rectSize", vec2(rect_width, rect_height));

	//fragment Shader
	shaderProgram_hex_normals->setUniform("max_rect_col", max_rect_col);
	shaderProgram_hex_normals->setUniform("max_rect_row", max_rect_row);


	return shaderProgram_hex_normals;
}

globjects::Program * molumes::HexTile::getGridProgram()
{
	
	auto shaderProgram_hexagon_grid = renderer->shaderProgram("hex-grid");

	shaderProgram_hexagon_grid->setUniform("horizontal_space", horizontal_space);
	shaderProgram_hexagon_grid->setUniform("vertical_space", vertical_space);
	shaderProgram_hexagon_grid->setUniform("num_cols", m_tile_cols);
	shaderProgram_hexagon_grid->setUniform("minBounds_Off", minBounds_Offset);

	return shaderProgram_hexagon_grid;
}

// --------------------------------------------------------------------------------------
// ########################### TILE CALC ###############################################
// --------------------------------------------------------------------------------------

void HexTile::calculateNumberOfTiles(vec3 boundingBoxSize, vec3 minBounds)
{

	//hex size is defined as the lenght from its middle to a corner (so half of its bounding box size)
	//to account for this we use half the size for computation to keep the size consistent with other tile formats
	tileSizeWS /= 2.0f;

	// calculations derived from: https://www.redblobgames.com/grids/hexagons/
	// we assume flat topped hexagons
	// we use "Offset Coordinates"
	horizontal_space = tileSizeWS * 1.5f;
	vertical_space = sqrt(3)*tileSizeWS;

	// the rectangles used to map points to hexagons are
	// half as high as the hexagon
	rect_height = vertical_space / 2.0f;
	// half as wide as the hexagon (tileSizeWS is half the width of the whole hexagon)
	rect_width = tileSizeWS;

	//number of hex columns
	//+1 because else the floor operation could return 0 
	float cols_tmp = 1 + (boundingBoxSize.x / horizontal_space);
	m_tile_cols = floor(cols_tmp);
	if ((cols_tmp - m_tile_cols) * horizontal_space >= tileSizeWS) {
		m_tile_cols += 1;
	}

	//number of hex rows
	float rows_tmp = 1 + (boundingBoxSize.y / vertical_space);
	m_tile_rows = floor(rows_tmp);
	if ((rows_tmp - m_tile_rows) * vertical_space >= vertical_space / 2) {
		m_tile_rows += 1;
	}

	//max index of rect columns
	if (m_tile_cols % 2 == 1) {
		max_rect_col = ceil(m_tile_cols*1.5f) - 1;
	}
	else {
		max_rect_col = ceil(m_tile_cols*1.5f);
	}

	//max index of rect rows
	max_rect_row = m_tile_rows * 2;

	//max indices of hexagon grid
	m_tileMaxX = m_tile_cols - 1;
	m_tileMaxY = m_tile_rows - 1;
	numTiles = m_tile_rows * m_tile_cols;

	//bounding box offsets
	minBounds_Offset = vec2(minBounds.x - tileSizeWS / 2.0f, minBounds.y - vertical_space / 2.0f);
	maxBounds_Offset = vec2(m_tile_cols * horizontal_space + minBounds_Offset.x + tileSizeWS / 2.0f, m_tile_rows * vertical_space + minBounds_Offset.y + vertical_space / 2.0f);

	//bounding box of rectangle (min is same as minBounds_Offset)
	maxBounds_rect = vec2((max_rect_col + 1) * rect_width + minBounds_Offset.x, (max_rect_row + 1)*rect_height + minBounds_Offset.y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DISCREPANCY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool molumes::HexTile::pointLeftOfLine(vec2 a, vec2 b, vec2 p) {
	return ((p.x - a.x)*(b.y - a.y) - (p.y - a.y)*(b.x - a.x)) < 0;
}

int molumes::HexTile::mapPointToTile1D(vec2 p)
{
	// to get intervals from 0 to maxCoord, we map the original Point interval to maxCoord+1
	// If the current value = maxValue, we take the maxCoord instead
	int rectX = min(max_rect_col, mapInterval(p.x, minBounds_Offset.x, maxBounds_rect.x, max_rect_col + 1));
	int rectY = min(max_rect_row, mapInterval(p.y, minBounds_Offset.y, maxBounds_rect.y, max_rect_row + 1));

	// rectangle left lower corner in space of points
	vec2 ll = vec2(rectX * rect_width + minBounds_Offset.x, rectY *rect_height + minBounds_Offset.y);
	vec2 a, b;

	// calculate hexagon index from rectangle index
	int hexX, hexY, modX, modY;

	// get modulo values
	modX = rectX % 3;
	modY = rectY % 2;

	//calculate X index
	hexX = int(rectX / 3) * 2 + modX - 1;
	if (modX != 2) {
		if (modX == 0) {
			if (modY == 0) {
				//Upper Left
				a = vec2(ll.x + rect_width / 2.0f, ll.y + rect_height);
				b = ll;
				if (pointLeftOfLine(a, b, p)) {
					hexX++;
				}
			}
			//modY = 1
			else {
				//Lower Left
				a = vec2(ll.x, ll.y + rect_height);
				b = vec2(ll.x + rect_width / 2.0f, ll.y);
				if (pointLeftOfLine(a, b, p)) {
					hexX++;
				}
			}
		}
		//modX = 1
		else {

			if (modY == 0) {
				//Upper Right
				a = vec2(ll.x + rect_width / 2.0f, ll.y + rect_height);
				b = vec2(ll.x + rect_width, ll.y);
				if (pointLeftOfLine(a, b, p)) {
					hexX++;
				}
			}
			//modY = 1
			else {
				//Lower Right
				a = vec2(ll.x + rect_width, ll.y + rect_height);
				b = vec2(ll.x + rect_width / 2.0f, ll.y);
				if (pointLeftOfLine(a, b, p)) {
					hexX++;
				}
			}
		}
	}

	// Y Coordinate
	hexY = int((rectY - (1 - (hexX % 2))) / 2);

	// calculate 1D coordinates
	return hexX + m_tile_cols * hexY;
}
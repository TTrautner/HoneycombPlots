#pragma once
#include "Tile.h"
#include <memory>

#include <glm/glm.hpp>

namespace molumes
{
	class HexTile : public Tile
	{
	public:
		HexTile(Renderer* renderer);

		// FETCH SHADER PROGRAMS -------------------------------------------------------------------
		// get correct shader program and sets tile specific uniforms
		// uniforms that are shared by all types of tiles are set in TileRenderer
		globjects::Program * getAccumulationProgram() override;
		globjects::Program * getTileProgram() override;
		globjects::Program * getTileNormalsProgram() override;
		globjects::Program * getGridProgram() override;

		// TILE CALC -------------------------------------------------------------------
		// calculates the number of tiles (rows and columns) that are needed to cover the data
		// with the given tileSise
		// also computes the bounding box of the resulting tile grid
		void calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds) override;
		
		// DISCREPANCY -------------------------------------------------------------------
		// maps a datapoint to its tile
		// returns 1D tile coordinates, assuming the tiles are saved in an 1D array one row after the other
		int mapPointToTile1D(glm::vec2 p) override;

	private:

		// DISCREPANCY -------------------------------------------------------------------
		// check if a points is left of the given line
		bool pointLeftOfLine(glm::vec2 a, glm::vec2 b, glm::vec2 p);

		// TILE CALC VARIABLES -------------------------------------------------------------------
		// horizontal space of hexagon tile = 1.5 * tileSize
		float horizontal_space = 0.0f;
		// vertical space of hexagon tile = sqrt(3) * tileSite
		float vertical_space = 0.0f;
		// width of the rectangle
		float rect_width = 0.0f;
		// height of the rectangle
		float rect_height = 0.0f;
		// maximum Y coordinate of rectangle tiles
		int max_rect_row = 0;
		// maximum X coordinate of rectangle tiles
		int max_rect_col = 0;
		// maximum bounds of recangle grid
		glm::vec2 maxBounds_rect;

	};

}


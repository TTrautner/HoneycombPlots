#pragma once
#include "Tile.h"
#include <memory>

#include <glm/glm.hpp>

namespace molumes
{
	class SquareTile : public Tile
	{
	public:
		SquareTile(Renderer* renderer);

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

	};

}


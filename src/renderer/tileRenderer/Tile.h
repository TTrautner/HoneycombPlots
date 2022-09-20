#pragma once
#include "../Renderer.h"
#include <memory>

#include <glm/glm.hpp>

namespace honeycomb
{
	class Renderer;

	//IS USED AS ABSTRACT CLASS!!
	//DOES NOT CONTAIN ANY IMPLEMENTATIONS
	class Tile
	{
	public:
		Tile(Renderer* renderer);

		// FETCH SHADER PROGRAMS -------------------------------------------------------------------
		// get correct shader program and sets tile specific uniforms
		// uniforms that are shared by all types of tiles are set in TileRenderer
		virtual globjects::Program * getAccumulationProgram();
		virtual globjects::Program * getTileProgram();
		virtual globjects::Program * getTileNormalsProgram();
		virtual globjects::Program * getGridProgram();

		// TILE CALC -------------------------------------------------------------------
		// calculates the number of tiles (rows and columns) that are needed to cover the data
		// with the given tileSise
		// also computes the bounding box of the resulting tile grid
		virtual void calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds);
		
		// DISCREPANCY -------------------------------------------------------------------
		// maps a datapoint to its tile
		// returns 1D tile coordinates, assuming the tiles are saved in an 1D array one row after the other
		virtual int mapPointToTile1D(glm::vec2 p);

		// TILE CALC VARIABLES -------------------------------------------------------------------
		// divisor of tile size that is set by the user when calculation tileSizeWS
		const float tileSizeDiv = 500.0f;
		// tile size in world space
		float tileSizeWS = 0.0f;

		// number of tile rows
		int m_tile_rows = 0; //Y
		// number of tile columns
		int m_tile_cols = 0; //X
		// number of tiles = rows*columns
		int numTiles = 0;
		// maximum Y coordinate of tiles = m_tile_rows - 1
		int m_tileMaxY = 0;
		// maximum X coordinate of tiles = m_tile_cols - 1
		int m_tileMaxX = 0;

		// used when accumulating floats into a buffer using atomicAdd
		const float bufferAccumulationFactor = 100.0f;

		// maximum bounds of the tile grid
		glm::vec2 maxBounds_Offset;
		// minimum bounds of the tile grid
		glm::vec2 minBounds_Offset;

	protected:

		//maps value x from [a,b] --> [0,c]
		int mapInterval(float x, float a, float b, int c);

		// RENDERER REFERENCE -------------------------------------------------------------
		Renderer* renderer;

	private:
	};

}


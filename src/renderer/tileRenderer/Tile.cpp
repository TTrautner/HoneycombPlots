#include "Tile.h"

using namespace molumes;

//IS USED AS ABSTRACT CLASS!!
//DOES NOT CONTAIN ANY IMPLEMENTATIONS
molumes::Tile::Tile(Renderer* renderer)
{
	this->renderer = renderer;
}

globjects::Program * molumes::Tile::getAccumulationProgram()
{
	return nullptr;
}

globjects::Program * molumes::Tile::getTileProgram()
{
	return nullptr;
}

globjects::Program * molumes::Tile::getTileNormalsProgram()
{
	return nullptr;
}

globjects::Program * molumes::Tile::getGridProgram()
{
	return nullptr;
}

void molumes::Tile::calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds)
{
}

int molumes::Tile::mapPointToTile1D(glm::vec2 p) 
{
	return 0;
}

//maps value x from [a,b] --> [0,c]
int Tile::mapInterval(float x, float a, float b, int c)
{
	return int((x - a)*c / (b - a));
}


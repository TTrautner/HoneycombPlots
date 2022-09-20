#include "Tile.h"

using namespace honeycomb;

//IS USED AS ABSTRACT CLASS!!
//DOES NOT CONTAIN ANY IMPLEMENTATIONS
honeycomb::Tile::Tile(Renderer* renderer)
{
	this->renderer = renderer;
}

globjects::Program * honeycomb::Tile::getAccumulationProgram()
{
	return nullptr;
}

globjects::Program * honeycomb::Tile::getTileProgram()
{
	return nullptr;
}

globjects::Program * honeycomb::Tile::getTileNormalsProgram()
{
	return nullptr;
}

globjects::Program * honeycomb::Tile::getGridProgram()
{
	return nullptr;
}

void honeycomb::Tile::calculateNumberOfTiles(glm::vec3 boundingBoxSize, glm::vec3 minBounds)
{
}

int honeycomb::Tile::mapPointToTile1D(glm::vec2 p) 
{
	return 0;
}

//maps value x from [a,b] --> [0,c]
int Tile::mapInterval(float x, float a, float b, int c)
{
	return int((x - a)*c / (b - a));
}


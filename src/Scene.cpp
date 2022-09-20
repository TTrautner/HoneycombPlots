#include "Scene.h"
#include <iostream>

using namespace honeycomb;

Scene::Scene()
{
	m_table = std::make_unique<Table>();
}

Table * Scene::table()
{
	return m_table.get();
}
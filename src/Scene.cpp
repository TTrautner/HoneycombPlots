#include "Scene.h"
#include <iostream>

using namespace molumes;

Scene::Scene()
{
	m_table = std::make_unique<Table>();
}

Table * Scene::table()
{
	return m_table.get();
}
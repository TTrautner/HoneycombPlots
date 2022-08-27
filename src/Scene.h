#pragma once

#include <memory>
#include "CSV/Table.h"

namespace molumes
{
	class Table;

	class Scene
	{
	public:
		Scene();
		Table* table();

	private:
		std::unique_ptr<Table> m_table;
	};


}
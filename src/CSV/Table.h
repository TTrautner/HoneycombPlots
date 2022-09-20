#pragma once

#include <vector>
#include <glm/glm.hpp>

// header to process CSV
#include "rapidcsv.h"


namespace honeycomb
{
	class Table
	{

	public:

		Table();
		Table(const std::string& filename);
		void load(const std::string& filename);

		// file properties
		const std::string & filename() const;
		const std::vector<std::string> getColumnNames();

		// handling buffers (IDs depend on column namens accessed using the GUI)
		const void updateBuffers(int xID, int yID, int radiusID, int colorID);

		// access complete currently active table
		const std::vector < std::vector<glm::vec4> > & activeTableData() const;

		// access individual currently active data columns
		const std::vector<float> & activeXColumn() const;
		const std::vector<float> & activeYColumn() const;
		const std::vector<float> & activeRadiusColumn() const;
		const std::vector<float> & activeColorColumn() const;

		// access current bounding volume
		glm::vec3 minimumBounds() const;
		glm::vec3 maximumBounds() const;

	private:

		std::string m_filename;

		// CSV file
		rapidcsv::Document m_csvDocument;

		// stored names of column-headers
		std::vector<std::string> m_columnNames;

		// complete table stored as vector of row vectors
		std::vector<std::vector<float>> m_tableData;

		// currently active table
		std::vector<std::vector<glm::vec4> > m_activeTableData;

		// current CSV data vectors that are used to fill VBOs
		std::vector<float> m_activeXColumn;
		std::vector<float> m_activeYColumn;
		std::vector<float> m_activeRadiusColumn;
		std::vector<float> m_activeColorColumn;

		// bounding box of current selection
		glm::vec3 m_minimumBounds = glm::vec3(0.0);
		glm::vec3 m_maximumBounds = glm::vec3(0.0);
		
	};
}
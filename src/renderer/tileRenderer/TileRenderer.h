#pragma once
#include "../Renderer.h"
#include "Tile.h"
#include "SquareTile.h"
#include "HexTile.h"
#include <memory>

#include <glm/glm.hpp>
#include <glbinding/gl/gl.h> 
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>

#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/Framebuffer.h>
#include <globjects/Renderbuffer.h>
#include <globjects/Texture.h>
#include <globjects/base/File.h>
#include <globjects/TextureHandle.h>
#include <globjects/NamedString.h>
#include <globjects/base/StaticStringSource.h>
#include <globjects/Query.h>

namespace molumes
{
	class Viewer;

	class TileRenderer : public Renderer
	{
	public:
		TileRenderer(Viewer *viewer);
		~TileRenderer();
		virtual void display();

	private:

		Tile* tile = nullptr;
		std::unordered_map<std::string, Tile*> tile_processors;

		// INITIAL POINT DATA ----------------------------------------------------------------------
		std::unique_ptr<globjects::VertexArray> m_vao = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_xColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_yColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_radiusColumnBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_colorColumnBuffer = std::make_unique<globjects::Buffer>();


		// TILES GRID VERTEX DATA------------------------------------------------------------
		std::unique_ptr<globjects::VertexArray> m_vaoTiles = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_verticesTiles = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_discrepanciesBuffer = std::make_unique<globjects::Buffer>();


		// QUAD VERTEX DATA -------------------------------------------------------------------------------
		std::unique_ptr<globjects::VertexArray> m_vaoQuad = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_verticesQuad = std::make_unique<globjects::Buffer>();


		// SHADER GLOBALS / DEFINES -------------------------------------------------------------------------------
		std::unique_ptr<globjects::StaticStringSource> m_shaderSourceDefines = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderDefines = nullptr;

		std::unique_ptr<globjects::File> m_shaderSourceGlobals = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderGlobals = nullptr;

		std::unique_ptr<globjects::File> m_shaderSourceGlobalsHexagon = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderGlobalsHexagon = nullptr;

		// TEXTURES -------------------------------------------------------------------------
		std::unique_ptr<globjects::Texture> m_depthTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_colorTexture = nullptr;

		std::unique_ptr<globjects::Texture> m_pointChartTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_pointCircleTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_tilesDiscrepanciesTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_tileAccumulateTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_tilesTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_normalsAndDepthTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_gridTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_kdeTexture = nullptr;

		int m_ColorMapWidth = 0;
		std::unique_ptr<globjects::Texture> m_colorMapTexture = nullptr;

		std::unique_ptr<globjects::Texture> m_tileTextureArray = nullptr;

		//---------------------------------------------------------------------------------------

		//SSBO
		std::unique_ptr<globjects::Buffer> m_valueMaxBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_tileNormalsBuffer = std::make_unique<globjects::Buffer>();

		// FRAMEBUFFER -------------------------------------------------------------------------
		std::unique_ptr<globjects::Framebuffer> m_pointFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_pointCircleFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_hexFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_tilesDiscrepanciesFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_tileAccumulateFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_tilesFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_gridFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_shadeFramebuffer = nullptr;

		glm::ivec2 m_framebufferSize;


		// LIGHTING -----------------------------------------------------------------------
		glm::vec3 ambientMaterial;
		glm::vec3 diffuseMaterial;
		glm::vec3 specularMaterial;
		float shininess;


		// TILES CALC----------------------------------------------------------------
		void calculateTileTextureSize(const glm::mat4 inverseModelViewProjectionMatrix);


		// GUI ----------------------------------------------------------------------------

		void renderGUI();
		void setShaderDefines();

		// items for ImGui Combo
		std::string m_guiFileNames = { 'N', 'o', 'n', 'e', '\0' };
		std::vector<std::string>  m_fileNames = { "None" };

		std::string m_guiColumnNames = { 'N', 'o', 'n', 'e', '\0' };

		// store combo ID of selected file
		int m_fileDataID = 0;
		int m_oldFileDataID = 0;

		// store combo ID of selected columns
		int m_xAxisDataID = 0, m_yAxisDataID = 0, m_radiusDataID = 0, m_colorDataID = 0;

		// selection of color maps
		int m_colorMap = 12;						// use "virdis" as default heatmap
		bool m_colorMapLoaded = false;
		// ---------------------------------
		int m_oldColorMap = 0;

		float m_aaoScaling = 2.5f;

		// Tiles Parameters
		// [none=0, square=1, hexagon=2]
		int m_selected_tile_style = 2;
		int m_selected_tile_style_tmp = m_selected_tile_style;

		// tileSize adjustable by user
		float m_tileSize = 20.0f;
		float m_tileSize_tmp = m_tileSize;
		// gridWidth adjustable by user
		float m_gridWidth = 1.75f;

		//point circle parameters 
		// circle radius adjustable by user
		float m_pointCircleRadius = 6.5f;
		// kde radius adjustable by user
		float m_kdeRadius = 65.0f;
		// divisor of radius that is set by the user when calculation radius in WorldSpace
		const float pointCircleRadiusDiv = 5000.0f;

		//discrepancy parameters
		// divisor of discrpenacy values adjustable by user
		float m_discrepancyDiv = 1.0f;
		// ease function modyfier adjustable by user
		float m_discrepancy_easeIn = 1.0f;
		float m_discrepancy_easeIn_tmp = m_discrepancy_easeIn;
		// adjustment rate for tiles with few points adjustable by user
		float m_discrepancy_lowCount = 0.0f;
		float m_discrepancy_lowCount_tmp = m_discrepancy_lowCount;

		//regression parameters
		// sigma of gauss kernel for KDE adjustable by user
		float m_sigma = 1.5f;
		// divisor of radius that is set by the user when calculation radius in WorldSpace
		const float gaussSampleRadiusMult = 400.0f;
		// multiplyer for KDE height field adjustable by user
		float m_densityMult = 20.0f;
		// multiplyer for pyramid height adjustable by user
		float m_tileHeightMult = 10.0f;
		// defines how much of pyramid border is shown = moves regression plane up and down inside pyramid adjustable by user
		float m_borderWidth = 0.2f; // width in percent to size
		// render pyramid yes,no adjustable by user
		bool m_showBorder = true;
		// invert pyramid adjustable by user
		bool m_invertPyramid = false;
		// blend range for anti-aliasing of pyramid edges
		float blendRange = 1.0f;

		// GUI parameters for diamond cut glyp width
		float m_diamondCutOutlineWidth = 2.5f;
		float m_diamondCutOutlineDarkening = 0.65f;

		// textured tiles default values
		glm::uint m_tileTextureWidth = 300;
		glm::uint  m_tileTextureHeight = 300;
		int m_numberTextureTiles = 99;			// total number of tile we want to load

		// define booleans
		bool m_renderPointCircles = true;
		bool m_renderDiscrepancy = false;
		bool m_renderDiscrepancy_tmp = m_renderDiscrepancy;
		bool m_renderGrid = true;
		bool m_renderKDE = false;
		bool m_renderTileNormals = true;
		bool m_renderNormalBuffer = false;
		bool m_renderDepthBuffer = false;
		bool m_renderAnalyticalAO = true;
		bool m_renderMomochromeTiles = false;
		bool m_tileTexturing = false;
		bool m_showDiamondCutOutline = true;

		// ------------------------------------------------------------------------------------------

		// INTERVAL MAPPING--------------------------------------------------------------------------
		//maps value x from [a,b] --> [0,c]
		float mapInterval(float x, float a, float b, int c);

		// DISCREPANCY------------------------------------------------------------------------------

		std::vector<float> calculateDiscrepancy2D(const std::vector<float>& samplesX, const std::vector<float>& samplesY, glm::vec3 maxBounds, glm::vec3 minBounds);
	};

}


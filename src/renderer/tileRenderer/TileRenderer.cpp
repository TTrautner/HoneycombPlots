#include "TileRenderer.h"
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <iostream>
#include <filesystem>
#include <imgui.h>
#include <omp.h>
#include "../../Viewer.h"
#include "../../Scene.h"
#include "../../CSV/Table.h"
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <ctime>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace honeycomb;
using namespace gl;
using namespace glm;
using namespace globjects;

TileRenderer::TileRenderer(Viewer* viewer) : Renderer(viewer)
{
	// initialise tile objects
	tile_processors["square"] = new SquareTile(this);
	tile_processors["hexagon"] = new HexTile(this);

	Shader::hintIncludeImplementation(Shader::IncludeImplementation::Fallback);

	m_verticesQuad->setStorage(std::array<vec3, 1>({ vec3(0.0f, 0.0f, 0.0f) }), gl::GL_NONE_BIT);
	auto vertexBindingQuad = m_vaoQuad->binding(0);
	vertexBindingQuad->setBuffer(m_verticesQuad.get(), 0, sizeof(vec3));
	vertexBindingQuad->setFormat(3, GL_FLOAT);
	m_vaoQuad->enable(0);
	m_vaoQuad->unbind();

	// shader storage buffer object for current maximum accumulated value and maximum alpha value of point circle blending
	m_valueMaxBuffer->setStorage(sizeof(uint) * 2, nullptr, gl::GL_NONE_BIT);

	m_shaderSourceDefines = StaticStringSource::create("");
	m_shaderDefines = NamedString::create("/defines.glsl", m_shaderSourceDefines.get());

	m_shaderSourceGlobals = File::create("./res/tiles/globals.glsl");
	m_shaderGlobals = NamedString::create("/globals.glsl", m_shaderSourceGlobals.get());

	m_shaderSourceGlobalsHexagon = File::create("./res/tiles/hexagon/globals.glsl");
	m_shaderGlobalsHexagon = NamedString::create("/hex/globals.glsl", m_shaderSourceGlobalsHexagon.get());

	// create shader programs
	createShaderProgram("points", {
		{GL_VERTEX_SHADER,"./res/tiles/point-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/point-fs.glsl"}
		});

	createShaderProgram("point-circle", {
		{GL_VERTEX_SHADER,"./res/tiles/point-circle-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/point-circle-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/point-circle-fs.glsl"}
		});

	createShaderProgram("tiles-disc", {
		{GL_VERTEX_SHADER,"./res/tiles/discrepancy-vs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/discrepancy-fs.glsl"}
		});

	createShaderProgram("max-val", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/bounding-quad-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/max-val-fs.glsl"}
		});

	createShaderProgram("shade", {
		{GL_VERTEX_SHADER,"./res/tiles/image-vs.glsl"},
		{GL_GEOMETRY_SHADER,"./res/tiles/image-gs.glsl"},
		{GL_FRAGMENT_SHADER,"./res/tiles/shade-fs.glsl"}
		});

	m_framebufferSize = viewer->viewportSize();

	// init textures
	m_pointChartTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_pointCircleTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// the size of this textures is set dynamicly depending on the tile grid resolution
	m_tilesDiscrepanciesTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, ivec2(1, 1), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	m_tileAccumulateTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, ivec2(1, 1), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_tilesTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	m_normalsAndDepthTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_gridTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_kdeTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_colorTexture = create2DTexture(GL_TEXTURE_2D, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	//colorMap - 1D Texture
	m_colorMapTexture = Texture::create(GL_TEXTURE_1D);
	m_colorMapTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_colorMapTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_colorMapTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_colorMapTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_colorMapTexture->generateMipmap();


	for (auto& d : std::filesystem::directory_iterator("./dat"))
	{
		std::filesystem::path csvPath(d);

		if (csvPath.extension().string() == ".csv")
		{
			// log CSV files that were found in the "./dat" folder
			globjects::debug() << "Found CSV file: " << csvPath.string() << " ...";
			std::string filename = csvPath.filename().string();

			// update collections containing all current CSV files
			m_guiFileNames += filename + '\0';
			m_fileNames.push_back(filename);
		}
	}

	// create Framebuffer
	m_pointFramebuffer = Framebuffer::create();
	m_pointFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_pointChartTexture.get());
	m_pointFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_pointFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_pointCircleFramebuffer = Framebuffer::create();
	m_pointCircleFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_pointCircleTexture.get());
	m_pointCircleFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_kdeTexture.get());
	m_pointCircleFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_pointCircleFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

	m_tilesDiscrepanciesFramebuffer = Framebuffer::create();
	m_tilesDiscrepanciesFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_tilesDiscrepanciesTexture.get());
	m_tilesDiscrepanciesFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_tilesDiscrepanciesFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_tileAccumulateFramebuffer = Framebuffer::create();
	m_tileAccumulateFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_tileAccumulateTexture.get());
	m_tileAccumulateFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_tileAccumulateFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_tilesFramebuffer = Framebuffer::create();
	m_tilesFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_tilesTexture.get());
	m_tilesFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_normalsAndDepthTexture.get());
	m_tilesFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_tilesFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

	m_gridFramebuffer = Framebuffer::create();
	m_gridFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_gridTexture.get());
	m_gridFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_gridFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_shadeFramebuffer = Framebuffer::create();
	m_shadeFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_colorTexture.get());
	m_shadeFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });
	m_shadeFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
}

//Destructor
honeycomb::TileRenderer::~TileRenderer()
{
	if (tile_processors.count("square") > 0) {
		delete tile_processors["square"];
	}
	if (tile_processors.count("hexagon") > 0) {
		delete tile_processors["hexagon"];
	}
}


void TileRenderer::display()
{
	auto currentState = State::currentState();

	// Scatterplot GUI -----------------------------------------------------------------------------------------------------------
	// needs to be first to set all the new values before rendering
	renderGUI();

	setShaderDefines();
	// ---------------------------------------------------------------------------------------------------------------------------

	// do not render if either the dataset was not loaded or the window is minimized 
	if (viewer()->scene()->table()->activeTableData().size() == 0 || viewer()->viewportSize().x == 0 || viewer()->viewportSize().y == 0)
	{
		return;
	}

	// number of datapoints
	int vertexCount = int(viewer()->scene()->table()->activeTableData()[0].size());

	// retrieve/compute all necessary matrices and related properties
	const mat4 viewMatrix = viewer()->viewTransform();
	const mat4 inverseViewMatrix = inverse(viewMatrix);
	const mat4 modelViewMatrix = viewer()->modelViewTransform();
	const mat4 inverseModelViewMatrix = inverse(modelViewMatrix);
	const mat4 modelLightMatrix = viewer()->modelLightTransform();
	const mat4 inverseModelLightMatrix = inverse(modelLightMatrix);
	const mat4 modelViewProjectionMatrix = viewer()->modelViewProjectionTransform();
	const mat4 inverseModelViewProjectionMatrix = inverse(modelViewProjectionMatrix);
	const mat4 projectionMatrix = viewer()->projectionTransform();
	const mat4 inverseProjectionMatrix = inverse(projectionMatrix);
	const mat3 normalMatrix = mat3(transpose(inverseModelViewMatrix));
	const mat3 inverseNormalMatrix = inverse(normalMatrix);
	const ivec2 viewportSize = viewer()->viewportSize();
	const vec2 maxBounds = viewer()->scene()->table()->maximumBounds();
	const vec2 minBounds = viewer()->scene()->table()->minimumBounds();
	const vec4 backgroundColor = vec4(viewer()->backgroundColor(), 0);

	// transform light position, since we evaluate the illumination model in view space
	vec4 viewLightPosition = viewMatrix * viewer()->lightTransform() * vec4(0.0f, 0.0f, 0.0f, 1.0f);

	// if any of these values changed we need to:
	// recompute the resolution and positioning of the current tile grid
	// reset the texture resolutions
	// recompute the discrepancy
	if (m_selected_tile_style != m_selected_tile_style_tmp || m_tileSize != m_tileSize_tmp || m_renderDiscrepancy != m_renderDiscrepancy_tmp
		|| m_discrepancy_easeIn != m_discrepancy_easeIn_tmp || m_discrepancy_lowCount != m_discrepancy_lowCount_tmp || viewer()->viewportSize() != m_framebufferSize) {

		// set new values
		m_selected_tile_style = m_selected_tile_style_tmp;
		m_tileSize = m_tileSize_tmp;
		m_renderDiscrepancy = m_renderDiscrepancy_tmp;
		m_discrepancy_easeIn = m_discrepancy_easeIn_tmp;
		m_discrepancy_lowCount = m_discrepancy_lowCount_tmp;

		//set tile processor
		switch (m_selected_tile_style) {
		case 1:
			tile = tile_processors["square"];
			break;
		case 2:
			tile = tile_processors["hexagon"];
			break;
		default:
			tile = nullptr;
			break;
		}

		if (viewer()->viewportSize() != m_framebufferSize)
		{
			m_framebufferSize = viewportSize;
			m_pointChartTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_pointCircleTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_tilesTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_normalsAndDepthTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_gridTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_kdeTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}

		calculateTileTextureSize(inverseModelViewProjectionMatrix);
	}

	double mouseX, mouseY;
	glfwGetCursorPos(viewer()->window(), &mouseX, &mouseY);
	vec2 focusPosition = vec2(2.0f*float(mouseX) / float(viewportSize.x) - 1.0f, -2.0f*float(mouseY) / float(viewportSize.y) + 1.0f);

	vec4 projectionInfo(float(-2.0 / (viewportSize.x * projectionMatrix[0][0])),
		float(-2.0 / (viewportSize.y * projectionMatrix[1][1])),
		float((1.0 - (double)projectionMatrix[0][2]) / projectionMatrix[0][0]),
		float((1.0 + (double)projectionMatrix[1][2]) / projectionMatrix[1][1]));

	float projectionScale = float(viewportSize.y) / fabs(2.0f / projectionMatrix[1][1]);

	//LIGHTING -------------------------------------------------------------------------------------------------------------------
	// adapted version of "pearl" from teapots.c File Reference
	// Mark J. Kilgard, Silicon Graphics, Inc. 1994 - http://web.archive.org/web/20100725103839/http://www.cs.utk.edu/~kuck/materials_ogl.htm
	ambientMaterial = vec3(0.20725f, 0.20725f, 0.20725f);
	diffuseMaterial = vec3(1, 1, 1);// 0.829f, 0.829f, 0.829f);
	specularMaterial = vec3(0.296648f, 0.296648f, 0.296648f);
	shininess = 11.264f;
	//----------------------------------------------------------------------------------------------------------------------------

	vec4 nearPlane = inverseProjectionMatrix * vec4(0.0, 0.0, -1.0, 1.0);
	nearPlane /= nearPlane.w;

	// ====================================================================================== POINTS RENDER PASS =======================================================================================
	// renders data set as point primitives
	// ONLY USED TO SHOW INITIAL POINTS

	if (m_selected_tile_style == 0 && !m_renderPointCircles) {

		m_pointFramebuffer->bind();
		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		// allow blending for the classical point chart color-attachment (0) of the point frame-buffer
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationi(0, GL_MAX);

		// -------------------------------------------------------------------------------------------------

		auto shaderProgram_points = shaderProgram("points");

		// vertex shader
		shaderProgram_points->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

		m_vao->bind();
		shaderProgram_points->use();

		m_vao->drawArrays(GL_POINTS, 0, vertexCount);

		shaderProgram_points->release();
		m_vao->unbind();

		// disable blending for draw buffer 0 (classical scatter plot)
		glDisablei(GL_BLEND, 0);

		m_pointFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
	// ====================================================================================== DISCREPANCIES RENDER PASS ======================================================================================
	// write tile discrepancies into texture

	if (m_renderDiscrepancy && tile != nullptr) {
		m_tilesDiscrepanciesFramebuffer->bind();

		// set viewport to size of accumulation texture
		glViewport(0, 0, tile->m_tile_cols, tile->m_tile_rows);

		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		// allow blending for the classical point chart color-attachment (0) of the point frame-buffer
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationi(0, GL_FUNC_ADD);

		// -------------------------------------------------------------------------------------------------

		auto shaderProgram_discrepancies = shaderProgram("tiles-disc");

		//vertex shader
		shaderProgram_discrepancies->setUniform("numCols", tile->m_tile_cols);
		shaderProgram_discrepancies->setUniform("numRows", tile->m_tile_rows);
		shaderProgram_discrepancies->setUniform("discrepancyDiv", m_discrepancyDiv);

		m_vaoTiles->bind();
		shaderProgram_discrepancies->use();

		m_vaoTiles->drawArrays(GL_POINTS, 0, tile->numTiles);

		shaderProgram_discrepancies->release();
		m_vaoTiles->unbind();

		// disable blending
		glDisablei(GL_BLEND, 0);

		//reset Viewport
		glViewport(0, 0, viewer()->viewportSize().x, viewer()->viewportSize().y);

		m_tilesDiscrepanciesFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// ====================================================================================== POINT CIRCLES  & KDE RENDER PASS =======================================================================================
	// render Point Circles into texture0
	// render Kernel Density Estimation into texture1

	if (m_renderPointCircles || m_renderKDE || m_renderTileNormals) {
		m_pointCircleFramebuffer->bind();
		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		// additive blending
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_ONE, GL_ONE);
		glBlendEquationi(0, GL_FUNC_ADD);
		glEnablei(GL_BLEND, 1);
		glBlendFunci(1, GL_ONE, GL_ONE);
		glBlendEquationi(1, GL_FUNC_ADD);
		// -------------------------------------------------------------------------------------------------

		auto shaderProgram_pointCircles = shaderProgram("point-circle");

		//set correct radius
		float scaleAdjustedPointCircleRadius = m_pointCircleRadius / pointCircleRadiusDiv * viewer()->scaleFactor();
		float scaleAdjustedKDERadius = m_kdeRadius / pointCircleRadiusDiv * viewer()->scaleFactor();
		float scaleAdjustedRadiusMult = gaussSampleRadiusMult / viewer()->scaleFactor();

		//geometry shader
		shaderProgram_pointCircles->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
		shaderProgram_pointCircles->setUniform("aspectRatio", viewer()->m_windowHeight / viewer()->m_windowWidth);

		//geometry & fragment shader
		shaderProgram_pointCircles->setUniform("pointCircleRadius", scaleAdjustedPointCircleRadius);
		shaderProgram_pointCircles->setUniform("kdeRadius", scaleAdjustedKDERadius);

		//fragment shader
		shaderProgram_pointCircles->setUniform("pointColor", viewer()->samplePointColor());
		shaderProgram_pointCircles->setUniform("sigma2", m_sigma);
		shaderProgram_pointCircles->setUniform("radiusMult", scaleAdjustedRadiusMult);
		shaderProgram_pointCircles->setUniform("densityMult", m_densityMult);

		m_vao->bind();
		shaderProgram_pointCircles->use();

		m_vao->drawArrays(GL_POINTS, 0, vertexCount);

		shaderProgram_pointCircles->release();
		m_vao->unbind();

		// disable blending for draw buffer 0 (classical scatter plot)
		glDisablei(GL_BLEND, 0);
		glDisablei(GL_BLEND, 1);

		m_pointCircleFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// ====================================================================================== ACCUMULATE RENDER PASS ======================================================================================
	// Accumulate Points into tiles
	if (tile != nullptr) {
		m_tileAccumulateFramebuffer->bind();

		// set viewport to size of accumulation texture
		glViewport(0, 0, tile->m_tile_cols, tile->m_tile_rows);

		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		//ADDITIVE Blending GL_COLOR_ATTACHMENT0
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_ONE, GL_ONE);
		glBlendEquationi(0, GL_FUNC_ADD);

		// -------------------------------------------------------------------------------------------------

		// get shader program and uniforms from ile program
		auto shaderProgram_tile_acc = tile->getAccumulationProgram();

		m_vao->bind();
		shaderProgram_tile_acc->use();

		m_vao->drawArrays(GL_POINTS, 0, vertexCount);

		shaderProgram_tile_acc->release();
		m_vao->unbind();

		// disable blending
		glDisablei(GL_BLEND, 0);

		//reset Viewport
		glViewport(0, 0, viewer()->viewportSize().x, viewer()->viewportSize().y);

		m_tileAccumulateFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// ====================================================================================== MAX VAL RENDER PASS ======================================================================================
	// Get maximum accumulated value (used for coloring) 
	// Get maximum alpha of additive blended point circles (used for alpha normalization)
	// no framebuffer needed, because we don't render anything. we just save the max value into the storage buffer


	// SSBO --------------------------------------------------------------------------------------------------------------------------------------------------
	m_valueMaxBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 0);

	// max accumulated Value
	const uint initialMaxValue = 0;
	m_valueMaxBuffer->clearSubData(GL_R32UI, 0, sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &initialMaxValue);
	m_valueMaxBuffer->clearSubData(GL_R32UI, sizeof(uint), sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &initialMaxValue);
	// -------------------------------------------------------------------------------------------------

	m_tileAccumulateTexture->bindActive(1);
	m_pointCircleTexture->bindActive(2);

	//can use the same shader for hexagon and square tiles
	auto shaderProgram_max_val = shaderProgram("max-val");

	//geometry & fragment shader
	shaderProgram_max_val->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

	//fragment shader
	shaderProgram_max_val->setUniform("pointCircleTexture", 2);

	if (tile == nullptr) {
		//geometry shader
		shaderProgram_max_val->setUniform("maxBounds_acc", maxBounds);
		shaderProgram_max_val->setUniform("minBounds_acc", minBounds);
	}
	else {
		//geometry shader
		shaderProgram_max_val->setUniform("maxBounds_acc", tile->maxBounds_Offset);
		shaderProgram_max_val->setUniform("minBounds_acc", tile->minBounds_Offset);

		shaderProgram_max_val->setUniform("windowWidth", viewer()->viewportSize()[0]);
		shaderProgram_max_val->setUniform("windowHeight", viewer()->viewportSize()[1]);
		//fragment Shader
		shaderProgram_max_val->setUniform("maxTexCoordX", tile->m_tileMaxX);
		shaderProgram_max_val->setUniform("maxTexCoordY", tile->m_tileMaxY);

		shaderProgram_max_val->setUniform("accumulateTexture", 1);
	}

	m_vaoQuad->bind();

	shaderProgram_max_val->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	shaderProgram_max_val->release();

	m_vaoQuad->unbind();

	m_tileAccumulateTexture->unbindActive(1);
	m_pointCircleTexture->unbindActive(2);

	//unbind shader storage buffer
	m_valueMaxBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// ====================================================================================== TILE NORMALS RENDER PASS ======================================================================================
	// accumulate tile normals using KDE texture and save them into tileNormalsBuffer
	// no framebuffer needed, because we don't render anything. we just save into the storage buffer

	// render Tile Normals into storage buffer
	if (tile != nullptr && m_renderTileNormals) {
		// SSBO --------------------------------------------------------------------------------------------------------------------------------------------------
		m_tileNormalsBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 0);

		// one Pixel of data is enough to clear whole buffer
		// https://www.khronos.org/opengl/wiki/GLAPI/glClearBufferData
		const int initialVal = 0;
		m_tileNormalsBuffer->clearData(GL_R32I, GL_RED_INTEGER, GL_INT, &initialVal);
		// -------------------------------------------------------------------------------------------------

		m_kdeTexture->bindActive(1);
		m_tileAccumulateTexture->bindActive(2);

		auto shaderProgram_tile_normals = tile->getTileNormalsProgram();

		//geometry shader
		shaderProgram_tile_normals->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

		shaderProgram_tile_normals->setUniform("windowWidth", viewportSize[0]);
		shaderProgram_tile_normals->setUniform("windowHeight", viewportSize[1]);

		//fragment Shader
		shaderProgram_tile_normals->setUniform("maxTexCoordX", tile->m_tileMaxX);
		shaderProgram_tile_normals->setUniform("maxTexCoordY", tile->m_tileMaxY);

		shaderProgram_tile_normals->setUniform("bufferAccumulationFactor", tile->bufferAccumulationFactor);
		shaderProgram_tile_normals->setUniform("tileSize", tile->tileSizeWS);

		shaderProgram_tile_normals->setUniform("kdeTexture", 1);
		shaderProgram_tile_normals->setUniform("accumulateTexture", 2);

		m_vaoQuad->bind();

		shaderProgram_tile_normals->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		shaderProgram_tile_normals->release();

		m_vaoQuad->unbind();

		m_kdeTexture->unbindActive(1);
		m_tileAccumulateTexture->unbindActive(2);

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}


	// ====================================================================================== TILES RENDER PASS ======================================================================================
	// Render tiles
	if (tile != nullptr) {

		m_valueMaxBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 1);

		m_tilesFramebuffer->bind();

		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		//Blending GL_COLOR_ATTACHMENT0
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationi(0, GL_MAX);

		// -------------------------------------------------------------------------------------------------

		m_tileAccumulateTexture->bindActive(2);
		m_tilesDiscrepanciesTexture->bindActive(3);

		auto shaderProgram_tiles = tile->getTileProgram();

		//geometry shader
		shaderProgram_tiles->setUniform("windowWidth", viewportSize[0]);
		shaderProgram_tiles->setUniform("windowHeight", viewportSize[1]);

		shaderProgram_tiles->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);

		//geometry & fragment shader
		shaderProgram_tiles->setUniform("tileSize", tile->tileSizeWS);
		shaderProgram_tiles->setUniform("tileColor", viewer()->tileColor());

		//fragment Shader
		shaderProgram_tiles->setUniform("tileHeightMult", m_tileHeightMult);
		shaderProgram_tiles->setUniform("densityMult", m_densityMult);
		shaderProgram_tiles->setUniform("borderWidth", m_borderWidth);
		shaderProgram_tiles->setUniform("invertPyramid", m_invertPyramid);
		shaderProgram_tiles->setUniform("showBorder", m_showBorder);
		shaderProgram_tiles->setUniform("blendRange", blendRange * viewer()->scaleFactor());

		shaderProgram_tiles->setUniform("maxTexCoordX", tile->m_tileMaxX);
		shaderProgram_tiles->setUniform("maxTexCoordY", tile->m_tileMaxY);

		shaderProgram_tiles->setUniform("bufferAccumulationFactor", tile->bufferAccumulationFactor);

		//lighting
		shaderProgram_tiles->setUniform("lightPos", vec3(viewLightPosition));
		shaderProgram_tiles->setUniform("viewPos", vec3(0));
		shaderProgram_tiles->setUniform("lightColor", vec3(1));

		// textures
		shaderProgram_tiles->setUniform("accumulateTexture", 2);
		shaderProgram_tiles->setUniform("tilesDiscrepancyTexture", 3);

		// analytical ambient occlusion
		shaderProgram_tiles->setUniform("aaoScaling", m_aaoScaling);

		// diamond cut outline
		shaderProgram_tiles->setUniform("diamondCutOutlineWidth", m_diamondCutOutlineWidth);
		shaderProgram_tiles->setUniform("diamondCutOutlineDarkening", m_diamondCutOutlineDarkening);

		if (m_colorMapLoaded)
		{
			m_colorMapTexture->bindActive(5);
			shaderProgram_tiles->setUniform("colorMapTexture", 5);
			shaderProgram_tiles->setUniform("textureWidth", m_ColorMapWidth);
		}

		m_vaoQuad->bind();

		shaderProgram_tiles->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		shaderProgram_tiles->release();

		m_vaoQuad->unbind();

		if (m_colorMapLoaded)
		{
			m_colorMapTexture->unbindActive(5);
		}

		m_tileAccumulateTexture->unbindActive(2);
		m_tilesDiscrepanciesTexture->unbindActive(3);

		// disable blending
		glDisablei(GL_BLEND, 0);

		m_tilesFramebuffer->unbind();

		//unbind shader storage buffer
		m_valueMaxBuffer->unbind(GL_SHADER_STORAGE_BUFFER);
		m_tileNormalsBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// ====================================================================================== GRID RENDER PASS ======================================================================================
	// render grid into texture

	if (m_renderGrid && tile != nullptr) {
		m_gridFramebuffer->bind();

		glClearDepth(1.0f);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// make sure points are drawn on top of each other
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		//Blending GL_COLOR_ATTACHMENT0
		glEnablei(GL_BLEND, 0);
		glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquationi(0, GL_MAX);

		// -------------------------------------------------------------------------------------------------

		m_gridTexture->bindActive(0);
		//used to check if we actually need to draw the grid for a given square
		m_tileAccumulateTexture->bindActive(1);

		auto shaderProgram_grid = tile->getGridProgram();

		shaderProgram_grid->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
		shaderProgram_grid->setUniform("windowWidth", viewer()->viewportSize()[0]);
		shaderProgram_grid->setUniform("windowHeight", viewer()->viewportSize()[1]);

		//fragment Shader
		shaderProgram_grid->setUniform("gridColor", viewer()->gridColor());
		shaderProgram_grid->setUniform("tileSize", tile->tileSizeWS);
		shaderProgram_grid->setUniform("gridWidth", m_gridWidth * viewer()->scaleFactor());
		shaderProgram_grid->setUniform("accumulateTexture", 1);

		//draw call
		m_vaoTiles->bind();

		shaderProgram_grid->use();
		m_vaoTiles->drawArrays(GL_POINTS, 0, tile->numTiles);
		shaderProgram_grid->release();

		m_vaoTiles->unbind();

		m_gridTexture->unbindActive(0);
		m_tileAccumulateTexture->unbindActive(1);

		// disable blending
		glDisablei(GL_BLEND, 0);

		m_gridFramebuffer->unbind();

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	// ====================================================================================== SHADE/BLEND RENDER PASS ======================================================================================
	// blend everything together and draw to screen

	m_shadeFramebuffer->bind();

	m_pointChartTexture->bindActive(0);
	m_tilesTexture->bindActive(1);
	m_gridTexture->bindActive(3);
	m_pointCircleTexture->bindActive(4);
	//debug
	m_kdeTexture->bindActive(5);
	m_normalsAndDepthTexture->bindActive(2);
	//end debug

	m_valueMaxBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 6);

	auto shaderProgram_shade = shaderProgram("shade");

	shaderProgram_shade->setUniform("backgroundColor", viewer()->backgroundColor());

	if (!m_renderPointCircles && m_selected_tile_style == 0) {
		shaderProgram_shade->setUniform("pointChartTexture", 0);
	}

	if (m_renderPointCircles) {
		shaderProgram_shade->setUniform("pointCircleTexture", 4);
	}

	if (m_selected_tile_style != 0) {
		shaderProgram_shade->setUniform("tilesTexture", 1);
	}

	if (m_renderGrid) {
		shaderProgram_shade->setUniform("gridTexture", 3);
	}

	if (m_renderKDE) {
		shaderProgram_shade->setUniform("kdeTexture", 5);
	}

	if (m_renderNormalBuffer || m_renderDepthBuffer) {
		shaderProgram_shade->setUniform("normalsAndDepthTexture", 2);
	}

	m_vaoQuad->bind();

	shaderProgram_shade->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	shaderProgram_shade->release();

	m_vaoQuad->unbind();

	m_pointChartTexture->unbindActive(0);
	m_tilesTexture->unbindActive(1);
	m_gridTexture->unbindActive(3);
	m_pointCircleTexture->unbindActive(4);
	m_kdeTexture->unbindActive(5);
	m_normalsAndDepthTexture->unbindActive(2);

	//unbind shader storage buffer
	m_valueMaxBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	m_shadeFramebuffer->unbind();

	m_shadeFramebuffer->blit(GL_COLOR_ATTACHMENT0, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, Framebuffer::defaultFBO().get(), GL_BACK, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	currentState->apply();
}

// --------------------------------------------------------------------------------------
// ###########################  TILES ############################################
// --------------------------------------------------------------------------------------
void TileRenderer::calculateTileTextureSize(const mat4 inverseModelViewProjectionMatrix) {

	// if we only render points, we do not need to calculate tile sizes
	if (tile != nullptr) {

		vec3 maxBounds = viewer()->scene()->table()->maximumBounds();
		vec3 minBounds = viewer()->scene()->table()->minimumBounds();

		tile->tileSizeWS = (inverseModelViewProjectionMatrix * vec4(m_tileSize_tmp / tile->tileSizeDiv, 0, 0, 0)).x;
		tile->tileSizeWS *= viewer()->scaleFactor();

		vec3 boundingBoxSize = maxBounds - minBounds;

		tile->calculateNumberOfTiles(boundingBoxSize, minBounds);

		//set texture size
		m_tilesDiscrepanciesTexture->image2D(0, GL_RGBA32F, ivec2(tile->m_tile_cols, tile->m_tile_rows), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_tileAccumulateTexture->image2D(0, GL_RGBA32F, ivec2(tile->m_tile_cols, tile->m_tile_rows), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		//allocate tile normals buffer storage
		m_tileNormalsBuffer->bind(GL_SHADER_STORAGE_BUFFER);
		// we safe each value of the normal (vec4) seperately + accumulated kde height = 5 values
		m_tileNormalsBuffer->setData(sizeof(int) * 5 * tile->m_tile_cols*tile->m_tile_rows, nullptr, GL_STREAM_DRAW);
		m_tileNormalsBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

		//setup grid vertices
		// create m_squareNumCols*m_squareNumRows vertices with position = 0.0f
		// we generate the correct position in the vertex shader
		m_verticesTiles->setData(std::vector<float>(tile->m_tile_cols*tile->m_tile_rows, 0.0f), GL_STATIC_DRAW);

		auto vertexBinding = m_vaoTiles->binding(0);
		vertexBinding->setAttribute(0);
		vertexBinding->setBuffer(m_verticesTiles.get(), 0, sizeof(float));
		vertexBinding->setFormat(1, GL_FLOAT);
		m_vaoTiles->enable(0);

		//calc2D discrepancy and setup discrepancy buffer
		std::vector<float> tilesDiscrepancies(tile->numTiles, 0.0f);

		if (m_renderDiscrepancy) {
			tilesDiscrepancies = calculateDiscrepancy2D(viewer()->scene()->table()->activeXColumn(), viewer()->scene()->table()->activeYColumn(),
				viewer()->scene()->table()->maximumBounds(), viewer()->scene()->table()->minimumBounds());
		}

		m_discrepanciesBuffer->setData(tilesDiscrepancies, GL_STATIC_DRAW);

		vertexBinding = m_vaoTiles->binding(1);
		vertexBinding->setAttribute(1);
		vertexBinding->setBuffer(m_discrepanciesBuffer.get(), 0, sizeof(float));
		vertexBinding->setFormat(1, GL_FLOAT);
		m_vaoTiles->enable(1);
	}
}

// --------------------------------------------------------------------------------------
// ###########################  GUI ##################################################### 
// --------------------------------------------------------------------------------------
/*
Renders the User interface
*/
void TileRenderer::renderGUI() {

	// boolean variable used to automatically update the data
	static bool dataChanged = false;
	ImGui::Begin("Illuminated Point Plots");

	if (ImGui::CollapsingHeader("CSV-Files"), ImGuiTreeNodeFlags_DefaultOpen)
	{

		ImGui::Combo("Files", &m_fileDataID, m_guiFileNames.c_str());

		if (m_fileDataID != m_oldFileDataID)
		{
			//std::cout << "File selection event - " << "File: " << m_fileDataID << "\n";

			// reset column names
			m_guiColumnNames = { 'N', 'o', 'n', 'e', '\0' };

			if (m_fileDataID != 0) {

				// initialize table
				viewer()->scene()->table()->load("./dat/" + m_fileNames[m_fileDataID]);

				// extract column names and prepare GUI
				std::vector<std::string> tempNames = viewer()->scene()->table()->getColumnNames();

				for (std::vector<std::string>::iterator it = tempNames.begin(); it != tempNames.end(); ++it) {
					m_guiColumnNames += *it + '\0';
				}

				// provide default selections assuming
				m_xAxisDataID = 1;		// contains the X-values
				m_yAxisDataID = 2;		// contains the Y-values
				m_radiusDataID = 3;		// contains the radii (currently not used)
				m_colorDataID = 4;		// contains the colors (currently not used)

				// provide default parameters on the selection ---------------------------------------------------------------------------------------	
				if (m_fileNames[m_fileDataID] == "CaliforniaHousing.csv") 
				{
					m_tileSize_tmp = 7.0f;
					m_pointCircleRadius = 4.0f;
					m_aaoScaling = 0.5f;
					m_sigma = 2.0f;
					m_kdeRadius = 1.0f;
					m_renderPointCircles = true;
					m_showDiamondCutOutline = true;
					m_renderTileNormals = true;
				} 
				else if (m_fileNames[m_fileDataID] == "GenderEquality-EU28-2020.csv") 
				{
					m_tileSize_tmp = 42.0;
					m_pointCircleRadius = 10.0f;
					m_aaoScaling = 4.0f;
					m_sigma = 2.0f;
					m_kdeRadius = 1.0f;
					m_renderPointCircles = false;
					m_showDiamondCutOutline = false;
					m_renderTileNormals = false;
				} 
				else if (m_fileNames[m_fileDataID] == "Methodology_AmberInclusions.csv") 
				{
					m_tileSize_tmp = 32.2f;
					m_pointCircleRadius = 10.0f;
					m_aaoScaling = 2.5f;
					m_sigma = 1.5f;
					m_kdeRadius = 65.0f;
					m_renderPointCircles = true;
					m_showDiamondCutOutline = true;
					m_renderTileNormals = true;
				} 
				else if (m_fileNames[m_fileDataID] == "Methodology_DiamondCut.csv") 
				{
					m_tileSize_tmp = 32.2f;
					m_pointCircleRadius = 10.0f;
					m_aaoScaling = 2.5f;
					m_sigma = 1.5f;
					m_kdeRadius = 65.0f;
					m_renderPointCircles = true;
					m_showDiamondCutOutline = true;
					m_renderTileNormals = true;
				} 
				else if (m_fileNames[m_fileDataID] == "Methodology_ReliefMosaic.csv") 
				{
					m_tileSize_tmp = 32.2f;
					m_pointCircleRadius = 10.0f;
					m_aaoScaling = 2.5f;
					m_sigma = 1.5f;
					m_kdeRadius = 65.0f;
					m_renderPointCircles = true;
					m_showDiamondCutOutline = true;
					m_renderTileNormals = true;
				} 
				else if (m_fileNames[m_fileDataID].compare("Tornados_1950-2019.csv") == 0) 
				{
					m_tileSize_tmp = 5.2f;
					m_pointCircleRadius = 2.0f;
					m_aaoScaling = 0.5f;
					m_sigma = 10.0f;
					m_kdeRadius = 5.0f;
					m_renderPointCircles = true;
					m_showDiamondCutOutline = true;
					m_renderTileNormals = true;
				} 
				else if (m_fileNames[m_fileDataID].length() >= 14 && m_fileNames[m_fileDataID].substr(0,14) == "UserStudy_Task") 
				{
					m_tileSize_tmp = 27.5f;
					m_pointCircleRadius = 6.5f;
					m_aaoScaling = 2.5f;
					m_sigma = 1.5f;
					m_kdeRadius = 65.0f;
					m_renderPointCircles = true;
					m_showDiamondCutOutline = true;
					m_renderTileNormals = true;
				}
				//------------------------------------------------------------------------------------------------------------------------------------

			}

			// update status
			m_oldFileDataID = m_fileDataID;
			dataChanged = true;
		}

		// show all column names from selected CSV file
		ImGui::Text("Selected Columns:");
		ImGui::Combo("X-axis", &m_xAxisDataID, m_guiColumnNames.c_str());
		ImGui::Combo("Y-axis", &m_yAxisDataID, m_guiColumnNames.c_str());

		if (ImGui::Button("Update") || dataChanged)
		{
			// update buffers according to recent changes -> since combo also contains 'None" we need to subtract 1 from ID
			viewer()->scene()->table()->updateBuffers(m_xAxisDataID - 1, m_yAxisDataID - 1, m_radiusDataID - 1, m_colorDataID - 1);

			// update VBOs for all four columns
			m_xColumnBuffer->setData(viewer()->scene()->table()->activeXColumn(), GL_STATIC_DRAW);
			m_yColumnBuffer->setData(viewer()->scene()->table()->activeYColumn(), GL_STATIC_DRAW);
			m_radiusColumnBuffer->setData(viewer()->scene()->table()->activeRadiusColumn(), GL_STATIC_DRAW);
			m_colorColumnBuffer->setData(viewer()->scene()->table()->activeColorColumn(), GL_STATIC_DRAW);


			// update VAO for all buffers ----------------------------------------------------
			auto vertexBinding = m_vao->binding(0);
			vertexBinding->setAttribute(0);
			vertexBinding->setBuffer(m_xColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(0);

			vertexBinding = m_vao->binding(1);
			vertexBinding->setAttribute(1);
			vertexBinding->setBuffer(m_yColumnBuffer.get(), 0, sizeof(float));
			vertexBinding->setFormat(1, GL_FLOAT);
			m_vao->enable(1);

			// -------------------------------------------------------------------------------

			// Scaling the model's bounding box to the canonical view volume
			vec3 boundingBoxSize = viewer()->scene()->table()->maximumBounds() - viewer()->scene()->table()->minimumBounds();
			float maximumSize = std::max({ boundingBoxSize.x, boundingBoxSize.y, boundingBoxSize.z });
			mat4 modelTransform = scale(vec3(3.0f) / vec3(maximumSize));
			modelTransform = modelTransform * translate(-0.5f*(viewer()->scene()->table()->minimumBounds() + viewer()->scene()->table()->maximumBounds()));
			viewer()->setModelTransform(modelTransform);

			// store diameter of current scatter plot and initialize light position
			viewer()->m_scatterPlotDiameter = sqrt(pow(boundingBoxSize.x, 2) + pow(boundingBoxSize.y, 2));

			// initial position of the light source (azimuth 120 degrees, elevation 45 degrees, 5 times the distance to the object in center) ---------------------------------------------------------
			glm::mat4 viewTransform = viewer()->viewTransform();
			glm::vec3 initLightDir = normalize(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(120.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
			glm::mat4 newLightTransform = glm::inverse(viewTransform)*glm::translate(mat4(1.0f), (5 * viewer()->m_scatterPlotDiameter*initLightDir))*viewTransform;
			viewer()->setLightTransform(newLightTransform);
			//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

			// calculate accumulate texture settings - needs to be last step here ------------------------------
			calculateTileTextureSize(inverse(viewer()->modelViewProjectionTransform()));
			// -------------------------------------------------------------------------------

		}

		if (ImGui::CollapsingHeader("Color Maps"), ImGuiTreeNodeFlags_DefaultOpen)
		{
			// show all available color-maps
			ImGui::Combo("Maps", &m_colorMap, "None\0Bone\0Cubehelix\0GistEart\0GnuPlot2\0Grey\0Inferno\0Magma\0Plasma\0PuBuGn\0Rainbow\0Summer\0Virdis\0Winter\0Wista\0YlGnBu\0YlOrRd\0");

			// allow the user to load a discrete version of the color map
			ImGui::Checkbox("Monochrome-Tiles", &m_renderMomochromeTiles);

			// load new texture if either the texture has changed or the type has changed from discrete to continuous or vice versa
			if (m_colorMap != m_oldColorMap)
			{
				if (m_colorMap > 0) {
					std::vector<std::string> colorMapFilenames = { "./dat/colormaps/bone_1D.png", "./dat/colormaps/cubehelix_1D.png", "./dat/colormaps/gist_earth_1D.png",  "./dat/colormaps/gnuplot2_1D.png" ,
						"./dat/colormaps/grey_1D.png", "./dat/colormaps/inferno_1D.png", "./dat/colormaps/magma_1D.png", "./dat/colormaps/plasma_1D.png", "./dat/colormaps/PuBuGn_1D.png",
						"./dat/colormaps/rainbow_1D.png", "./dat/colormaps/summer_1D.png", "./dat/colormaps/virdis_1D.png", "./dat/colormaps/winter_1D.png", "./dat/colormaps/wista_1D.png", "./dat/colormaps/YlGnBu_1D.png",
						"./dat/colormaps/YlOrRd_1D.png" };

					int colorMapWidth, colorMapHeight, colorMapChannels;
					std::string textureName = colorMapFilenames[m_colorMap - 1];

					stbi_set_flip_vertically_on_load(true);
					unsigned char* data = stbi_load(textureName.c_str(), &colorMapWidth, &colorMapHeight, &colorMapChannels, 0);

					if (data)
					{
						m_colorMapTexture->image1D(0, GL_RGBA, colorMapWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
						m_colorMapTexture->generateMipmap();

						// store width of texture and mark as loaded
						m_ColorMapWidth = colorMapWidth;
						m_colorMapLoaded = true;

						stbi_image_free(data);
					}
					else
					{
						globjects::debug() << "Could not load " << colorMapFilenames[m_colorMap - 1] << "!";
					}
				}
				else
				{
					// disable color map
					m_colorMapLoaded = false;
				}

				// update status
				m_oldColorMap = m_colorMap;
			}
		}

		if (ImGui::CollapsingHeader("Discrepancy"), ImGuiTreeNodeFlags_DefaultOpen)
		{
			ImGui::Checkbox("Render Point Circles", &m_renderPointCircles);
			ImGui::SliderFloat("Point Circle Radius", &m_pointCircleRadius, 1.0f, 100.0f);
			// ImGui::Checkbox("Show Discrepancy", &m_renderDiscrepancy_tmp);
			// ImGui::SliderFloat("Ease In", &m_discrepancy_easeIn_tmp, 1.0f, 5.0f);
			// ImGui::SliderFloat("Low Point Count", &m_discrepancy_lowCount_tmp, 0.0f, 1.0f);
			// ImGui::SliderFloat("Discrepancy Divisor", &m_discrepancyDiv, 1.0f, 3.0f);
		}
		if (ImGui::CollapsingHeader("Tiles"), ImGuiTreeNodeFlags_DefaultOpen)
		{
			const char* tile_styles[]{ "none", "square", "hexagon" };
			//ImGui::Combo("Tile Rendering", &m_selected_tile_style_tmp, tile_styles, IM_ARRAYSIZE(tile_styles));
			ImGui::Checkbox("Render Grid", &m_renderGrid);
			ImGui::SliderFloat("Grid Width", &m_gridWidth, 1.0f, 3.0f);
			ImGui::SliderFloat("Tile Size", &m_tileSize_tmp, 1.0f, 10.0f);

			// allow for (optional) analytical ambient occlusion of the hex-tiles
			ImGui::Checkbox("Analytical AO", &m_renderAnalyticalAO);
			ImGui::SliderFloat("AAO Scaling", &m_aaoScaling, 0.0f, 16.0f);

			// allow for modification of the diamond cut outline
			ImGui::Checkbox("Diamond Cut Outline", &m_showDiamondCutOutline);
			ImGui::SliderFloat("Outline Width", &m_diamondCutOutlineWidth, 0.0f, 10.0f);
			ImGui::SliderFloat("Outline Darkening", &m_diamondCutOutlineDarkening, 0.0f, 1.0f);
		}

		if (ImGui::CollapsingHeader("Regression Plane"), ImGuiTreeNodeFlags_DefaultOpen)
		{
			// ImGui::Checkbox("Render KDE", &m_renderKDE);
			ImGui::Checkbox("Render Tile Normals", &m_renderTileNormals);
			ImGui::SliderFloat("Sigma", &m_sigma, 0.1f, 10.0f);
			ImGui::SliderFloat("Sample Radius", &m_kdeRadius, 1.0f, 100.0f);
			ImGui::SliderFloat("Density Multiply", &m_densityMult, 1.0f, 20.0f);
			ImGui::SliderFloat("Tile Height Mult", &m_tileHeightMult, 1.0f, 20.0f);
			// the borderWidth cannot go from 0-1
			// if borderWidth == 1, all inside corner points are at the exact same position (tile center)
			// and we can not longer decide if a point is in the border or not
			// if borderWidth == 0, at least one of the inside corner points is at the exact same position as its corresponding outside corner point
			// then it can happen, that we can no longer compute the lighting normal for the corresponding border.
			ImGui::SliderFloat("Border Width", &m_borderWidth, 0.01f, 0.99f);
			//ImGui::Checkbox("Show Border", &m_showBorder);
			ImGui::Checkbox("Invert Pyramid", &m_invertPyramid);
		}

		// ImGui::Checkbox("Show Normal Buffer", &m_renderNormalBuffer);
		// ImGui::Checkbox("Show Depth Buffer", &m_renderDepthBuffer);


		// update status
		dataChanged = false;
	}
	ImGui::End();
}


/*
Gathers all #define, reset the defines files and reload the shaders
*/
void TileRenderer::setShaderDefines() {
	std::string defines = "";


	if (m_colorMapLoaded)
		defines += "#define COLORMAP\n";

	if (m_renderPointCircles) {
		defines += "#define RENDER_POINT_CIRCLES\n";
	}

	if (m_renderDiscrepancy) {
		defines += "#define RENDER_DISCREPANCY\n";
	}

	if (m_selected_tile_style != 0)
	{
		defines += "#define RENDER_TILES\n";
	}

	if (m_selected_tile_style == 2)
	{
		defines += "#define RENDER_HEXAGONS\n";
	}

	if (m_renderGrid) {
		defines += "#define RENDER_GRID\n";
	}

	if (m_renderKDE)
		defines += "#define RENDER_KDE\n";

	if (m_renderTileNormals)
		defines += "#define RENDER_TILE_NORMALS\n";

	if (m_renderNormalBuffer)
		defines += "#define RENDER_NORMAL_BUFFER\n";

	if (m_renderDepthBuffer)
		defines += "#define RENDER_DEPTH_BUFFER\n";

	if (m_renderAnalyticalAO)
		defines += "#define RENDER_ANALYTICAL_AO\n";

	if (m_renderMomochromeTiles)
		defines += "#define RENDER_MONOCHROME_TILES\n";

	if (m_showDiamondCutOutline)
		defines += "#define RENDER_DIAMONDCUT_OUTLINE\n";

	if (defines != m_shaderSourceDefines->string())
	{
		m_shaderSourceDefines->setString(defines);

		reloadShaders();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//INTERVAL MAPPING
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//maps value x from [a,b] --> [0,c]
float honeycomb::TileRenderer::mapInterval(float x, float a, float b, int c)
{
	return (x - a)*c / (b - a);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DISCREPANCY
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//PRECONDITION: tile != nullptr
std::vector<float> TileRenderer::calculateDiscrepancy2D(const std::vector<float>& samplesX, const std::vector<float>& samplesY, vec3 maxBounds, vec3 minBounds)
{

	// Calculates the discrepancy of this data.
	// Assumes the data is [0,1) for valid sample range.
	// Assmues samplesX.size() == samplesY.size()
	int numSamples = samplesX.size();

	std::vector<float> sortedX(numSamples, 0.0f);
	std::vector<float> sortedY(numSamples, 0.0f);

	std::vector<float> pointsInTilesCount(tile->numTiles, 0.0f);
	std::vector<float> tilesMaxBoundX(tile->numTiles, -INFINITY);
	std::vector<float> tilesMaxBoundY(tile->numTiles, -INFINITY);
	std::vector<float> tilesMinBoundX(tile->numTiles, INFINITY);
	std::vector<float> tilesMinBoundY(tile->numTiles, INFINITY);

	std::vector<float> discrepancies(tile->numTiles, 0.0f);

	float eps = 0.05;

	//std::cout << "Discrepancy: " << numSamples << " Samples; " << numSquares << " Tiles" << '\n';

	//timing
	std::clock_t start;
	double duration;

	start = std::clock();

	//Step 1: Count how many elements belong to each tile
	for (int i = 0; i < numSamples; i++) {

		float sampleX = samplesX[i];
		float sampleY = samplesY[i];

		pointsInTilesCount[tile->mapPointToTile1D(vec2(sampleX, sampleY))]++;
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	//std::cout << "Step1: " << duration << '\n';

	start = clock();

	//Step 2: calc prefix Sum
	std::vector<float> pointsInTilesPrefixSum(pointsInTilesCount);
	int prefixSum = 0;
	int sampleCount = 0;
	int maxSampleCount = 0;
	for (int i = 0; i < tile->numTiles; i++) {
		sampleCount = pointsInTilesPrefixSum[i];
		pointsInTilesPrefixSum[i] = prefixSum;
		prefixSum += sampleCount;
		maxSampleCount = max(maxSampleCount, sampleCount);
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	//std::cout << "Step2: " << duration << '\n';

	start = clock();

	//Step 3: sort points according to tile
	// 1D array with "buckets" according to prefix Sum of tile
	// also set the bounding box of points inside tile
	std::vector<float> pointsInTilesRunningPrefixSum(pointsInTilesPrefixSum);
	for (int i = 0; i < numSamples; i++) {

		float sampleX = samplesX[i];
		float sampleY = samplesY[i];

		int squareIndex1D = tile->mapPointToTile1D(vec2(sampleX, sampleY));

		// put sample in correct position and increment prefix sum
		int sampleIndex = pointsInTilesRunningPrefixSum[squareIndex1D]++;
		sortedX[sampleIndex] = sampleX;
		sortedY[sampleIndex] = sampleY;

		//set bounding box of samples inside tile
		tilesMaxBoundX[squareIndex1D] = max(tilesMaxBoundX[squareIndex1D], sampleX);
		tilesMaxBoundY[squareIndex1D] = max(tilesMaxBoundY[squareIndex1D], sampleY);

		tilesMinBoundX[squareIndex1D] = min(tilesMinBoundX[squareIndex1D], sampleX);
		tilesMinBoundY[squareIndex1D] = min(tilesMinBoundY[squareIndex1D], sampleY);
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	//std::cout << "Step3: " << duration << '\n';

	start = clock();

	//Step 4: calculate discrepancy for each tile
	// this is done using a parallel for loop with the OpenMP Framework - need to compile with OpenMP support (add in CMakeList)
	// the computations of the discrpeancy of each seperate tile is independent
	// the number of threads used can be set to how many cores the respective PC has
	omp_set_num_threads(4);
#pragma omp parallel
	{
		/*
#pragma omp master
		{
			std::cout << "NumThreads: " << omp_get_num_threads() << '\n';
		}
#pragma omp barrier

		std::cout << "Thread: " << omp_get_thread_num() << '\n';
#pragma omp barrier
*/

#pragma omp for
		for (int i = 0; i < tile->numTiles; i++) {

			float maxDifference = 0.0f;
			const int startPoint = pointsInTilesPrefixSum[i];
			const int stopPoint = pointsInTilesPrefixSum[i] + pointsInTilesCount[i];

			//use the addition and not PrefixSum[i+1] so we can easily get the last boundary
			for (int j = startPoint; j < stopPoint; j++) {
				//normalize sample inside its tile
				float stopValueXNorm = mapInterval(sortedX[j], tilesMinBoundX[i], tilesMaxBoundX[i], 1);
				float stopValueYNorm = mapInterval(sortedY[j], tilesMinBoundY[i], tilesMaxBoundY[i], 1);

				// calculate area
				float area = stopValueXNorm * stopValueYNorm;

				// closed interval [startValue, stopValue]
				float countInside = 0.0f;
				for (int k = startPoint; k < stopPoint; k++)
				{
					//normalize sample inside its square
					float sampleXNorm = mapInterval(sortedX[k], tilesMinBoundX[i], tilesMaxBoundX[i], 1);
					float sampleYNorm = mapInterval(sortedY[k], tilesMinBoundY[i], tilesMaxBoundY[i], 1);


					if (sampleXNorm <= stopValueXNorm &&
						sampleYNorm <= stopValueYNorm)
					{
						countInside++;
					}
				}
				float density = countInside / float(pointsInTilesCount[i]);
				float difference = std::abs(density - area);

				if (difference > maxDifference)
				{
					maxDifference = difference;
				}
			}

			maxDifference = max(eps, maxDifference);

			// Ease In
			maxDifference = pow(maxDifference, m_discrepancy_easeIn);

			// account for tiles with few points
			//maxDifference = (1 - m_discrepancy_lowCount) * maxDifference + m_discrepancy_lowCount * (1 - pow((pointsInTilesCount[i] / float(maxSampleCount)), 2));
			maxDifference = min(maxDifference + m_discrepancy_lowCount * (1 - float(pow((pointsInTilesCount[i] / float(maxSampleCount)), 2))), 1.0f);


			discrepancies[i] = maxDifference;
		}
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	//std::cout << "Step4: " << duration << '\n' << '\n';

	return discrepancies;
}
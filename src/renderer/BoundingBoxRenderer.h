#pragma once
#include "Renderer.h"
#include <memory>

#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>
#include <glm/glm.hpp>

#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/base/File.h>

namespace honeycomb
{
	class Viewer;

	class BoundingBoxRenderer : public Renderer
	{
	public:
		BoundingBoxRenderer(Viewer *viewer);
		virtual void display();
		virtual std::list<globjects::File*> shaderFiles() const;

	private:
		
		std::unique_ptr<globjects::VertexArray> m_vao = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_vertices = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_indices = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Program> m_program = std::make_unique<globjects::Program>();

		std::unique_ptr<globjects::File> m_vertexShaderSource = nullptr;
		std::unique_ptr<globjects::File> m_tesselationControlShaderSource = nullptr;
		std::unique_ptr<globjects::File> m_tesselationEvaluationShaderSource = nullptr;
		std::unique_ptr<globjects::File> m_geometryShaderSource = nullptr;
		std::unique_ptr<globjects::File> m_fragmentShaderSource = nullptr;
		
		std::unique_ptr<globjects::Shader> m_tesselationControlShader = nullptr;
		std::unique_ptr<globjects::Shader> m_tesselationEvaluationShader = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShader = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShader = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShader = nullptr;
		gl::GLsizei m_size;
	};

}
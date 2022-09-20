#pragma once

#include <memory>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <globjects/Texture.h>
#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <imgui.h>

#include "Scene.h"
#include "Interactor.h"
#include "renderer/Renderer.h"

namespace honeycomb
{

	class Viewer
	{
	public:
		Viewer(GLFWwindow* window, Scene* scene);
		void display();

		GLFWwindow * window();
		Scene* scene();

		glm::ivec2 viewportSize() const;

		glm::vec3 backgroundColor() const;
		glm::vec3 samplePointColor() const;
		glm::vec3 gridColor() const;
		glm::vec3 tileColor() const;

		float scaleFactor() const;

		glm::mat4 modelTransform() const;
		glm::mat4 viewTransform() const;
		glm::mat4 lightTransform() const;
		glm::mat4 projectionTransform() const;

		glm::mat4 modelViewTransform() const;
		glm::mat4 modelViewProjectionTransform() const;
		glm::mat4 modelLightTransform() const;

		float m_windowHeight = 720.0f;
		float m_windowWidth = 1280.0f;
		float m_scatterPlotDiameter = 2.0f*sqrt(3.0f);

		void setBackgroundColor(const glm::vec3& c);
		void setSamplePointColor(const glm::vec3& c);
		void setGridColor(const glm::vec3& c);
		void setTileColor(const glm::vec3& c);

		void setScaleFactor(const float s);

		void setViewTransform(const glm::mat4& m);
		void setModelTransform(const glm::mat4& m);
		void setLightTransform(const glm::mat4& m);
		void setProjectionTransform(const glm::mat4& m);

		void saveImage(const std::string & filename);

	private:

		void beginFrame();
		void endFrame();
		void renderUi();
		void mainMenu();

		static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
		static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
		static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		static void charCallback(GLFWwindow* window, unsigned int c);

		static const char* GetClipboardText(void* user_data);
		static void SetClipboardText(void* user_data, const char* text);

		GLFWwindow* m_window;
		Scene *m_scene;

		std::vector<std::unique_ptr<Interactor>> m_interactors;
		std::vector<std::unique_ptr<Renderer>> m_renderers;

		double m_time = 0.0;
		bool m_mousePressed[3] = { false, false, false };
		float m_mouseWheel = 0.0f;

		std::unique_ptr<globjects::File> m_vertexShaderSourceUi = nullptr;
		std::unique_ptr<globjects::File> m_fragmentShaderSourceUi = nullptr;

		std::unique_ptr<globjects::Shader> m_vertexShaderUi = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderUi = nullptr;
		std::unique_ptr<globjects::Program> m_programUi;

		std::unique_ptr<globjects::Texture> m_fontTexture = nullptr;
		std::unique_ptr<globjects::Buffer> m_verticesUi = nullptr;
		std::unique_ptr<globjects::Buffer> m_indicesUi = nullptr;
		std::unique_ptr<globjects::VertexArray> m_vaoUi = nullptr;

		// default colors for background and scatter plot sample points
		glm::vec3 m_backgroundColor = glm::vec3(0, 0, 0);				// black
		glm::vec3 m_samplePointColor = glm::vec3(0, 0, 0);				// gray
		glm::vec3 m_gridColor = glm::vec3(0, 0, 0);						// black
		glm::vec3 m_tileColor = glm::vec3(1, 1, 1);						// white

		float m_scaleFactor = 1.0f; //used to calculate correct size of elements whose size is set by the user

		glm::mat4 m_modelTransform = glm::mat4(1.0f);
		glm::mat4 m_viewTransform = glm::mat4(1.0f);
		glm::mat4 m_projectionTransform = glm::mat4(1.0f);
		glm::mat4 m_lightTransform = glm::mat4(1.0f);

		bool m_showUi = true;
		bool m_saveScreenshot = false;
	};


}
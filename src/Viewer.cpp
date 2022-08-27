#include "Viewer.h"

#include <glbinding/gl/gl.h>
#include <iostream>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#endif

#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include "CameraInteractor.h"
#include "renderer/BoundingBoxRenderer.h"
#include "renderer/tileRenderer/TileRenderer.h"
#include "Scene.h" 
#include "CSV/Table.h"
#include <fstream>
#include <sstream>
#include <list>
#include <lodepng.h>

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;

Viewer::Viewer(GLFWwindow *window, Scene *scene) : m_window(window), m_scene(scene)
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	//io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
	//io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)

	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

	//io.RenderDrawListsFn = nullptr;	// https://github.com/ocornut/imgui/issues/1599
	io.SetClipboardTextFn =  SetClipboardText;
	io.GetClipboardTextFn = GetClipboardText;
	io.ClipboardUserData = window;

	int width, height;
	unsigned char* pixels;

	//io.Fonts->AddFontDefault();
	io.Fonts->AddFontFromFileTTF("./res/ui/Lato-Semibold.ttf", 18);
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	m_fontTexture = Texture::create(GL_TEXTURE_2D);
	m_fontTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_fontTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_fontTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_fontTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_fontTexture->image2D(0, GL_RGBA, ivec2(width,height), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	io.Fonts->TexID = (ImTextureID) m_fontTexture->id();
	
	ImGuiStyle& style = ImGui::GetStyle();

	m_verticesUi = Buffer::create();
	m_indicesUi = Buffer::create();
	m_vaoUi = VertexArray::create();

	m_vertexShaderSourceUi = Shader::sourceFromFile("./res/ui/ui-vs.glsl");
	m_fragmentShaderSourceUi = Shader::sourceFromFile("./res/ui/ui-fs.glsl");

	m_vertexShaderUi = Shader::create(GL_VERTEX_SHADER, m_vertexShaderSourceUi.get());
	m_fragmentShaderUi = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderSourceUi.get());

	m_programUi = Program::create();
	m_programUi->attach(m_vertexShaderUi.get(), m_fragmentShaderUi.get());

	
#ifdef _WIN32
	io.ImeWindowHandle = glfwGetWin32Window(window);
#endif

	glfwSetWindowUserPointer(window, static_cast<void*>(this));
	glfwSetFramebufferSizeCallback(window, &Viewer::framebufferSizeCallback);
	glfwSetKeyCallback(window, &Viewer::keyCallback);
	glfwSetMouseButtonCallback(window, &Viewer::mouseButtonCallback);
	glfwSetCursorPosCallback(window, &Viewer::cursorPosCallback);
	glfwSetCharCallback(window, &Viewer::charCallback);
	glfwSetScrollCallback(window, &Viewer::scrollCallback);

	m_interactors.emplace_back(std::make_unique<CameraInteractor>(this));
	//m_renderers.emplace_back(std::make_unique<SphereRenderer>(this));
	m_renderers.emplace_back(std::make_unique<TileRenderer>(this));

	// remove the following line to disable the bounding box renderer---------
	//m_renderers.emplace_back(std::make_unique<BoundingBoxRenderer>(this));
	//------------------------------------------------------------------------

	int i = 1;

	globjects::debug() << "Available renderers (use the number keys to toggle):";

	for (auto& r : m_renderers)
	{
		globjects::debug() << "  " << i << " - " << typeid(*r.get()).name();
		++i;
	}
}

void Viewer::display()
{
	beginFrame();
	mainMenu();

	glClearColor(m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, viewportSize().x, viewportSize().y);

	// update screen dimensions
	m_windowWidth = viewportSize().x;
	m_windowHeight = viewportSize().y;
	
	for (auto& r : m_renderers)
	{
		if (r->isEnabled())
		{		
			r->display();
		}
	}
	
	for (auto& i : m_interactors)
	{
		i->display();
	}

	endFrame();
}

GLFWwindow * Viewer::window()
{
	return m_window;
}

Scene* Viewer::scene()
{
	return m_scene;
}

ivec2 Viewer::viewportSize() const
{
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	return ivec2(width,height);
}

glm::vec3 Viewer::backgroundColor() const
{
	return m_backgroundColor;
}

glm::vec3 Viewer::samplePointColor() const
{
	return m_samplePointColor;
}

glm::vec3 Viewer::gridColor() const
{
	return m_gridColor;
}

glm::vec3 Viewer::tileColor() const
{
	return m_tileColor;
}

float Viewer::scaleFactor() const
{
	return m_scaleFactor;
}

mat4 Viewer::modelTransform() const
{
	return m_modelTransform;
}

mat4 Viewer::viewTransform() const
{
	return m_viewTransform;
}

void Viewer::setModelTransform(const glm::mat4& m)
{
	m_modelTransform = m;
}

void molumes::Viewer::setBackgroundColor(const glm::vec3 & c)
{
	m_backgroundColor = c;
}

void molumes::Viewer::setSamplePointColor(const glm::vec3 & c)
{
	m_samplePointColor = c;
}

void molumes::Viewer::setGridColor(const glm::vec3 & c)
{
	m_gridColor = c;
}

void molumes::Viewer::setTileColor(const glm::vec3 & c)
{
	m_tileColor = c;
}

void molumes::Viewer::setScaleFactor(const float s)
{
	m_scaleFactor = s;
}

void Viewer::setViewTransform(const glm::mat4& m)
{
	m_viewTransform = m;
}

void Viewer::setProjectionTransform(const glm::mat4& m)
{
	m_projectionTransform = m;
}

void Viewer::setLightTransform(const glm::mat4& m)
{
	m_lightTransform = m;
}


mat4 Viewer::projectionTransform() const
{
	return m_projectionTransform;
}

mat4 Viewer::lightTransform() const
{
	return m_lightTransform;
}


mat4 Viewer::modelViewTransform() const
{
	return viewTransform()*modelTransform();
}

mat4 Viewer::modelViewProjectionTransform() const
{
	return projectionTransform()*modelViewTransform();
}

mat4 Viewer::modelLightTransform() const
{
	return lightTransform()*modelTransform();
}

void Viewer::saveImage(const std::string & filename)
{
	ivec2 size = viewportSize();
	std::vector<unsigned char> image(size.x*size.y * 4);
	std::vector<unsigned char> flipped(size.x*size.y * 4);

	glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&image.front());

	for (uint y = 0; y < size.y; y++)
	{
		for (uint x = 0; x < size.x; x++)
		{
			flipped[4* x + 0 + 4 * y*size.x] = image[4 * x + 0 + 4 * (size.y - 1 - y)*size.x];
			flipped[4* x + 1 + 4 * y*size.x] = image[4 * x + 1 + 4 * (size.y - 1 - y)*size.x];
			flipped[4* x + 2 + 4 * y*size.x] = image[4 * x + 2 + 4 * (size.y - 1 - y)*size.x];
			flipped[4* x + 3 + 4 * y*size.x] = image[4 * x + 3 + 4 * (size.y - 1 - y)*size.x];
		}
	}

	lodepng::encode(filename, flipped, size.x, size.y);
}

void Viewer::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		for (auto& i : viewer->m_interactors)
		{
			i->framebufferSizeEvent(width, height);
		}
	}
}

void Viewer::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		if (viewer->m_showUi)
		{
			ImGuiIO& io = ImGui::GetIO();
			if (action == GLFW_PRESS)
				io.KeysDown[key] = true;
			if (action == GLFW_RELEASE)
				io.KeysDown[key] = false;

			(void)mods; // Modifiers are not reliable across systems
			io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
			io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
			io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
			io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];

			if (io.WantCaptureKeyboard)
				return;
		}

		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
		{
			viewer->m_showUi = !viewer->m_showUi;
		}

		if (key == GLFW_KEY_F5 && action == GLFW_RELEASE)
		{
			for (auto& r : viewer->m_renderers)
			{
				std::cout << "Reloading shaders for instance of " << typeid(*r.get()).name() << " ... " << std::endl;

				r->reloadShaders();

				std::cout << "Reloading shaders for viewer << " << std::endl;
				
					viewer->m_vertexShaderSourceUi->reload();
				viewer->m_fragmentShaderSourceUi->reload();

				std::cout << "2 shaders reloaded." << std::endl << std::endl;

			}
		}
		else if (key == GLFW_KEY_F2 && action == GLFW_RELEASE)
		{
			viewer->m_saveScreenshot = true;
		}
		else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 && action == GLFW_RELEASE)		
		{
			int index = key - GLFW_KEY_1;

			if (index < viewer->m_renderers.size())
			{
				bool enabled = viewer->m_renderers[index]->isEnabled();

				if (enabled)
					std::cout << "Renderer " << index + 1 << " of type " << typeid(*viewer->m_renderers[index].get()).name() << " is now disabled." << std::endl << std::endl;
				else
					std::cout << "Renderer " << index + 1 << " of type " << typeid(*viewer->m_renderers[index].get()).name() << " is now enabled." << std::endl << std::endl;

				viewer->m_renderers[index]->setEnabled(!enabled);
			}
		}


		for (auto& i : viewer->m_interactors)
		{
			i->keyEvent(key, scancode, action, mods);
		}
	}
}

void Viewer::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		if (viewer->m_showUi)
		{
			ImGuiIO& io = ImGui::GetIO();

			if (io.WantCaptureMouse)
				return;
		}

		if (action == GLFW_PRESS && button >= 0 && button < 3)
			viewer->m_mousePressed[button] = true;

		for (auto& i : viewer->m_interactors)
		{
			i->mouseButtonEvent(button, action, mods);
		}
	}
}

void Viewer::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		if (viewer->m_showUi)
		{
			ImGuiIO& io = ImGui::GetIO();

			if (io.WantCaptureMouse)
				return;
		}

		for (auto& i : viewer->m_interactors)
		{
			i->cursorPosEvent(xpos, ypos);
		}
	}
}

void Viewer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		if (viewer->m_showUi)
		{
			ImGuiIO& io = ImGui::GetIO();

			if (io.WantCaptureMouse)
				return;
		}

		viewer->m_mouseWheel += (float)yoffset; // Use fractional mouse wheel.
	}
}

void Viewer::charCallback(GLFWwindow* window, unsigned int c)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		if (viewer->m_showUi)
		{
			ImGuiIO& io = ImGui::GetIO();

			if (c > 0 && c < 0x10000)
				io.AddInputCharacter((unsigned short)c);

			if (io.WantCaptureKeyboard)
				return;
		}
	}
}


void Viewer::beginFrame()
{
	ImGuiIO& io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	int display_w, display_h;
	glfwGetWindowSize(m_window, &w, &h);
	glfwGetFramebufferSize(m_window, &display_w, &display_h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

	// Setup time step
	double current_time = glfwGetTime();
	io.DeltaTime = m_time > 0.0 ? (float)(current_time - m_time) : (float)(1.0f / 60.0f);
	m_time = current_time;

	// Setup inputs
	// (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
	if (glfwGetWindowAttrib(m_window, GLFW_FOCUSED))
	{
		if (io.WantSetMousePos)
		{
			glfwSetCursorPos(m_window, (double)io.MousePos.x, (double)io.MousePos.y);   // Set mouse position if requested by io.WantMoveMouse flag (used when io.NavMovesTrue is enabled by user and using directional navigation)
		}
		else
		{
			double mouse_x, mouse_y;
			glfwGetCursorPos(m_window, &mouse_x, &mouse_y);
			io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);   // Get mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
		}
	}
	else
	{
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	}

	for (int i = 0; i < 3; i++)
	{
		// If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
		io.MouseDown[i] = m_mousePressed[i] || glfwGetMouseButton(m_window, i) != 0;
		m_mousePressed[i] = false;
	}

	io.MouseWheel = m_mouseWheel;
	m_mouseWheel = 0.0f;

	// Hide OS mouse cursor if ImGui is drawing it
	glfwSetInputMode(m_window, GLFW_CURSOR, io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);

	// Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
	ImGui::NewFrame();
	ImGui::BeginMainMenuBar();
}

void Viewer::endFrame()
{
	static std::list<float> frameratesList;
	frameratesList.push_back(ImGui::GetIO().Framerate);

	while (frameratesList.size() > 64)
		frameratesList.pop_front();

	static float framerates[64];
	int i = 0;
	for (auto v : frameratesList)
		framerates[i++] = v;

	std::stringstream stream;
	stream << std::fixed << std::setprecision(2) << ImGui::GetIO().Framerate << " fps";
	std::string s = stream.str();

	//		ImGui::Begin("Information");
	ImGui::SameLine(ImGui::GetWindowWidth() - 220.0f);
	ImGui::PlotLines(s.c_str(), framerates, frameratesList.size(), 0, 0, 0.0f, 200.0f,ImVec2(128.0f,0.0f));
	//		ImGui::End();

	ImGui::EndMainMenuBar();

	if (m_saveScreenshot)
	{
		std::string basename = scene()->table()->filename();
		size_t pos = basename.rfind('.', basename.length());

		if (pos != std::string::npos)
			basename = basename.substr(0,pos);

		uint i = 0;
		std::string filename;

		for (uint i = 0; i <= 9999; i++)
		{
			std::stringstream ss;
			ss << basename << "-";
			ss << std::setw(4) << std::setfill('0') << i;
			ss << ".png";

			filename = ss.str();

			std::ifstream f(filename.c_str());
			
			if (!f.good())
				break;
		}

		std::cout << "Saving screenshot to " << filename << " ..." << std::endl;

		saveImage(filename);
		m_saveScreenshot = false;
	}

	if (m_showUi)
		renderUi();
}

void Viewer::renderUi()
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();

	int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);

	if (fb_width == 0 || fb_height == 0)
		return;

	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
	glActiveTexture(GL_TEXTURE0);
	GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
	GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
	GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
	GLenum last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
	GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
	GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
	GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
	GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
	GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
	GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
	GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
	GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
	GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
	GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);


	mat4 ortho = glm::ortho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f );

	m_programUi->setUniform("projection", ortho);
	m_programUi->use();
	glBindSampler(0, 0); // Rely on combined texture/sampler state

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawIdx* idx_buffer_offset = 0;

		m_verticesUi->setData((GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
		m_indicesUi->setData((GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);
		
		m_vaoUi->bindElementBuffer(m_indicesUi.get());
		
		auto posBinding = m_vaoUi->binding(0);
		posBinding->setAttribute(0);
		posBinding->setBuffer(m_verticesUi.get(), (size_t)&(((ImDrawVert*)0)->pos), sizeof(ImDrawVert));
		posBinding->setFormat(2, GL_FLOAT, GL_FALSE);
		m_vaoUi->enable(0);

		auto uvBinding = m_vaoUi->binding(1);
		uvBinding->setAttribute(1);
		uvBinding->setBuffer(m_verticesUi.get(), (size_t)&(((ImDrawVert*)0)->uv), sizeof(ImDrawVert));
		uvBinding->setFormat(2, GL_FLOAT, GL_FALSE);
		m_vaoUi->enable(1);

		auto colBinding = m_vaoUi->binding(2);
		colBinding->setAttribute(2);
		colBinding->setBuffer(m_verticesUi.get(), (size_t)&(((ImDrawVert*)0)->col), sizeof(ImDrawVert));
		colBinding->setFormat(4, GL_UNSIGNED_BYTE, GL_TRUE);
		m_vaoUi->enable(2);
		
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				m_vaoUi->drawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);

			}
			idx_buffer_offset += pcmd->ElemCount;
		}
	}

	m_programUi->release();

	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindSampler(0, last_sampler);
	glActiveTexture(last_active_texture);
	glBindVertexArray(last_vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
	glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
	glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
	if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
	if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
	if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, last_polygon_mode[0]);
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
	glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);


}

void Viewer::mainMenu()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Screenshot", "F2"))
			m_saveScreenshot = true;

		if (ImGui::MenuItem("Exit", "Alt+F4"))
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Settings"))
	{
		ImGui::ColorEdit3("Background", (float*)&m_backgroundColor);
		ImGui::ColorEdit3("Point Color", (float*)&m_samplePointColor);
		ImGui::ColorEdit3("Grid Color", (float*)&m_gridColor);
		ImGui::ColorEdit3("Tile Color", (float*)&m_tileColor);

		if (ImGui::BeginMenu("Viewport"))
		{
			if (ImGui::MenuItem("512 x 512"))
				glfwSetWindowSize(m_window, 512, 512);

			if (ImGui::MenuItem("768 x 768"))
				glfwSetWindowSize(m_window, 768, 768);

			if (ImGui::MenuItem("1024 x 1024"))
				glfwSetWindowSize(m_window, 1024, 1024);

			if (ImGui::MenuItem("1280 x 1280"))
				glfwSetWindowSize(m_window, 1280, 1280);

			if (ImGui::MenuItem("1280 x 720"))
				glfwSetWindowSize(m_window, 1280, 720);

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}
}

const char* Viewer::GetClipboardText(void* user_data)
{
	return glfwGetClipboardString((GLFWwindow*)user_data);
}

void Viewer::SetClipboardText(void* user_data, const char* text)
{
	glfwSetClipboardString((GLFWwindow*)user_data, text);
}

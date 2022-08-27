#include "Renderer.h"
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <iostream>
#include <filesystem>

using namespace molumes;
using namespace gl;
using namespace glm;
using namespace globjects;


Renderer::Renderer(Viewer* viewer) : m_viewer(viewer)
{

}

Viewer * Renderer::viewer()
{
	return m_viewer;
}

void Renderer::setEnabled(bool enabled)
{
	m_enabled = enabled;
}

bool Renderer::isEnabled() const
{
	return m_enabled;
}

void Renderer::reloadShaders()
{
	for (auto & p : m_shaderPrograms)
	{
		globjects::debug() << "Reloading shader program " << p.first << " ...";

		for (auto & f : p.second.m_files)
		{
			globjects::debug() << "Reloading shader file " << f->filePath() << " ...";
			f->reload();
		}
	}
}

bool Renderer::createShaderProgram(const std::string & name, std::initializer_list< std::pair<GLenum, std::string> > shaders, std::initializer_list < std::string> shaderIncludes)
{
	globjects::debug() << "Creating shader program " << name << " ...";

	ShaderProgram program;

	for (auto i : shaderIncludes)
	{
		globjects::debug() << "Loading include file " << i << " ...";

		std::filesystem::path path(i);

		auto file = File::create(i);
		auto string = NamedString::create("/" + path.filename().string(), file.get());

		program.m_files.insert(std::move(file));
		program.m_strings.insert(std::move(string));
	}

	for (auto i : shaders)
	{
		globjects::debug() << "Loading shader file " << i.second << " ...";

		auto file = Shader::sourceFromFile(i.second);
		auto source = Shader::applyGlobalReplacements(file.get());
		auto shader = Shader::create(i.first, source.get());

		program.m_program->attach(shader.get());

		program.m_files.insert(std::move(file));
		program.m_sources.insert(std::move(source));
		program.m_shaders.insert(std::move(shader));
	}

	m_shaderPrograms[name] = std::move(program);

	return false;
}

std::unique_ptr<globjects::Texture> Renderer::create2DTexture(GLenum tex, GLenum minFilter, GLenum magFilter, GLenum wrapS, GLenum wrapT,
	gl::GLint level, gl::GLenum internalFormat, const glm::ivec2 & size, gl::GLint border, gl::GLenum format, gl::GLenum type, const gl::GLvoid * data) {

	std::unique_ptr<globjects::Texture> texture = Texture::create(tex);
	texture->setParameter(GL_TEXTURE_MIN_FILTER, minFilter);
	texture->setParameter(GL_TEXTURE_MAG_FILTER, magFilter);
	texture->setParameter(GL_TEXTURE_WRAP_S, wrapS);
	texture->setParameter(GL_TEXTURE_WRAP_T, wrapT);
	texture->image2D(level, internalFormat, size, border, format, type, data);

	return texture;
}

globjects::Program * Renderer::shaderProgram(const std::string & name)
{
	return m_shaderPrograms[name].m_program.get();
}


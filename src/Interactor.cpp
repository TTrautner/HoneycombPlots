#include "Interactor.h"
using namespace honeycomb;

Interactor::Interactor(Viewer* viewer) : m_viewer(viewer)
{

}

Viewer * Interactor::viewer()
{
	return m_viewer;
}

void Interactor::setEnabled(bool enabled)
{
	m_enabled = enabled;
}

bool Interactor::isEnabled() const
{
	return m_enabled;
}

void Interactor::framebufferSizeEvent(int width, int height)
{
}

void Interactor::keyEvent(int key, int scancode, int action, int mods)
{
}

void Interactor::mouseButtonEvent(int button, int action, int mods)
{
}

void Interactor::cursorPosEvent(double xpos, double ypos)
{
}

void Interactor::display()
{
}


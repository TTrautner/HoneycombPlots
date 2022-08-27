#pragma once
#include "Interactor.h"
#include <glm/glm.hpp>

namespace molumes
{
	class Viewer;

	class CameraInteractor : public Interactor
	{
	public:
		CameraInteractor(Viewer * viewer);
		virtual void framebufferSizeEvent(int width, int height);
		virtual void keyEvent(int key, int scancode, int action, int mods);
		virtual void mouseButtonEvent(int button, int action, int mods);
		virtual void cursorPosEvent(double xpos, double ypos);
		virtual void display();

		void resetProjectionTransform();
		void resetViewTransform();
		void setLightPosition();

	private:

		glm::vec3 arcballVector(double x, double y);

		float m_fov = glm::radians(60.0f);
		float m_near = 0.125f;
		float m_far = 32768.0f;
		float m_distance = 2.0f*sqrt(3.0f);
		bool m_perspective = false;

		bool m_light = false;
		bool m_rotating = false;
		bool m_scaling = false;
		bool m_panning = false;
		bool m_benchmark = false;
		double m_startTime = 0.0;
		glm::uint m_frameCount = 0;
		double m_xPrevious = 0.0, m_yPrevious = 0.0;
		double m_xCurrent = 0.0, m_yCurrent = 0.0;
	};

}
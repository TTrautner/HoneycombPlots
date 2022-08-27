#pragma once

namespace molumes
{
	class Viewer;

	class Interactor
	{
	public:
		Interactor(Viewer* viewer);
		Viewer * viewer();

		void setEnabled(bool enabled);
		bool isEnabled() const;

		virtual void framebufferSizeEvent(int width, int height);
		virtual void keyEvent(int key, int scancode, int action, int mods);
		virtual void mouseButtonEvent(int button, int action, int mods);
		virtual void cursorPosEvent(double xpos, double ypos);
		virtual void display();

	private:
		Viewer* m_viewer;
		bool m_enabled = true;
	};

}
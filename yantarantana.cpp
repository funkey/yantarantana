#include <iostream>
#include <memory>
#include <sg_gui/Window.h>

class TestView : public sg::Agent<
		TestView,
		sg::Accepts<
			sg_gui::Draw
		>
>{

public:

	void onSignal(sg_gui::Draw& signal) {

		glBegin(GL_LINES);
		glVertex2f(0, 0);
		glVertex2f(100, 0);
		glVertex2f(100, 100);
		glVertex2f(0, 100);
		glVertex2f(0, 0);
		glVertex2f(100, 100);
		glEnd();
	}
};

int main(int argc, char** argv) {

	auto window = std::make_shared<sg_gui::Window>("yantarantana");
	auto test   = std::make_shared<TestView>();

	window->add(test);

	window->processEvents();
}

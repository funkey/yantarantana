#include <iostream>
#include <memory>
#include <sg_gui/Window.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include <util/exceptions.h>

class TestView : public sg::Agent<
		TestView,
		sg::Accepts<
			sg_gui::Draw,
			sg_gui::PenDown,
			sg_gui::PenUp,
			sg_gui::FingerDown,
			sg_gui::FingerUp
		>
>{

public:

	void onSignal(sg_gui::Draw& signal) {

		std::cout << "[Test] drawing" << std::endl;

		glBegin(GL_LINES);
		glVertex2f(0, 0);
		glVertex2f(100, 0);
		glVertex2f(100, 100);
		glVertex2f(0, 100);
		glVertex2f(0, 0);
		glVertex2f(100, 100);
		glEnd();
	}

	void onSignal(sg_gui::PenDown& signal) {

		std::cout << "pen down at " << signal.ray << ", button " << signal.button << std::endl;
	}

	void onSignal(sg_gui::PenUp& signal) {

		std::cout << "pen up at " << signal.ray << ", button " << signal.button << std::endl;
	}

	void onSignal(sg_gui::FingerDown& signal) {

		std::cout << "finger " << signal.id << " down at " << signal.ray << std::endl;
	}

	void onSignal(sg_gui::FingerUp& signal) {

		std::cout << "finger " << signal.id << " up at " << signal.ray << std::endl;
	}
};

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		sg_gui::WindowMode mode;
		mode.fullscreen = true;
		auto window = std::make_shared<sg_gui::Window>("yantarantana", mode);
		auto test = std::make_shared<TestView>();

		window->add(test);

		window->processEvents();

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
		return 1;
	}

	return 0;
}

#include <iostream>
#include <memory>
#include <sg_gui/Window.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include <util/exceptions.h>

class TestView : public sg::Agent<
		TestView,
		sg::Accepts<
			sg_gui::Draw
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
};

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		auto window = std::make_shared<sg_gui::Window>("yantarantana");
		auto test = std::make_shared<TestView>();

		window->add(test);

		window->processEvents();

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
		return 1;
	}

	return 0;
}

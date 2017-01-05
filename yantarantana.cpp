#include <iostream>
#include <memory>
#include <sg_gui/Window.h>
#include <sg_gui/ZoomView.h>
#include <gui/DocumentView.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include <util/exceptions.h>

class TestView : public sg::Agent<TestView, sg::Accepts<sg_gui::Draw>> {

public:

	void onSignal(sg_gui::Draw& signal) {

		glBegin(GL_LINES);
		glVertex2d(0,0);
		glVertex2d(100,0);
		glVertex2d(100,100);
		glVertex2d(0,100);
		glVertex2d(0,0);
		glEnd();
	}
};

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		sg_gui::WindowMode mode;
#ifdef NDEBUG
		mode.fullscreen = true;
#endif
		auto window = std::make_shared<sg_gui::Window>("yantarantana", mode);
		auto zoomView = std::make_shared<sg_gui::ZoomView>();
		auto documentView = std::make_shared<DocumentView>();
		auto testView = std::make_shared<TestView>();

		window->add(zoomView);
		zoomView->add(documentView);
		zoomView->add(testView);

		auto document = std::make_shared<Document>();
		document->createPage(util::point<double,2>(0,0), util::point<double,2>(100,100));
		document->createPage(util::point<double,2>(100,100), util::point<double,2>(100,100));
		document->createNewStroke(util::point<double,2>(0,0), 1.0, 0);
		document->addStrokePoint(util::point<double,2>(100,100), 1.0, 1);
		document->finishCurrentStroke();
		documentView->setDocument(document);

		window->processEvents();

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
		return 1;
	}

	return 0;
}

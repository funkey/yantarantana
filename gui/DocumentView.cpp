#include "DocumentView.h"

DocumentView::DocumentView() :
	_documentPainter(
			std::make_shared<SkiaDocumentPainter>(
				sg_gui::skia_pixel_t(255, 0, 255))),
	_documentCleanUpPainter(
			std::make_shared<SkiaDocumentPainter>(
				sg_gui::skia_pixel_t(255, 255, 0))) {}

void
DocumentView::onSignal(sg_gui::Draw& signal) {

	util::box<float,2> roi = signal.roi();

	if (
			!_texture ||
			_currentRoi.width() != roi.width() ||
			_currentRoi.height() != roi.height()) {

		_currentRoi = roi;
		_texture = std::make_shared<TorusTexture>(roi);
		_texture->setBackgroundRasterizer(_documentCleanUpPainter);
	}

	_texture->render(roi, *_documentPainter);
}

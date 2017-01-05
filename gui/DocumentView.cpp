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

	_texture.render(signal.roi(), *_documentPainter);
}

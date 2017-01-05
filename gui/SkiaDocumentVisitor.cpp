#include "SkiaDocumentVisitor.h"

logger::LogChannel skiadocumentvisitorlog("skiadocumentvisitorlog", "[SkiaDocumentVisitor] ");

SkiaDocumentVisitor::SkiaDocumentVisitor() :
	_pixelsPerDeviceUnit(1.0, 1.0),
	_pixelOffset(0, 0) {}

void
SkiaDocumentVisitor::prepare(const util::box<DocumentPrecision,2>& roi) {

	if (!roi.isZero()) {

		// clip outside our responsibility
		_canvas->clipRect(SkRect::MakeLTRB(roi.min().x(), roi.min().y(), roi.max().x(), roi.max().y()));

		// roi is given in device units, transform it into document coordinates 
		// according to the current device transformation

		// transform roi downwards
		setRoi((roi - _pixelOffset)/_pixelsPerDeviceUnit);

	} else {

		// set the zero roi
		setRoi(roi);
	}

	// apply the device transformation
	_canvas->save();
	_canvas->translate(_pixelOffset.x(), _pixelOffset.y());
	_canvas->scale(_pixelsPerDeviceUnit.x(), _pixelsPerDeviceUnit.y());
}

void
SkiaDocumentVisitor::finish() {

	// restore original transformation
	_canvas->restore();
}

void
SkiaDocumentVisitor::enter(DocumentElement& element) {

	DocumentTreeRoiVisitor::enter(element);

	const util::point<DocumentPrecision,2>& shift = element.getShift();
	const util::point<DocumentPrecision,2>& scale = element.getScale();

	_canvas->save();

	// apply the element transformation
	_canvas->translate(shift.x(), shift.y());
	_canvas->scale(scale.x(), scale.y());
}

void
SkiaDocumentVisitor::leave(DocumentElement& element) {

	DocumentTreeRoiVisitor::leave(element);

	_canvas->restore();
}

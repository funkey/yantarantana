#include <SkPath.h>
#include <SkMaskFilter.h>
#include <SkBlurMaskFilter.h>

#include <util/Logger.h>
#include "SkiaDocumentPainter.h"

logger::LogChannel skiadocumentpainterlog("skiadocumentpainterlog", "[SkiaDocumentPainter] ");

SkiaDocumentPainter::SkiaDocumentPainter(
		const sg_gui::skia_pixel_t& clearColor,
		bool drawPaper) :
	_clearColor(clearColor),
	_drawPaper(drawPaper),
	_drawnUntilStrokePoint(0),
	_drawnUntilStrokePointTmp(0),
	_canvasCleared(false),
	_canvasClearedTmp(false),
	_paperDrawn(false),
	_paperDrawnTmp(false),
	_incremental(false) {}

void
SkiaDocumentPainter::draw(SkCanvas& canvas, const util::box<DocumentPrecision,2>& roi) {

	LOG_DEBUG(skiadocumentpainterlog) << "drawing document in " << roi << std::endl;

	setCanvas(canvas);

	// prepare the visitor to draw only within roi
	prepare(roi);

	// get the number of pixels that correspond to one skia unit, which is one 
	// millimeter in our case
	double scale = canvas.getTotalMatrix().getScaleX();

	LOG_DEBUG(skiadocumentpainterlog) << "pixel scale is " << scale << std::endl;

	bool qualitWasAuto = (getQuality() == Auto);

	if (qualitWasAuto) {

		if (scale < 1)
			setQuality(Worst);
		else if (scale < 5)
			setQuality(Medium);
		else
			setQuality(Best);
	}

	{
		// make sure reading access to the stroke points are safe
		boost::shared_lock<boost::shared_mutex> lock(getDocument().getStrokePoints().getMutex());

		// go visit the document
		getDocument().accept(*this);
	}

	if (qualitWasAuto)
		setQuality(Auto);

	finish();
}

bool
SkiaDocumentPainter::needRedraw() {

	if (!_canvasCleared || !_paperDrawn)
		return true;

	if (getDocument().numStrokes() == 0)
		return false;

	// did we draw all the stroke points?
	if (_drawnUntilStrokePoint == getDocument().getStrokePoints().size())
		return false;

	return true;
}

void
SkiaDocumentPainter::visit(Document&) {

	LOG_DEBUG(skiadocumentpainterlog) << "visiting document" << std::endl;

	// reset temporal memory about what we drew already
	_drawnUntilStrokePointTmp = _drawnUntilStrokePoint;
	_canvasClearedTmp = _canvasCleared;
	_paperDrawnTmp = _paperDrawn;

	// clear the surface, respecting the clipping
	if (!_incremental || !_canvasCleared) {
		getCanvas().drawColor(SkColorSetRGB(_clearColor.blue, _clearColor.green, _clearColor.red));
		_canvasClearedTmp = true;
	}
}

void
SkiaDocumentPainter::visit(Page& page) {

	LOG_ALL(skiadocumentpainterlog) << "visiting page with roi " << getRoi() << std::endl;

	if ((_incremental && _paperDrawn) || !_drawPaper)
		return;

	// even though the roi might intersect the page's content, it might not 
	// intersect the paper -- check that here (in page coordinates)
	if (!getRoi().isZero() && !getRoi().intersects(
			util::box<PagePrecision,2>(
					-page.getBorderSize(),
					-page.getBorderSize(),
					page.getSize().x() + page.getBorderSize(),
					page.getSize().y() + page.getBorderSize()))) {

		LOG_ALL(skiadocumentpainterlog) << "page does not intersect roi (but content probably does)" << std::endl;
		return;
	}

	SkPath outline;
	const util::point<PagePrecision,2>& pageSize = page.getSize();
	outline.lineTo(pageSize.x(), 0);
	outline.lineTo(pageSize.x(), pageSize.y());
	outline.lineTo(0, pageSize.y());
	outline.lineTo(0, 0);
	outline.close();

	const char pageRed   = 255;
	const char pageGreen = 255;
	const char pageBlue  = 245;

	const char gridRed   = 55;
	const char gridGreen = 55;
	const char gridBlue  = 45;

	const double gridSizeX = 5.0;
	const double gridSizeY = 5.0;
	const double gridWidth = 0.03;

	SkPaint paint;

	// shadow-like thingie

	// we blur the boundary with a "standard deviation" of 1/10 of the border 
	// size of the paper -- this makes sure that at the end of the border there 
	// is almost no trace of the blur anymore
	auto maskFilter = SkBlurMaskFilter::Make(kOuter_SkBlurStyle, page.getBorderSize()/10.0, SkBlurMaskFilter::kNone_BlurFlag);
	paint.setMaskFilter(maskFilter);
	paint.setColor(SkColorSetRGB(0.5*pageRed, 0.5*pageGreen, 0.5*pageBlue));
	getCanvas().drawPath(outline, paint);
	paint.setMaskFilter(NULL);

	// the paper

	paint.setStyle(SkPaint::kFill_Style);
	paint.setColor(SkColorSetRGB(pageRed, pageGreen, pageBlue));
	getCanvas().drawPath(outline, paint);

	// the grid

	paint.setStyle(SkPaint::kStroke_Style);
	paint.setColor(SkColorSetRGB(gridRed, gridGreen, gridBlue));
	paint.setStrokeWidth(gridWidth);
	paint.setAntiAlias(true);

	double startX = std::max(getRoi().min().x(), 0.0);
	double startY = std::max(getRoi().min().y(), 0.0);
	double endX   = getRoi().isZero() ? pageSize.x() : std::min(getRoi().max().x(), pageSize.x());
	double endY   = getRoi().isZero() ? pageSize.y() : std::min(getRoi().max().y(), pageSize.y());

	for (int x = (int)ceil(startX/gridSizeX)*gridSizeX; x <= (int)floor(endX/gridSizeX)*gridSizeX; x += gridSizeX)
		getCanvas().drawLine(x, startY, x, endY, paint);
	for (int y = (int)ceil(startY/gridSizeY)*gridSizeY; y <= (int)floor(endY/gridSizeY)*gridSizeY; y += gridSizeY)
		getCanvas().drawLine(startX, y, endX, y, paint);

	// the outline

	paint.setColor(SkColorSetRGB(0.5*pageRed, 0.5*pageGreen, 0.5*pageBlue));
	paint.setStrokeWidth(gridWidth);
	paint.setStrokeCap(SkPaint::kRound_Cap);
	paint.setStrokeJoin(SkPaint::kRound_Join);
	getCanvas().drawPath(outline, paint);

	_paperDrawnTmp = true;
}

void
SkiaDocumentPainter::visit(Stroke& stroke) {

	unsigned long end = stroke.end();

	// end is one beyond the last point of the stroke. _drawnUntilStrokePoint is 
	// one beyond the last point until which we drew already.  If end is less or 
	// equal what we drew, there is nothing to do.

	// drawn already?
	if (_incremental && end <= _drawnUntilStrokePoint)
		return;

	unsigned long begin = stroke.begin();

	if (_incremental && _drawnUntilStrokePoint > 0 && _drawnUntilStrokePoint - 1 > stroke.begin())
		begin = _drawnUntilStrokePoint - 1;

	LOG_ALL(skiadocumentpainterlog)
			<< "drawing stroke (" << stroke.begin() << " - " << stroke.end()
			<< ") , starting from point " << begin << " until " << end << std::endl;

	if (getQuality() < Best) {

		_worseStrokePainter.setQuality(getQuality());
		_worseStrokePainter.draw(getCanvas(), getDocument().getStrokePoints(), stroke, getRoi(), begin, end);

	} else
		_bestStrokePainter.draw(getCanvas(), getDocument().getStrokePoints(), stroke, getRoi(), begin, end);

	// remember until which point we drew already in our temporary memory
	_drawnUntilStrokePointTmp = std::max(_drawnUntilStrokePointTmp, end);
}


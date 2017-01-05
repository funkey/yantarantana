#ifndef YANTA_SKIA_STROKE_BALL_PAINTER_H__
#define YANTA_SKIA_STROKE_BALL_PAINTER_H__

#include <util/box.hpp>

// forward declarations
class SkCanvas;
class Stroke;
class StrokePoints;

class SkiaStrokeBallPainter {

public:

	SkiaStrokeBallPainter();

	void draw(
		SkCanvas& canvas,
		const StrokePoints& strokePoints,
		const Stroke& stroke,
		const util::box<double,2>& roi,
		unsigned long beginStroke = 0,
		unsigned long endStroke   = 0);

private:

	double widthPressureCurve(double pressure);
	double alphaPressureCurve(double pressure);
};

#endif // YANTA_SKIA_STROKE_BALL_PAINTER_H__


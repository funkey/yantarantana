#ifndef YANTA_SKIA_STROKE_PATH_EFFECT_PAINTER_H__
#define YANTA_SKIA_STROKE_PATH_EFFECT_PAINTER_H__

#include <util/box.hpp>

// forward declarations
class SkCanvas;
class SkPathEffect;
class Stroke;
class StrokePoints;

class SkiaStrokePathEffectPainter {

public:

	SkiaStrokePathEffectPainter(SkCanvas& canvas, const StrokePoints& strokePoints);

	void draw(
		const Stroke& stroke,
		const util::box<double,2>& roi,
		unsigned long beginStroke = 0,
		unsigned long endStroke   = 0);

private:

	sk_sp<SkPathEffect> makePathEffect(
			const Stroke& stroke,
			unsigned int beginStroke,
			unsigned int endStroke);

	double widthPressureCurve(double pressure);

	double alphaPressureCurve(double pressure);

	SkCanvas&           _canvas;
	const StrokePoints& _strokePoints;
};

#endif // YANTA_SKIA_STROKE_PATH_EFFECT_PAINTER_H__


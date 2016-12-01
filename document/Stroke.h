#ifndef STROKE_H__
#define STROKE_H__

#include <vector>
#include <util/point.hpp>
#include <util/box.hpp>

#include "DocumentElement.h"
#include "StrokePoints.h"
#include "Style.h"

class Stroke : public DocumentElement {

public:

	UTIL_TREE_VISITABLE();

	Stroke(unsigned long begin = 0) :
		_finished(false),
		_begin(begin),
		_end(0) {}

	/**
	 * Get the number of stroke points in this stroke.
	 */
	inline unsigned long size() const {

		if (_end <= _begin)
			return 0;

		return _end - _begin;
	}

	/**
	 * Set the style this stroke should be drawn with.
	 */
	inline void setStyle(const Style& style) { _style = style; }

	/**
	 * Get the style this stroke is supposed to be drawn with.
	 */
	inline const Style& getStyle() const {

		return _style;
	}

	/**
	 * Get the style this stroke is supposed to be drawn with.
	 */
	inline Style& getStyle() {

		return _style;
	}

	/**
	 * Set the first stroke point of this stroke.
	 */
	inline void setBegin(unsigned long index) {

		_begin = index;
	}

	/**
	 * Set the last (exclusive) stroke point of this stroke.
	 */
	inline void setEnd(unsigned long index, const StrokePoints& points) {

		// update bounding box
		for (unsigned int i = std::max(_begin, _end); i < index; i++) {

			const StrokePoint& point = points[i];

			fitBoundingBox(util::box<DocumentPrecision,2>(
					point.position.x() - _style.width(),
					point.position.y() - _style.width(),
					point.position.x() + _style.width(),
					point.position.y() + _style.width()));
		}

		// update end pointer
		_end = index;
	}

	/**
	 * Get the index of the first point of this stroke.
	 */
	inline unsigned long begin() const { return _begin; }

	/**
	 * Get the index of the point after the last point of this stroke.
	 */
	inline unsigned long end() const { return _end; }

	/**
	 * Finish this stroke. Computes the bounding box.
	 */
	inline void finish() {

		_finished = true;
	}

	/**
	 * Test, whether this stroke was finished already.
	 */
	inline bool finished() const {

		return _finished;
	}

	/**
	 * Recompute the bounding box of this stroke.
	 */
	inline void updateBoundingBox(const StrokePoints& points) {

		resetBoundingBox();

		for (unsigned int i = _begin; i < _end; i++) {

			const StrokePoint& point = points[i];

			fitBoundingBox(util::box<DocumentPrecision,2>(
					point.position.x() - _style.width(),
					point.position.y() - _style.width(),
					point.position.x() + _style.width(),
					point.position.y() + _style.width()));
		}
	}

private:

	Style _style;

	bool _finished;

	// indices of the stroke points in the global point list
	unsigned long _begin;
	unsigned long _end;
};

#endif // STROKE_H__


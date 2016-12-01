#ifndef YANTA_TRANSFORMABLE_H__
#define YANTA_TRANSFORMABLE_H__

#include <util/point.hpp>
#include <util/box.hpp>
#include "Transformation.h"

template <typename Precision>
class Transformable {

public:

	Transformable() :
		_boundingBox(0, 0, 0, 0) {}

	/**
	 * Get the shift of this object.
	 */
	inline const util::point<Precision,2>& getShift() const {

		return _transformation.getShift();
	}

	/**
	 * Get the scale of this object.
	 */
	inline const util::point<Precision,2>& getScale() const {

		return _transformation.getScale();
	}

	/**
	 * Get the transformation of this object.
	 */
	inline const Transformation<Precision>& getTransformation() const {

		return _transformation;
	}

	/**
	 * Set the shift of this object.
	 */
	inline void setShift(const util::point<Precision,2>& shift) {

		_boundingBox += shift - _transformation.getShift();
		_transformation.setShift(shift);
	}

	/**
	 * Set the scale of this object.
	 */
	inline void setScale(const util::point<Precision,2>& scale) {

		_boundingBox *= scale/_transformation.getScale();
		_transformation.setScale(scale);
	}

	/**
	 * Set the transformation of this object.
	 */
	inline void setTransformation(const Transformation<Precision>& transformation) {

		_transformation = transformation;
	}

	/**
	 * Shift this object.
	 */
	inline void shift(const util::point<Precision,2>& shift) {

		_transformation.shift(shift);
		_boundingBox += shift;
	}

	/**
	 * Scale this object.
	 */
	inline void scale(const util::point<Precision,2>& scale) {

		_transformation.scale(scale);
		_boundingBox *= scale;
	}

	/**
	 * Set the bounding box to be empty.
	 */
	void resetBoundingBox() {

		_boundingBox = util::box<Precision,2>(0, 0, 0, 0);
	}

	/**
	 * Change the bounding box to fit the given area after scaling and shifting.
	 */
	void fitBoundingBox(const util::box<Precision,2>& r) {

		if (_boundingBox.isZero())
			_boundingBox = r*_transformation.getScale() + _transformation.getShift();
		else
			_boundingBox.fit(r*_transformation.getScale() + _transformation.getShift());
	}

	/**
	 * Change the bounding box to fit the given point after scaling and 
	 * shifting.
	 */
	void fitBoundingBox(const util::point<Precision,2>& p) {

		if (_boundingBox.isZero())
			_boundingBox = util::box<Precision,2>(p.x(), p.y(), p.x(), p.y())*_transformation.getScale() + _transformation.getShift();
		else
			_boundingBox.fit(p*_transformation.getScale() + _transformation.getShift());
	}

	/**
	 * Get the bounding box of this object, accordingly scaled and shifted.
	 */
	const util::box<Precision,2>& getBoundingBox() const {

		return _boundingBox;
	}

private:

	Transformation<Precision> _transformation;

	util::box<Precision,2> _boundingBox;
};


#endif // YANTA_TRANSFORMABLE_H__


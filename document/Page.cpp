#include "Document.h"
#include "Page.h"
#include <util/Logger.h>

logger::LogChannel pagelog("pagelog", "[Page] ");

Page::Page(
		Document* document,
		const util::point<DocumentPrecision,2>& position,
		const util::point<PagePrecision,2>&     size) :
	_size(size),
	_borderSize(15),
	_pageBoundingBox(position.x(), position.y(), position.x() + size.x(), position.y() + size.y()),
	_strokePoints(document->getStrokePoints()) {

	fitBoundingBox(util::box<PagePrecision,2>(-getBorderSize(), -getBorderSize(), size.x() + getBorderSize(), size.y() + getBorderSize()));
	shift(position);
}

Page&
Page::operator=(const Page& other) {

	// copy the elements of the container
	DocumentElementContainer<Stroke>::operator=(other);

	_size            = other._size;
	_pageBoundingBox = other._pageBoundingBox;

	// we don't copy the stroke points, since they might belong to another 
	// document
	return *this;
}

void
Page::createNewStroke(
		const util::point<DocumentPrecision,2>& start,
		double                              pressure,
		unsigned long                       timestamp) {

	createNewStroke(_strokePoints.size());
	addStrokePoint(start, pressure, timestamp);
}

void
Page::createNewStroke(unsigned long begin) {

	LOG_ALL(pagelog) << "creating a new stroke starting at " << begin << std::endl;

	// if this happens in the middle of a draw, finish the unfinished
	if (numStrokes() > 0 && !currentStroke().finished())
		currentStroke().finish();

	add(Stroke(begin));
}

void
Page::recomputeBoundingBox() {

	resetBoundingBox();
	fitBoundingBox(util::box<PagePrecision,2>(-getBorderSize(), -getBorderSize(), _size.x() + getBorderSize(), _size.y() + getBorderSize()));
	for_each(UpdateBoundingBox(*this));
}

util::box<DocumentPrecision,2>
Page::erase(const util::point<DocumentPrecision,2>& begin, const util::point<DocumentPrecision,2>& end) {

	//LOG_ALL(pagelog) << "erasing strokes that intersect line from " << begin << " to " << end << std::endl;

	// get the erase line in page coordinates
	const util::point<PagePrecision,2> pageBegin = toPageCoordinates(begin);
	const util::point<PagePrecision,2> pageEnd   = toPageCoordinates(end);

	util::box<PagePrecision,2> eraseBoundingBox(
			pageBegin.x(),
			pageBegin.y(),
			pageBegin.x(),
			pageBegin.y());
	eraseBoundingBox.fit(pageEnd);

	util::box<PagePrecision,2> changedArea(0, 0, 0, 0);

	unsigned int n = numStrokes();

	for (unsigned int i = 0; i < n; i++)
		if (getStroke(i).getBoundingBox().intersects(eraseBoundingBox)) {

			//LOG_ALL(pagelog) << "stroke " << i << " is close to the erase position" << std::endl;

			util::box<PagePrecision,2> changedStrokeArea = erase(getStroke(i), pageBegin, pageEnd);

			if (changedArea.isZero()) {

				changedArea = changedStrokeArea;

			} else {

				if (!changedStrokeArea.isZero()) {

					changedArea.min().x() = std::min(changedArea.min().x(), changedStrokeArea.min().x());
					changedArea.min().y() = std::min(changedArea.min().y(), changedStrokeArea.min().y());
					changedArea.max().x() = std::max(changedArea.max().x(), changedStrokeArea.max().x());
					changedArea.max().y() = std::max(changedArea.max().y(), changedStrokeArea.max().y());
				}
			}
		}

	LOG_ALL(pagelog) << "changed area is " << changedArea << std::endl;

	// transform the changed area to document coordinates
	return toDocumentCoordinates(changedArea);
}

util::box<DocumentPrecision,2>
Page::erase(const util::point<PagePrecision,2>& position, PagePrecision radius) {

	//LOG_ALL(pagelog) << "erasing at " << position << " with radius " << radius << std::endl;

	// get the erase position in page coordinates
	const util::point<PagePrecision,2> pagePosition = toPageCoordinates(position);

	util::box<PagePrecision,2> eraseBoundingBox(
			pagePosition.x() - radius,
			pagePosition.y() - radius,
			pagePosition.x() + radius,
			pagePosition.y() + radius);

	util::box<PagePrecision,2> changedArea(0, 0, 0, 0);

	unsigned int n = numStrokes();

	for (unsigned int i = 0; i < n; i++)
		if (getStroke(i).getBoundingBox().intersects(eraseBoundingBox)) {

			//LOG_ALL(pagelog) << "stroke " << i << " is close to the erase pagePosition" << std::endl;

			util::box<PagePrecision,2> changedStrokeArea = erase(&getStroke(i), pagePosition, radius*radius);

			if (changedArea.isZero()) {

				changedArea = changedStrokeArea;

			} else {

				if (!changedStrokeArea.isZero()) {

					changedArea.min().x() = std::min(changedArea.min().x(), changedStrokeArea.min().x());
					changedArea.min().y() = std::min(changedArea.min().y(), changedStrokeArea.min().y());
					changedArea.max().x() = std::max(changedArea.max().x(), changedStrokeArea.max().x());
					changedArea.max().y() = std::max(changedArea.max().y(), changedStrokeArea.max().y());
				}
			}
		}

	LOG_ALL(pagelog) << "changed area is " << changedArea << std::endl;

	// transform the changed area to document coordinates
	return toDocumentCoordinates(changedArea);
}

util::box<PagePrecision,2>
Page::erase(Stroke* stroke, const util::point<PagePrecision,2>& center, PagePrecision radius2) {

	util::box<PagePrecision,2> changedArea(0, 0, 0, 0);

	if (stroke->begin() == stroke->end())
		return changedArea;

	unsigned long begin = stroke->begin();
	unsigned long end   = stroke->end() - 1;

	LOG_ALL(pagelog) << "testing stroke lines " << begin << " until " << (end - 1) << std::endl;

	Style style = stroke->getStyle();
	bool wasErasing = false;

	// for each line in the stroke
	for (unsigned long i = begin; i < end; i++) {

		// this line should be erased
		if (intersectsErasorCircle(_strokePoints[i].position, _strokePoints[i+1].position, center, radius2)) {

			LOG_ALL(pagelog) << "line " << i << " needs to be erased" << std::endl;

			// update the changed area
			if (changedArea.isZero()) {

				changedArea.min().x() = _strokePoints[i].position.x();
				changedArea.min().y() = _strokePoints[i].position.y();
				changedArea.max().x() = _strokePoints[i].position.x();
				changedArea.max().y() = _strokePoints[i].position.y();
			}

			changedArea.fit(_strokePoints[i].position);
			changedArea.fit(_strokePoints[i+1].position);

			if (changedArea.isZero())
				LOG_ERROR(pagelog) << "the change area is empty for line " << _strokePoints[i].position << " -- " << _strokePoints[i+1].position << std::endl;

			// if this is the first line to delete, we have to split
			if (!wasErasing) {

				LOG_ALL(pagelog) << "this is the first line to erase on this stroke" << std::endl;

				stroke->setEnd(i+1, _strokePoints);
				stroke->finish();
				wasErasing = true;
			}

		// this line should be kept
		} else if (wasErasing) {

			LOG_ALL(pagelog) << "line " << i << " is the next line not to erase on this stroke" << std::endl;

			createNewStroke(i);
			stroke = &(currentStroke());
			stroke->setStyle(style);
			wasErasing = false;
		}
	}

	// if we didn't split at all, this is a no-op, otherwise we finish the last 
	// stroke
	if (!wasErasing) {

		stroke->setEnd(end+1, _strokePoints);
		stroke->finish();
	}

	// increase the size of the changedArea (if there is one) by the style width
	if (!changedArea.isZero()) {

		changedArea.min().x() -= style.width();
		changedArea.min().y() -= style.width();
		changedArea.max().x() += style.width();
		changedArea.max().y() += style.width();
	}

	LOG_ALL(pagelog) << "done erasing this stroke, changed area is " << changedArea << std::endl;

	return changedArea;
}

util::box<PagePrecision,2>
Page::erase(Stroke& stroke, const util::point<PagePrecision,2>& lineBegin, const util::point<PagePrecision,2>& lineEnd) {

	util::box<PagePrecision,2> changedArea(0, 0, 0, 0);

	if (stroke.begin() == stroke.end())
		return changedArea;

	unsigned long begin = stroke.begin();
	unsigned long end   = stroke.end() - 1;

	LOG_ALL(pagelog) << "testing stroke lines " << begin << " until " << (end - 1) << std::endl;

	util::point<PagePrecision,2> startPoint = _strokePoints[begin].position*stroke.getScale() + stroke.getShift();

	// for each line in the stroke
	for (unsigned long i = begin; i < end; i++) {

		util::point<PagePrecision,2> endPoint = _strokePoints[i+1].position*stroke.getScale() + stroke.getShift();

		// this line should be erased
		if (intersectLines(
				startPoint,
				endPoint - startPoint,
				lineBegin,
				lineEnd - lineBegin)) {

			LOG_ALL(pagelog) << "this stroke needs to be erased" << std::endl;

			changedArea = stroke.getBoundingBox();

			// make this an empty stroke
			stroke.setEnd(begin, _strokePoints);
			stroke.finish();

			break;
		}

		startPoint = endPoint;
	}

	return changedArea;
}

bool
Page::intersectsErasorCircle(
		const util::point<PagePrecision,2> lineStart,
		const util::point<PagePrecision,2> lineEnd,
		const util::point<PagePrecision,2> center,
		PagePrecision radius2) {

	// if either of the points are in the circle, the line intersects
	util::point<PagePrecision,2> diff = center - lineStart;
	if (diff.x()*diff.x() + diff.y()*diff.y() < radius2)
		return true;
	diff = center - lineEnd;
	if (diff.x()*diff.x() + diff.y()*diff.y() < radius2)
		return true;

	// see if the closest point on the line is in the circle

	// the line
	util::point<PagePrecision,2> lineVector = lineEnd - lineStart;
	PagePrecision lenLineVector = sqrt(lineVector.x()*lineVector.x() + lineVector.y()*lineVector.y());

	// unit vector in the line's direction
	util::point<PagePrecision,2> lineDirection = lineVector/lenLineVector;

	// the direction to the erasor
	util::point<PagePrecision,2> erasorVector = center - lineStart;
	PagePrecision lenErasorVector2 = erasorVector.x()*erasorVector.x() + erasorVector.y()*erasorVector.y();

	// the dotproduct gives the distance from _strokePoints[i] to the closest 
	// point on the line to the erasor
	PagePrecision a = lineDirection.x()*erasorVector.x() + lineDirection.y()*erasorVector.y();

	// if a is beyond the beginning of end of the line, the line does not 
	// intersect the circle (since the beginning and end are not in the circle)
	if (a <= 0 || a >= lenLineVector)
		return false;

	// get the distance of the closest point to the center
	PagePrecision erasorDistance2 = lenErasorVector2 - a*a;

	if (erasorDistance2 < radius2)
		return true;

	return false;
}

bool
Page::intersectLines(
		const util::point<PagePrecision,2>& p,
		const util::point<PagePrecision,2>& r,
		const util::point<PagePrecision,2>& q,
		const util::point<PagePrecision,2>& s) {

	// Line 1 is p + t*r, line 2 is q + u*s. We want to find t and u, such that 
	// p + t*r = q + u*s.

	// cross product between r and s
	PagePrecision rXs = r.x()*s.y() - r.y()*s.x();

	// vector from p to q
	const util::point<PagePrecision,2> pq = q - p;

	// cross product of pq with s and r
	PagePrecision pqXs = pq.x()*s.y() - pq.y()*s.x();
	PagePrecision pqXr = pq.x()*r.y() - pq.y()*r.x();

	PagePrecision t = pqXs/rXs;
	PagePrecision u = pqXr/rXs;

	// only if both t and u are between 0 and 1 the lines intersected
	return t >= 0 && t <= 1 && u >= 0 && u <= 1;
}

#ifndef YANTA_PAGE_H__
#define YANTA_PAGE_H__

#include <util/tree.h>

#include "DocumentElementContainer.h"
#include "Precision.h"
#include "Stroke.h"
#include "StrokePoints.h"

// forward declaration
class Document;

class Page : public DocumentElementContainer<Stroke> {

public:

	UTIL_TREE_VISITABLE();

	Page(
			Document* document,
			const util::point<DocumentPrecision,2>& position,
			const util::point<PagePrecision,2>&   size);

	Page& operator=(const Page& other);

	/**
	 * Get the bounding box of the page, irrespective of its content.
	 */
	inline const util::box<DocumentPrecision,2>& getPageBoundingBox() const { return _pageBoundingBox; }

	/**
	 * Get the size of the page, irrespective of its content.
	 */
	inline const util::point<PagePrecision,2>& getSize() const { return _size; }

	/**
	 * Get the size of the border around the page.
	 */
	inline double getBorderSize() const { return _borderSize; }

	/**
	 * Create a new stroke starting behind the existing points.
	 */
	void createNewStroke(
			const util::point<DocumentPrecision,2>& start,
			double                              pressure,
			unsigned long                       timestamp);

	/**
	 * Create a new stroke starting with stroke point 'begin' (and without an 
	 * end, yet).
	 */
	void createNewStroke(unsigned long begin);

	/**
	 * Add a complete stroke to this page.
	 */
	void addStroke(const Stroke& stroke) { add(stroke); }

	/**
	 * Add a stroke point to the current stroke. This appends the stroke point 
	 * to the global list of stroke points.
	 */
	inline void addStrokePoint(
			const util::point<DocumentPrecision,2>& position,
			double                                pressure,
			unsigned long                         timestamp) {

		// position is in document units -- transform it into page units and store 
		// it in global stroke points list
		util::point<PagePrecision,2> p = position - getShift();

		_strokePoints.add(StrokePoint(p, pressure, timestamp));
		currentStroke().setEnd(_strokePoints.size(), _strokePoints);

		fitBoundingBox(position);
	}

	/**
	 * Get a stroke by its index.
	 */
	inline Stroke& getStroke(unsigned int i) { return get<Stroke>(i); }
	inline const Stroke& getStroke(unsigned int i) const { return get<Stroke>(i); }

	/**
	 * Get the number of strokes.
	 */
	inline unsigned int numStrokes() const {

		return size<Stroke>();
	}

	/**
	 * Get the current stroke of this page.
	 */
	Stroke& currentStroke() { return get<Stroke>().back(); }
	const Stroke& currentStroke() const { return get<Stroke>().back(); }

	/**
	 * Virtually erase points within the given postion and radius by splitting 
	 * the involved strokes.
	 */
	util::box<DocumentPrecision,2> erase(
			const util::point<DocumentPrecision,2>& position,
			DocumentPrecision                     radius);

	/**
	 * Virtually erase strokes that intersect with the given line by setting 
	 * their end to their start.
	 */
	util::box<DocumentPrecision,2> erase(
			const util::point<DocumentPrecision,2>& begin,
			const util::point<DocumentPrecision,2>& end);

	/**
	 * Remove all the strokes from this page for which the given unary predicate 
	 * evaluates to true.
	 *
	 * @return The removed strokes.
	 */
	template <typename Predicate>
	std::vector<Stroke> removeStrokes(const Predicate& pred) {

		std::vector<Stroke>::iterator newEnd = std::partition(get<Stroke>().begin(), get<Stroke>().end(), pred);

		std::vector<Stroke> removed(newEnd, get<Stroke>().end());
		get<Stroke>().resize(newEnd - get<Stroke>().begin());

		recomputeBoundingBox();

		return removed;
	}

	/**
	 * Recompute the bounding box of this page to fit its content.
	 */
	void recomputeBoundingBox();

private:

	struct UpdateBoundingBox {

		UpdateBoundingBox(Page& page_) : page(page_) {}

		template <typename T>
		void operator()(T& t) {

			page.fitBoundingBox(t.getBoundingBox());
		}

		Page& page;
	};

	/**
	 * Erase points from a stroke by splitting. Reports the changed area.
	 */
	util::box<PagePrecision,2> erase(
			Stroke* stroke,
			const util::point<PagePrecision,2>& position,
			PagePrecision radius);

	/**
	 * Erase the given stroke if it intersects the line.
	 */
	util::box<PagePrecision,2> erase(
			Stroke& stroke,
			const util::point<PagePrecision,2>& lineBegin,
			const util::point<PagePrecision,2>& lineEnd);

	bool intersectsErasorCircle(
			const util::point<PagePrecision,2> lineStart,
			const util::point<PagePrecision,2> lineEnd,
			const util::point<PagePrecision,2> center,
			PagePrecision radius2);

	/**
	 * Test, whether the lines p + t*r and q + u*s, with t and u in [0,1], 
	 * intersect.
	 */
	bool intersectLines(
			const util::point<PagePrecision,2>& p,
			const util::point<PagePrecision,2>& r,
			const util::point<PagePrecision,2>& q,
			const util::point<PagePrecision,2>& s);

	inline util::box<DocumentPrecision,2> toDocumentCoordinates(const util::box<PagePrecision,2>& r) {

		util::box<DocumentPrecision,2> result = r;
		return result + getShift();
	}

	inline util::point<DocumentPrecision,2> toDocumentCoordinates(const util::point<PagePrecision,2>& p) {

		return getShift() + p;
	}

	inline util::box<PagePrecision,2> toPageCoordinates(const util::box<DocumentPrecision,2>& r) {

		return r - getShift();
	}

	inline util::point<PagePrecision,2> toPageCoordinates(const util::point<DocumentPrecision,2>& p) {

		return p - getShift();
	}

	// the size of this page on the document
	util::point<PagePrecision,2>   _size;

	// the size of the border around the page (for instance for shadow effects)
	double _borderSize;

	// the bounding box of the page (not its content) in document units
	util::box<DocumentPrecision,2> _pageBoundingBox;

	// the global list of stroke points
	StrokePoints& _strokePoints;
};

#endif // YANTA_PAGE_H__


#ifndef YANTA_SKIA_DOCUMENT_VISITOR_H__
#define YANTA_SKIA_DOCUMENT_VISITOR_H__

#include <SkCanvas.h>

#include <document/Document.h>
#include <document/DocumentTreeRoiVisitor.h>

class SkiaDocumentVisitor : public DocumentTreeRoiVisitor {

public:

	SkiaDocumentVisitor();

	/**
	 * Set the canvas to draw to.
	 */
	void setCanvas(SkCanvas& canvas) { _canvas = &canvas; }

	/**
	 * Set the document that shall be draw by subsequent draw() calls.
	 */
	void setDocument(std::shared_ptr<Document> document) { _document = document; }

	/**
	 * Get the document to visit.
	 */
	Document& getDocument() { return *_document; }

	/**
	 * Check whether a document was set for this visitor already.
	 */
	bool hasDocument() { return static_cast<bool>(_document); }

	/**
	 * Set the transformation to map from document units to pixel units.
	 */
	void setDeviceTransformation(
			const util::point<double,2>& pixelsPerDeviceUnit,
			const util::point<int,2>&    pixelOffset) {

		_pixelsPerDeviceUnit = pixelsPerDeviceUnit;
		_pixelOffset         = pixelOffset;
	}

	/**
	 * Prepare the processing of the whole document.
	 */
	void prepare(const util::box<DocumentPrecision,2>& roi);

	/**
	 * Finish processing of the whole document.
	 */
	void finish();

	/**
	 * Visitor callback to enter a document element. Applies the transformation 
	 * that is stored in this element.
	 */
	void enter(DocumentElement& element);

	/**
	 * Visitor callback to leave a document element. Restores the transformation 
	 * that was changed for this element.
	 */
	void leave(DocumentElement& element);

protected:

	/**
	 * Get the skia canvas to draw to.
	 */
	SkCanvas& getCanvas() { return *_canvas; }

private:

	// the skia canvas to draw to
	SkCanvas* _canvas;

	// the document to draw
	std::shared_ptr<Document> _document;

	// the device transformation (document to skia canvas)
	util::point<double,2> _pixelsPerDeviceUnit;
	util::point<int,2>    _pixelOffset;
};

#endif // YANTA_SKIA_DOCUMENT_VISITOR_H__


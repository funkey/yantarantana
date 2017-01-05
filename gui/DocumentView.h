#ifndef YANTARANTANA_GUI_DOCUMENT_VIEW_H__
#define YANTARANTANA_GUI_DOCUMENT_VIEW_H__

#include <scopegraph/Agent.h>
#include <document/Document.h>
#include <gui/TexturePyramid.h>
#include <gui/SkiaDocumentPainter.h>
#include <sg_gui/GuiSignals.h>

class DocumentView : public sg::Agent<
		DocumentView,
		sg::Accepts<
				sg_gui::Draw
		>
>{

public:

	DocumentView();

	void setDocument(std::shared_ptr<Document> document) {

		_document = document;
		_documentPainter->setDocument(document);
		_documentCleanUpPainter->setDocument(document);
	}

	void onSignal(sg_gui::Draw& signal);

private:

	std::shared_ptr<Document> _document;
	TexturePyramid            _texture;

	std::shared_ptr<SkiaDocumentPainter> _documentPainter;
	std::shared_ptr<SkiaDocumentPainter> _documentCleanUpPainter;

	util::box<float,2> _currentRoi;
};

#endif // YANTARANTANA_GUI_DOCUMENT_VIEW_H__


#include "ContentPreview.h"

void ContentPreview::createModels()
{
	hexModel = new HexDumpModel(this->myPeHndl, true);
	textModel = new HexDumpModel(this->myPeHndl, false);
	
	connect(this, SIGNAL(signalChangeHexViewSettings(HexViewSettings &)), &hexView, SLOT(changeSettings(HexViewSettings &)) );
	connect(this, SIGNAL(signalChangeHexViewSettings(HexViewSettings &)), &textView, SLOT(changeSettings(HexViewSettings &)) );
}

void ContentPreview::deleteModels()
{
	delete hexModel;
	delete textModel;
}

ContentPreview::ContentPreview(PeHandler *peHndl, QWidget *parent)
	: QSplitter(Qt::Horizontal, parent), PeViewItem(peHndl),
	hexView(this), textView(this),
	textModel(NULL), hexModel(NULL)
{
	if (!this->myPeHndl) return;
	if (this->myPeHndl->getPe() == NULL) return;

	createModels();
	addWidget(&this->hexView);
	addWidget(&this->textView);
	
	setAutoFillBackground(true);
	applyViewsSettings();

	/* set Models */
	this->hexView.setModel(hexModel);
	this->textView.setModel(textModel);

	this->textModel->setHexView(false);
	this->hexView.setVHdrVisible(true);
	this->textView.setVHdrVisible(false);

	hexScroll = new QScrollBar(Qt::Vertical, &hexView);
	hexView.setVerticalScrollBar(hexScroll);
	hexView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	hexScroll->setVisible(false);

	textScroll = new QScrollBar(Qt::Vertical, &textView);
	textView.setVerticalScrollBar(textScroll);
	textView.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	connect(textScroll, SIGNAL(sliderMoved(int)), this, SLOT(onSliderMoved(int)) );
	connectViewsSignals();
}

ContentPreview::~ContentPreview()
{
	deleteModels();
}

void ContentPreview::onSliderMoved(int val)
{
	if (hexScroll == NULL || textScroll == NULL) return;
	hexScroll->setSliderPosition(val);
}

void ContentPreview::applyViewsSettings()
{
	QString textStyle = QString() + 
	"HexTableView"
	"{"
		"background-color: " + HEXDMP_HBG + ";"
		"alternate-background-color: " + HEXDMP_ALTBG +";"
		"selection-background-color: " + HEXDMP_BG + ";"
		"selection-color: "+ HEXDMP_TXT +";"
		"color: " + HEXDMP_HTXT + ";"
	"}";
	this->textView.setStyleSheet(textStyle);

	QString hexStyle = QString() + 
	"HexTableView"
	"{"
		"background-color: " + HEXDMP_BG + ";"
		"alternate-background-color: " + HEXDMP_ALTBG +";"
		"selection-background-color: " + HEXDMP_HBG + ";"
		"selection-color: "+ HEXDMP_HTXT +";"
		"color: " + HEXDMP_TXT + ";"
	"}";
	this->hexView.setStyleSheet(hexStyle);
	
	this->hexView.setVHdrVisible(true);
	this->textView.setVHdrVisible(false);
}

void ContentPreview::connectViewsSignals()
{
	connect(this->myPeHndl, SIGNAL(pageOffsetModified(offset_t, bufsize_t)), textModel, SLOT(setShownContent(offset_t, bufsize_t)) );
	connect(this->myPeHndl, SIGNAL(pageOffsetModified(offset_t, bufsize_t)), hexModel, SLOT(setShownContent(offset_t, bufsize_t)) );
}

void ContentPreview::onGoToRVA()
{
	if (!this->m_PE) return;
	
	bool isValid = true;
	offset_t number = 0;
	
	try {
		number = m_PE->rawToRva(this->myPeHndl->displayedOffset);
	} catch (CustomException e) {
		number = 0;
	}

	QString text = QInputDialog::getText(this, 
	    tr("Go to address"),
	    tr("Pointer To RVA (hex):"), 
	    QLineEdit::Normal, 
	    QString::number(number, 16).toUpper(), 
	    &isValid
	);

	if (!isValid || text.isEmpty()) return;
	offset_t raw = 0;

	number = text.toUpper().toLongLong(&isValid, 16);
	if (!isValid) {
		QMessageBox::warning(0,"Warning!", "Wrong number format supplied!");
		return;
	}

	try {
		number = m_PE->VaToRva(number);
		raw = m_PE->rvaToRaw(number);
	} catch (CustomException e) {
		isValid = false;
		QMessageBox::warning(0, "Warning!","RVA:"+ QString::number(number, 16) + " is invalid:\n" + e.what());
	}
	if (!isValid) return;

	this->myPeHndl->setDisplayed(false, raw);
}

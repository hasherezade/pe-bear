#include "OffsetHeader.h"

OffsetHeader::OffsetHeader(QWidget *parent)
	: QHeaderView(Qt::Vertical, parent),
	hexModel(NULL), selectedOffset(INVALID_ADDR), selectedType(Executable::RAW)
{
#if QT_VERSION >= 0x050000
	setSectionsClickable(true);
	setSectionsMovable(false);
#else
	setClickable(true);
	setMovable(false);
#endif
	setAutoFillBackground(true);
	this->setContentsMargins(QMargins(0, 0, 0, 0));
	setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	this->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuEvent(QPoint)) );
	
	this->followAction = new QAction("Follow the offset", this);
	connect(followAction, SIGNAL(triggered()), this, SLOT(followOffset()));

	this->copyAction = new QAction("Copy the offset", this);
	connect(copyAction, SIGNAL(triggered()), this, SLOT(copyOffset()));

	this->defaultMenu.addAction(followAction);
	this->defaultMenu.addAction(copyAction);
}

void OffsetHeader::customMenuEvent(QPoint p)
{
	if (!hexModel) return;
	
	if (selectedOffset == INVALID_ADDR) {
		followAction->setText("Invalid offset");
		followAction->setEnabled(false);
	
	} else {

		QString type;
		switch (selectedType) {
			case Executable::RAW :
				type = "Raw"; break;
			case Executable::RVA :
				type = "RVA"; break;
			case Executable::VA :
				type = "VA"; break;
		}
		followAction->setEnabled(true);
		followAction->setText("Follow " + type + ": " + QString::number(selectedOffset, 16).toUpper());
	}
	QPoint p2 = this->mapToGlobal(p); 
	this->defaultMenu.exec(p2);
}

void OffsetHeader::followOffset()
{
	if (hexModel == NULL || selectedOffset == INVALID_ADDR) return;
	bool isVirtual = (selectedType != Executable::RAW);
	uint64_t offset = selectedOffset;

	if (selectedType == Executable::VA) {
		offset = hexModel->m_PE->VaToRva(selectedOffset);
	}
	hexModel->myPeHndl->setDisplayed(isVirtual, offset);
}

void OffsetHeader::copyOffset()
{
	if (hexModel == NULL || selectedOffset == INVALID_ADDR) return;
	bool isVirtual = (selectedType != Executable::RAW);

	QMimeData *mimeData = new QMimeData;
	mimeData->setText(QString::number(selectedOffset, 16));
	QApplication::clipboard()->setMimeData(mimeData);
}

offset_t OffsetHeader::getSelectedOffset()
{
	offset_t offset = selectedOffset;
	if (selectedType == Executable::VA) {
		offset = hexModel->m_PE->VaToRva(selectedOffset);
	}
	return offset;
}

void OffsetHeader::mousePressEvent(QMouseEvent *event)
{
	QPoint p = event->pos();
	int indx = logicalIndexAt(p) ;

	if (this->hexModel) {
		QVariant data = this->hexModel->headerData(indx, Qt::Vertical, Qt::DisplayRole);
		bool isOk;
		offset_t offset = data.toString().toLongLong(&isOk, 16);
		if (isOk) {
			selectedType = this->hexModel->getAddrType();
			selectedOffset = offset;
		} else {
			selectedType = Executable::NOT_ADDR;
			selectedOffset = INVALID_ADDR;
		}
	}
	QHeaderView::mousePressEvent(event);
}


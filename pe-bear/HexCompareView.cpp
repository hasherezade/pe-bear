#include "HexCompareView.h"

#define MIN_FIELD_HEIGHT 18
#define MIN_FIELD_WIDTH 10

MenuHeader::MenuHeader(QWidget *parent)
	: QHeaderView(Qt::Vertical, parent), tableModel(NULL)
{
#if QT_VERSION >= 0x050000
	setSectionsClickable(true);
	setSectionsMovable(false);
#else
	setClickable(true);
	setMovable(false);
#endif
	setAutoFillBackground(true);

	this->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuEvent(QPoint)) );

	this->copyAction = new QAction(tr("Copy the offset"), this);
	connect(copyAction, SIGNAL(triggered()), this, SLOT(copyOffset()));

	this->defaultMenu.addAction(copyAction);
}

void MenuHeader::customMenuEvent(QPoint p)
{
	if (this->selectedOffset == INVALID_ADDR) {
		return;
	}
	const QPoint p2 = this->mapToGlobal(p); 
	this->defaultMenu.exec(p2);
}

void MenuHeader::mousePressEvent(QMouseEvent *event)
{
	QPoint p = event->pos();
	int indx = logicalIndexAt(p) ;

	if (this->tableModel) {
		
		QVariant data = this->tableModel->headerData(indx, Qt::Vertical, Qt::DisplayRole);
		bool isOk;
		offset_t offset = data.toString().toLongLong(&isOk, 16);
		if (isOk) {
			selectedOffset = offset;
		}
		copyAction->setText(tr("Copy the offset: ") + QString::number(selectedOffset, 16).toUpper());
	}
	QHeaderView::mousePressEvent(event);
}

//----

HexCompareView::HexCompareView(QWidget *parent)
{
	vHdr = new MenuHeader(parent);
	this->setVerticalHeader(vHdr);
	
	init();
	initHeader();
}

void HexCompareView::init()
{
	setShowGrid(false);
	setDragEnabled(false);
	setAutoFillBackground(true);
	setAlternatingRowColors(true);

	this->resizeColumnsToContents();
	this->resizeRowsToContents();

	this->setCursor(Qt::PointingHandCursor);
	setSelectionBehavior(QTreeWidget::SelectItems);
	setSelectionMode(QTreeWidget::ExtendedSelection);
	setDragDropMode(QAbstractItemView::NoDragDrop);

	this->setContentsMargins(0, 0, 0, 0);
	this->setContextMenuPolicy(Qt::CustomContextMenu);
}

void HexCompareView::initHeader()
{
	horizontalHeader()->setContentsMargins(QMargins(0, 0, 0, 0));
	verticalHeader()->setContentsMargins(QMargins(0, 0, 0, 0));
	this->horizontalHeader()->setMinimumSectionSize(MIN_FIELD_WIDTH);
	this->verticalHeader()->setMinimumSectionSize(MIN_FIELD_HEIGHT);

#if QT_VERSION >= 0x050000
	this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	this->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	this->verticalHeader()->setSectionsClickable(true);
#else
	this->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	this->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	this->verticalHeader()->setClickable(true);
#endif
}

#include "HexView.h"

#include <QClipboard>

#define HEX_COL_NUM 16
#define COL_NUM (HEX_COL_NUM)
#define VHDR_UNITS 6

#define COL_WIDTH 10
#define MIN_FIELD_HEIGHT 18
#define MIN_FIELD_WIDTH 10

#include <iostream>

//----
QModelIndex getNextIndex(QAbstractItemModel &model, const QModelIndex &index)
{
	const int rowcount = model.rowCount();
	const int colcount = model.columnCount();
	if (rowcount == 0 || colcount == 0) {
		return QModelIndex();
	}
	
	int currCol = index.column();
	int currRow = index.row();

	if (currCol == (colcount - 1)) {
		currRow++;
		currCol = 0;
	} else {
		currCol++;
	}
	return model.index(currRow, currCol);
}

//----

HexItemDelegate::HexItemDelegate(QObject* parent) :
	QStyledItemDelegate(parent)
{
	validator.setRegExp(QRegExp("[0-9A-Fa-f]{2,}"));
}

void HexItemDelegate::selectNextParentItem(const QModelIndex &index) const
{
	QTableView *parentView = qobject_cast<QTableView*>(this->parent());
	if (!parentView) return;
	
	parentView->setCurrentIndex(index);
	parentView->edit(index);
}


QWidget* HexItemDelegate::createEditor(QWidget *parent,
	const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);

	QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
	if (!lineEdit) {
		return editor;
	}
	QPalette *palette = new QPalette();
	palette->setColor(QPalette::Text, Qt::red);
	palette->setColor(QPalette::Window, Qt::yellow);
	palette->setColor(QPalette::Base, Qt::white);
	lineEdit->setPalette(*palette);
	lineEdit->setAutoFillBackground(true);
	lineEdit->setFrame(false);

	QTableView *parentView = qobject_cast<QTableView*>(this->parent());
	HexDumpModel *hexModel = (!parentView) ? NULL : qobject_cast<HexDumpModel*>(parentView->model());
	if (hexModel) {
		QFont littleFont(hexModel->getSettings()->myFont);
		littleFont.setPointSize(littleFont.pointSize() + 2);
		littleFont.setBold(true);
		lineEdit->setFont(littleFont);
	
		if (hexModel->isHexView()) {
			lineEdit->setValidator(&validator);
			lineEdit->setMaxLength(2);
		} else {
			lineEdit->setMaxLength(1);
		}
	}
	return editor;
}

void HexItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const
{
	QStyledItemDelegate::setModelData(editor, model, index);
	emit dataSet(index.column(), index.row());
}

//--------------------------------------------------------------------

HexTableView::HexTableView(QWidget *parent)
	: ExtTableView(parent), hexModel(NULL), hexColWidth(COL_WIDTH)
{
	this->vHdr = new OffsetHeader(this);
	hHdr = new QHeaderView(Qt::Horizontal, this);	
	
	this->setVerticalHeader(vHdr);
	this->setHorizontalHeader(hHdr);
	vHdr->setVisible(true);
	hHdr->setVisible(true);
	//hHdr->setFrameShape(QFrame::Shape::Panel);
#if QT_VERSION >= 0x050000
	vHdr->setSectionsClickable(true);
#else
	vHdr->setClickable(true);
#endif
	init();
	initHeader();
	initHeaderMenu();
	initMenu();

	adjustMinWidth();
	enableMenu(true);

	HexItemDelegate *delegate = new HexItemDelegate(this);
	setItemDelegate(delegate);
	connect (delegate, SIGNAL(dataSet(int, int)), this, SLOT(onDataSet(int, int)) );
}

void HexTableView::init()
{
	setShowGrid(false);
	setDragEnabled(false);
	setAutoFillBackground(true);

	this->resizeColumnsToContents();
	this->resizeRowsToContents();

	this->setCursor(Qt::PointingHandCursor);
	setSelectionBehavior(QTreeWidget::SelectItems);
	setSelectionMode(QTreeWidget::ExtendedSelection);
	setDragDropMode(QAbstractItemView::NoDragDrop);

	this->setContentsMargins(0, 0, 0, 0);
	this->setContextMenuPolicy(Qt::CustomContextMenu);
}

void HexTableView::initHeaderMenu()
{
	if (!vHdr) return;
	QMenu &hdrMenu = vHdr->defaultMenu;
	QMenu *naviSubMenu = hdrMenu.addMenu("Navigation");

	QString pageSize = QString::number(PREVIEW_SIZE, 16);

	QAction *pgUp = new QAction("$-" + pageSize, naviSubMenu);
	pgUp->setShortcut(QKeySequence(Qt::Key_PageUp));
	connect(pgUp, SIGNAL(triggered()), this, SLOT(setPageUp()));
	naviSubMenu->addAction(pgUp);

	QAction *pgDn = new QAction("$+" + pageSize, naviSubMenu);
	pgDn->setShortcut(QKeySequence(Qt::Key_PageDown));
	connect(pgDn, SIGNAL(triggered()), this, SLOT(setPageDown()));
	naviSubMenu->addAction(pgDn);

	back = new QAction("Back to offset", &hdrMenu);
	back->setShortcut(Qt::Key_B);
	connect(back, SIGNAL(triggered()), this, SLOT(undoOffset()));
	naviSubMenu->addAction(back);
}

void HexTableView::initMenu()
{
	QMenu* menu = &defaultMenu;
	//QMenu* editSubmenu = menu.addMenu("&Selection");

	QAction *copySelAction = new QAction("Copy", menu);
	copySelAction->setShortcut(Qt::CTRL + Qt::Key_C);

	menu->addAction(copySelAction);
	connect(copySelAction, SIGNAL(triggered()), this, SLOT(copySelected()));

	QAction *pasteSelAction = new QAction("Paste to selected", menu);
	pasteSelAction->setShortcut(Qt::CTRL + Qt::Key_V);
	menu->addAction(pasteSelAction);
	connect(pasteSelAction, SIGNAL(triggered()), this, SLOT(pasteToSelected()));

	QMenu* fillSubmenu = menu->addMenu("Fill selected");
	
	QAction *clearSelAction = new QAction("Clear", fillSubmenu);
	clearSelAction->setShortcut(Qt::Key_Delete);
	fillSubmenu->addAction(clearSelAction);
	connect(clearSelAction, SIGNAL(triggered()), this, SLOT(clearSelected()));

	QAction *fillSelAction = new QAction("NOP", fillSubmenu);
	fillSubmenu->addAction(fillSelAction);
	connect(fillSelAction, SIGNAL(triggered()), this, SLOT(fillSelected()));

	undo = new QAction("Undo", menu);
	undo->setShortcut(Qt::CTRL + Qt::Key_Z);
	connect(undo, SIGNAL(triggered()), this, SLOT(undoLastModification()));
}

void HexTableView::initHeader()
{
	horizontalHeader()->setContentsMargins(QMargins(0, 0, 0, 0));
	verticalHeader()->setContentsMargins(QMargins(0, 0, 0, 0));
	this->verticalHeader()->setMinimumWidth(40);
	this->verticalHeader()->setAlternatingRowColors(true);
	
	this->horizontalHeader()->setMinimumSectionSize(MIN_FIELD_WIDTH);
	this->verticalHeader()->setMinimumSectionSize(MIN_FIELD_HEIGHT);
#if QT_VERSION >= 0x050000
	this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	this->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
	this->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	this->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#endif
}


void HexTableView::adjustMinWidth()
{
	int width = (COL_NUM) * this->hexColWidth;
	if (this->hexModel) {
		this->hexColWidth = this->hexModel->isHexView() ? (2 * COL_WIDTH) : COL_WIDTH;
		width = (COL_NUM) * this->hexColWidth;
		//if (this->hexModel->isHexView()) width += (VHDR_UNITS * COL_WIDTH);
	}
	if (this->isVHdrVisible) width += (VHDR_UNITS * COL_WIDTH);
	this->setMinimumWidth(width);
}

void HexTableView::onDataSet(int col, int row)
{
	if (!this->model()) return; // invalid
	
	QModelIndex indx = model()->index(row, col);
	QModelIndex nextIndx = getNextIndex(*this->model(), indx);
	this->setCurrentIndex(nextIndx);
	this->edit(nextIndx);
}

void HexTableView::copySelected()
{
	if (!this->hexModel) return;
	QItemSelectionModel *model = this->selectionModel();
	if (!model) return;

	QModelIndexList list = model->selectedIndexes();
	const int size = list.size();
	if (size == 0) return;

	std::sort(list.begin(), list.end());

	QByteArray bytes;
	for (int i = 0; i < size; i++) {
		QModelIndex index = list.at(i);
		QVariant c = hexModel->getRawContentAt(index);
		if (c.canConvert(QVariant::Char)){ 
#if QT_VERSION >= 0x050000
			BYTE b = c.toChar().toLatin1();
#else
			BYTE b = c.toChar().toAscii();
#endif
			bytes.append(b);
		}
	}
	QString separator = this->hexModel->isHexView() ? " " : "";

	QMimeData *mimeData = new QMimeData;
	//mimeData->setText(getSelectedText(separator, separator));
	QString text = getSelectedText(separator, separator);
#if QT_VERSION >= 0x050000
	mimeData->setData("text/plain", text.toLatin1());
#else
	mimeData->setData("text/plain", text.toAscii());
#endif
	mimeData->setData("application/octet-stream", bytes);
	QApplication::clipboard()->setMimeData(mimeData);
}

bool HexTableView::isIndexListContinuous(QModelIndexList &list)
{
	const int size = list.size();
	if (size == 0) return true;
	std::sort(list.begin(), list.end());

	bool isContinuous = true;
	offset_t prevIndx = INVALID_ADDR;

	for (int i = 0; i < list.size(); i++) {
		QModelIndex index = list.at(i);
		offset_t cIndx = hexModel->contentOffsetAt(index);
		if (prevIndx != INVALID_ADDR && cIndx != prevIndx + 1) {
			return false;
		}
		prevIndx = cIndx;
	}
	return true;
}

void HexTableView::pasteToSelected()
{
	QItemSelectionModel *model = this->selectionModel();
	if (!model) return;

	QModelIndexList list = model->selectedIndexes();
	const int bufSize = list.size();
	if (bufSize == 0) return;

	if (!isIndexListContinuous(list)) {
		QMessageBox::warning(0, "Warning!", "Select continuous area!");
		return;
	}
	offset_t first = hexModel->contentOffsetAt(list.at(0));
	if (first == INVALID_ADDR) return;

	BYTE *buf = new BYTE[bufSize];
	bool isHex = this->hexModel->isHexView();
	size_t clipSize = ClipboardUtil::getFromClipboard(isHex, buf, bufSize);
	
	bool success = hexModel->myPeHndl->substBlock(first, clipSize, buf);
	delete []buf; buf = NULL;
	
	if (success == false) {
		QMessageBox::warning(0, "Error!", "Modification in this area in  unacceptable!\n(Causes format corruption)");
		return;
	}
}

void HexTableView::fillSelected()
{
	QItemSelectionModel *model = this->selectionModel();
	if (!model) return;

	QModelIndexList list = model->selectedIndexes();
	const int size = list.size();
	if (size == 0) return;

	if (!isIndexListContinuous(list)) {
		QMessageBox::warning(0,"Warning!", "Select continuous area!");
		return;
	}

	offset_t first = hexModel->contentOffsetAt(list.at(0));
	if (first == INVALID_ADDR) return;

	if (hexModel->myPeHndl->fillBlock(first, size, 0x90) == false) {
		QMessageBox::warning(0, "Error!", "Modification in this area in  unacceptable!\n(Causes format corruption)");
		return;
	}
}


void HexTableView::clearSelected()
{
	QItemSelectionModel *model = this->selectionModel();
	if (!model) return;

	QModelIndexList list = model->selectedIndexes();
	const int size = list.size();
	if (size == 0) return;

	if (!isIndexListContinuous(list)) {
		QMessageBox::warning(0,"Warning!", "Select continuous area!");
		return;
	}
	offset_t first = hexModel->contentOffsetAt(list.at(0));
	if (first == INVALID_ADDR) return;

	if (hexModel->myPeHndl->fillBlock(first, size, 0) == false) {
		QMessageBox::warning(0, "Error!", "Modification in this area in  unacceptable!\n(Causes format corruption)");
		return;
	}
}

void HexTableView::setPageUp() 
{ 
	if (!hexModel) return; 
	hexModel->myPeHndl->advanceOffset(-(PREVIEW_SIZE));
}

void HexTableView::setPageDown()
{ 
	if (!hexModel) return; ;
	hexModel->myPeHndl->advanceOffset(PREVIEW_SIZE);
}

void HexTableView::undoOffset()
{
	if (!hexModel) return; 
	hexModel->myPeHndl->undoDisplayOffset();
}

void HexTableView::undoLastModification()
{
	if (!hexModel) return; 
	hexModel->myPeHndl->unModify();
}

void HexTableView::updateUndoAction()
{
	if (!hexModel) return; 
	if (hexModel->myPeHndl->prevOffsets.size() > 0) {
		this->back->setEnabled(true);
		this->back->setText("Back to: 0x" + QString::number(hexModel->myPeHndl->prevOffsets.top(), 16).toUpper());
	} else {
		this->back->setEnabled(false);
	}
}

void HexTableView::keyPressEvent(QKeyEvent *event) 
{
	bool isHex = (this->hexModel) ? this->hexModel->showHex : false;

	if (event->matches(QKeySequence::Undo)) {
		undoLastModification();
		return;
	} else if (event->matches(QKeySequence::Copy)) {
		copySelected();
		return;
	} else if (event->matches(QKeySequence::Paste)) {
		pasteToSelected();
		return;
	} else if (event->matches(QKeySequence::Delete)) {
		clearSelected();
		return;
	}
	ExtTableView::keyPressEvent(event);
}

void HexTableView::setVHdrVisible(bool isVisible)
{
	this->vHdr->setVisible(isVisible);
	isVHdrVisible = isVisible;
	adjustMinWidth();
}

void HexTableView::setModel(HexDumpModel *model)
{
	QTableView::setModel(model);
	this->vHdr->setHexModel(model);
	if (this->hexModel){
		disconnect(this->hexModel, SIGNAL(modelUpdated()), this, SLOT(onModelUpdated()));
		disconnect(this->hexModel, SIGNAL(scrollReset()), this, SLOT(onScrollReset()));

		disconnect(this->hexModel->myPeHndl, SIGNAL(pageOffsetModified(offset_t, bufsize_t)), this, SLOT(updateUndoAction()) );
		disconnect(this->hexModel->myPeHndl, SIGNAL(hovered()), this, SLOT(onResetRequested()) );
	}

	this->hexModel = model;
	connect(this->hexModel, SIGNAL(modelUpdated()), this, SLOT(onModelUpdated()));
	connect(this->hexModel, SIGNAL(scrollReset()), this, SLOT(onScrollReset()));

	connect(this->hexModel->myPeHndl, SIGNAL(pageOffsetModified(offset_t, bufsize_t)), this, SLOT(updateUndoAction()) );
	connect(this->hexModel->myPeHndl, SIGNAL(hovered()), this, SLOT(onResetRequested()) );
	adjustMinWidth();
}

void HexTableView::changeSettings(HexViewSettings &_settings)
{
	if (!hexModel) return;
	hexModel->changeSettings(_settings);
	initHeader();
	reset();
}

void HexTableView::onScrollReset()
{
	QScrollBar *scroll = this->verticalScrollBar();
	if (scroll) {
		scroll->setSliderPosition(0);
	}
}

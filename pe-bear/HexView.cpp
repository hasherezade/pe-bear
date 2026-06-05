#include "HexView.h"

#include <QClipboard>

#define HEX_COL_NUM 16
#define COL_NUM (HEX_COL_NUM)
#define VHDR_UNITS 6

#define COL_WIDTH 10
#define MIN_FIELD_HEIGHT 18
#define MIN_FIELD_WIDTH 10

#include <iostream>
#include "TempBuffer.h"

// Util:

quint64 littleEndianToInt(const QByteArray& bytes)
{
	quint64 value = 0;

	for (int i = 0; i < bytes.size() && i < 8; ++i) {
		value |= (static_cast<quint64>(
			static_cast<unsigned char>(bytes[i])) << (8 * i));
	}

	return value;
}

bool getBytesContent(const QModelIndexList& list, const HexDumpModel& hexModel, QByteArray& bytes)
{
	const int size = list.size();
	if (!size) {
		return false;
	}
	for (int i = 0; i < size; i++) {
		QModelIndex index = list.at(i);
		QVariant c = hexModel.getRawContentAt(index);
		if (c.canConvert(QVariant::Char)) {
#if QT_VERSION >= 0x050000
			BYTE b = c.toChar().toLatin1();
#else
			BYTE b = c.toChar().toAscii();
#endif
			bytes.append(b);
		}
	}
	return !bytes.isEmpty();
}

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
	QStyledItemDelegate(parent), 
	m_selectionBgColor(QColor(255, 0, 0)), m_selectionTextColor(QColor(255, 255, 255))
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	validator.setRegularExpression(QRegularExpression("[0-9A-Fa-f]{2,}"));
#else
	validator.setRegExp(QRegularExpression("[0-9A-Fa-f]{2,}"));
#endif
}

QWidget* HexItemDelegate::createEditor(QWidget *parent,
	const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);

	// Register the editor with its owning view so the view can later tell its own
	// (live) editors apart from stale/duplicate commit/close deliveries.
	if (HexTableView* view = qobject_cast<HexTableView*>(this->parent())) {
		view->noteEditorOpened(editor);
	}

	QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
	if (!lineEdit) {
		return editor;
	}
	QPalette palette;
	palette.setColor(QPalette::Text, Qt::red);
	palette.setColor(QPalette::Window, Qt::yellow);
	palette.setColor(QPalette::Base, Qt::white);
	lineEdit->setPalette(palette);

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
	const QString before = model->data(index, Qt::EditRole).toString();
	QStyledItemDelegate::setModelData(editor, model, index);
	const QString after = model->data(index, Qt::EditRole).toString();
	if (before != after) {
		emit dataSet(index.column(), index.row());
	}
}

void HexItemDelegate::paint(QPainter* painter,
	const QStyleOptionViewItem& option,
	const QModelIndex& index) const
{
	QStyleOptionViewItem opt(option);
	initStyleOption(&opt, index);

	if (opt.state & QStyle::State_Selected)
	{
		// Draw custom background
		painter->fillRect(opt.rect, m_selectionBgColor);

		// Tell Qt the item is not selected anymore
		// so it won't draw its own selection background
		opt.state &= ~QStyle::State_Selected;

		// Use desired text color
		opt.palette.setColor(QPalette::Text, m_selectionTextColor);
		opt.palette.setColor(QPalette::WindowText, m_selectionTextColor);
	}

	QStyledItemDelegate::paint(painter, opt, index);
}

//--------------------------------------------------------------------

HexTableView::HexTableView(QWidget *parent)
	: ExtTableView(parent), hexModel(nullptr), hexColWidth(COL_WIDTH), m_delegate(nullptr),
	followAddrSubmenu(nullptr), backAction(nullptr), undoAction(nullptr)
{
	for (int i = 0; i < static_cast<int>(Executable::ADDR_TYPE_COUNT); ++i) {
		followAction[i] = nullptr;
	}

	this->vHdr = new OffsetHeader(this);
	this->hHdr = new QHeaderView(Qt::Horizontal, this);	

	this->setVerticalHeader(vHdr);
	this->setHorizontalHeader(hHdr);
	vHdr->setVisible(true);
	hHdr->setVisible(true);

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

	m_delegate = new HexItemDelegate(this);
	setItemDelegate(m_delegate);
	connect (m_delegate, SIGNAL(dataSet(int, int)), this, SLOT(onDataSet(int, int)), Qt::QueuedConnection );
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

void HexTableView::setSelectionColor(const QColor& bgColor, const QColor& textColor)
{
	this->m_delegate->setSelectionColor(bgColor, textColor);
}

void HexTableView::initHeaderMenu()
{
	if (!vHdr) return;
	QMenu &hdrMenu = vHdr->defaultMenu;
	QMenu *naviSubMenu = hdrMenu.addMenu(tr("Navigation"));

	QString pageSize = QString::number(PREVIEW_SIZE, 16);

	QAction *pgUp = new QAction("$-" + pageSize, naviSubMenu);
	pgUp->setShortcut(QKeySequence(Qt::Key_PageUp));
	connect(pgUp, SIGNAL(triggered()), this, SLOT(setPageUp()));
	naviSubMenu->addAction(pgUp);

	QAction *pgDn = new QAction("$+" + pageSize, naviSubMenu);
	pgDn->setShortcut(QKeySequence(Qt::Key_PageDown));
	connect(pgDn, SIGNAL(triggered()), this, SLOT(setPageDown()));
	naviSubMenu->addAction(pgDn);

	backAction = new QAction(tr("Back to offset"), &hdrMenu);
	backAction->setShortcut(Qt::Key_B);
	connect(backAction, SIGNAL(triggered()), this, SLOT(undoOffset()));
	naviSubMenu->addAction(backAction);
}

void HexTableView::initMenu()
{
	QMenu* menu = &defaultMenu;
	QAction *copySelAction = new QAction(tr("Copy"), menu);
	copySelAction->setShortcut(Qt::CTRL | Qt::Key_C);

	menu->addAction(copySelAction);
	connect(copySelAction, SIGNAL(triggered()), this, SLOT(copySelected()));

	QAction *pasteSelAction = new QAction(tr("Paste to selected"), menu);
	pasteSelAction->setShortcut(Qt::CTRL | Qt::Key_V);
	menu->addAction(pasteSelAction);
	connect(pasteSelAction, SIGNAL(triggered()), this, SLOT(pasteToSelected()));

	QMenu* fillSubmenu = menu->addMenu(tr("Fill selected"));
	
	QAction *clearSelAction = new QAction(tr("Clear"), fillSubmenu);
	clearSelAction->setShortcut(Qt::Key_Delete);
	fillSubmenu->addAction(clearSelAction);
	connect(clearSelAction, SIGNAL(triggered()), this, SLOT(clearSelected()));

	QAction *nopSelAction = new QAction(tr("NOP"), fillSubmenu);
	fillSubmenu->addAction(nopSelAction);
	connect(nopSelAction, SIGNAL(triggered()), this, SLOT(fillSelectedNOP()));

	QAction* fillSelAction = new QAction(tr("Custom..."), fillSubmenu);
	fillSubmenu->addAction(fillSelAction);
	connect(fillSelAction, SIGNAL(triggered()), this, SLOT(fillSelectedCustom()));

	undoAction = new QAction(tr("Undo"), menu);
	undoAction->setShortcut(Qt::CTRL | Qt::Key_Z);
	connect(undoAction, SIGNAL(triggered()), this, SLOT(undoLastModification()));

	followAddrSubmenu = menu->addMenu(tr("Follow selected address"));

	followAction[Executable::RAW] = new QAction(tr("Raw"), menu);
	followAddrSubmenu->addAction(followAction[Executable::RAW]);
	connect(followAction[Executable::RAW], SIGNAL(triggered()), this, SLOT(followSelectedRaw()));

	followAction[Executable::RVA] = new QAction(tr("RVA"), menu);
	followAddrSubmenu->addAction(followAction[Executable::RVA]);
	connect(followAction[Executable::RVA], SIGNAL(triggered()), this, SLOT(followSelectedRva()));

	followAction[Executable::VA] = new QAction(tr("VA"), menu);
	followAddrSubmenu->addAction(followAction[Executable::VA]);
	connect(followAction[Executable::VA], SIGNAL(triggered()), this, SLOT(followSelectedVa()));

	connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateFollowAction()));
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

void HexTableView::commitData(QWidget *editor)
{
	// Drop a commit for an editor this view no longer owns (stale/duplicate
	// delivery under fast editing). The base would only warn and return.
	if (editor && m_liveEditors.contains(editor)) {
		ExtTableView::commitData(editor);
	}
}

void HexTableView::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
	// Drop a close for an editor this view no longer owns; otherwise forward it and
	// forget the editor. This suppresses the "editor does not belong to this view"
	// warning flood without changing valid editing behaviour.
	if (editor && m_liveEditors.remove(editor)) {
		ExtTableView::closeEditor(editor, hint);
	}
}

void HexTableView::editorDestroyed(QObject *editor)
{
	// Keep the live-editor set free of dangling pointers.
	m_liveEditors.remove(static_cast<QWidget*>(editor));
	ExtTableView::editorDestroyed(editor);
}

void HexTableView::onDataSet(int col, int row)
{
	QAbstractItemModel* model = this->model();
	if (!model) return; // invalid
	
	QModelIndex indx = model->index(row, col);
	if (!indx.isValid()) {
		return;
	}
	QModelIndex nextIndx = getNextIndex(*model, indx);
	if (!nextIndx.isValid()) {
		return;
	}
	this->setCurrentIndex(nextIndx);
	// Use the protected bool overload instead of the public edit(index) slot: under fast
	// editing the auto-advance can land while a commit is still settling, in which case
	// opening the editor fails harmlessly. The public slot would warn ("editing failed");
	// here we just let the cursor advance without forcing the editor open.
	this->edit(nextIndx, QAbstractItemView::AllEditTriggers, NULL);
}

void HexTableView::updateFollowAction()
{
	const QString defaultDesc = tr("Follow selected address");

	PeHandler* hndl = hexModel->myPeHndl;
	const offset_t addr = getSelectedAddress();

	if (!hndl || addr == INVALID_ADDR) {
		this->followAddrSubmenu->setTitle(defaultDesc);
		followAddrSubmenu->setEnabled(false);
		return;
	}

	bool isMenuEnabled = false;

	for (int t = 0; t < Executable::ADDR_TYPE_COUNT; ++t) {
		const Executable::addr_type aType = static_cast<Executable::addr_type>(t);
		bool _isVisible = false;
		if (hndl->isValidAddr(aType, addr)) {
			isMenuEnabled = true;
			_isVisible = true;
		}
		if (followAction[aType]) {
			followAction[aType]->setVisible(_isVisible);
		}
	}

	followAddrSubmenu->setEnabled(isMenuEnabled);

	if (!isMenuEnabled) {
		this->followAddrSubmenu->setTitle(defaultDesc);
		return;
	}
	followAddrSubmenu->setTitle("Follow: [0x" + QString::number(addr, 16).toUpper() + "] as");
}

offset_t HexTableView::getSelectedAddress()
{
	if (!this->hexModel) return INVALID_ADDR;

	PeHandler* hndl = hexModel->myPeHndl;
	if (!hndl) return INVALID_ADDR;

	QItemSelectionModel* model = this->selectionModel();
	QModelIndexList list = model->selectedIndexes();
	const int size = list.size();
	if (!size || !isIndexListContinuous(list)) {
		return INVALID_ADDR;
	}
	QByteArray bytes;
	if (!getBytesContent(list, *hexModel, bytes)) return INVALID_ADDR;

	const quint64 number = littleEndianToInt(bytes);
	return static_cast<offset_t>(number);
}

void HexTableView::followSelected(const Executable::addr_type& aType)
{
	if (!this->hexModel) return;

	PeHandler* hndl = hexModel->myPeHndl;
	if (!hndl) return;

	const offset_t addr = getSelectedAddress();
	hndl->setDisplayed(aType, addr);
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
		QMessageBox::warning(0, tr("Warning!"), tr("Select continuous area!"));
		return;
	}
	offset_t first = hexModel->contentOffsetAt(list.at(0));
	if (first == INVALID_ADDR) return;

	TempBuffer temp;
	temp.init(bufSize);
	BYTE *buf = temp.getContent();
	if (!buf) return;

	bool isHex = this->hexModel->isHexView();
	size_t clipSize = ClipboardUtil::getFromClipboard(isHex, buf, bufSize);
	
	bool success = hexModel->myPeHndl->substBlock(first, clipSize, buf);
	if (success == false) {
		QMessageBox::warning(0, tr("Error!"), tr("Modification in this area in  unacceptable!")+"\n"+tr("(Causes format corruption)"));
		return;
	}
}

void HexTableView::fillSelectedCustom()
{
	bool ok = false;

	QString text = QInputDialog::getText(
		this,
		tr("Fill Selection"),
		tr("Enter byte value (hex):"),
		QLineEdit::Normal,
		"90",
		&ok
	);

	if (!ok || text.isEmpty())
		return;

	bool convOk = false;
	int value = text.toInt(&convOk, 16);

	if (!convOk || value < 0x00 || value > 0xFF) {
		QMessageBox::warning(
			this,
			tr("Invalid value"),
			tr("Please enter a hexadecimal byte value between 00 and FF.")
		);
		return;
	}

	fillSelected(static_cast<char>(value));
}

void HexTableView::fillSelected(const char val)
{
	QItemSelectionModel *model = this->selectionModel();
	if (!model) return;

	QModelIndexList list = model->selectedIndexes();
	const int size = list.size();
	if (size == 0) return;
	if (!isIndexListContinuous(list)) {
		QMessageBox::warning(0, tr("Warning!"), tr("Select continuous area!"));
		return;
	}

	offset_t first = hexModel->contentOffsetAt(list.at(0));
	if (first == INVALID_ADDR) return;

	if (hexModel->myPeHndl->fillBlock(first, size, val) == false) {
		QMessageBox::warning(0, tr("Error!"), tr("Modification in this area in  unacceptable!")+"\n"+ tr("(Causes format corruption)"));
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
		this->backAction->setEnabled(true);
		this->backAction->setText(tr("Back to: 0x") + QString::number(hexModel->myPeHndl->prevOffsets.top(), 16).toUpper());
	} else {
		this->backAction->setEnabled(false);
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

	// TODO: get from the style sheet:
	if (this->hexModel->isHexView()) {
		this->setSelectionColor(QColor(SELECTION_HBG_COLOR), QColor(SELECTION_HTXT_COLOR));
	}
	else {
		this->setSelectionColor(QColor(SELECTION_TBG_COLOR), QColor(SELECTION_TTXT_COLOR));
	}
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

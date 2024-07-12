#include "DisasmView.h"
#include <QtGlobal>
#include <bearparser/Util.h>
#include <set>
#include "gui/CommentView.h"
#include "TempBuffer.h"

#define VHDR_WIDTH 50
#define COL_WIDTH 18

#define MIN_FIELD_HEIGHT 18
#define MIN_FIELD_WIDTH 18
#define MIN_OFFSET_HDR_WIDTH 18

using namespace pe_bear;
using namespace DisasmView;


void ArgDependentAction::onOffsetChanged(int argNum, offset_t offset)
{
	//printf("ArgDependentAction:: got arg: %d RVA = %llx\n", argNum, offset);
	if (this->myArgNum != argNum) return; // not mine...
	OffsetDependentAction::onOffsetChanged(offset);
}

void ArgDependentAction::onOffsetChanged(int argNum, offset_t offset, Executable::addr_type addrType)
{
	if (this->myArgNum != argNum) return; // not mine...
	OffsetDependentAction::onOffsetChanged(offset, addrType);
}

//-------------------------
DisasmScrollBar::DisasmScrollBar(QWidget *parent) 
	: QScrollBar(parent), myModel(NULL)
{
	this->setAutoFillBackground(true);
	QPalette *palette = new QPalette();
	palette->setColor(QPalette::Text, Qt::red);
	palette->setColor(QPalette::Window, Qt::yellow);
	palette->setColor(QPalette::Base, Qt::white);
	this->setPalette(*palette);

	initMenu();
	enableMenu(true);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuEvent(QPoint)) );
}

void DisasmScrollBar::setModel(DisasmModel* disasmModel)
{
	myModel = disasmModel;
	setSliderPosition(0);
}

void DisasmScrollBar::enableMenu(bool enable)
{
	if (enable) {
		this->setContextMenuPolicy(Qt::CustomContextMenu);
	} else {
		this->setContextMenuPolicy(Qt::DefaultContextMenu);
	}
}

void DisasmScrollBar::initMenu()
{
	QString pageSize = QString::number(PREVIEW_SIZE, 16);
	QAction *upAction = new QAction("$-" + pageSize, &defaultMenu);
	upAction->setShortcut(Qt::Key_PageUp);
	QAction *downAction = new QAction("$+" + pageSize, &defaultMenu);
	downAction->setShortcut(Qt::Key_PageDown);

	defaultMenu.addAction(downAction);
	defaultMenu.addAction(upAction);

	connect(upAction, SIGNAL(triggered()), this, SLOT(pgUp()) );
	connect(downAction, SIGNAL(triggered()), this, SLOT(pgDown()) );
}

void DisasmScrollBar::pgUp()
{
	if (!myModel) return;

	int last = this->myModel->disasmCount() - 1;
	
	bool isOk = false;
	offset_t rva = myModel->getRvaAt(last);
	if (rva == INVALID_ADDR) return;

	if (!myModel->getPeHandler()->setDisplayed(true, rva)) {
		printf("Conversion failed, simple advance...\n");
		myModel->getPeHandler()->advanceOffset(PREVIEW_SIZE);
	}
}

void DisasmScrollBar::pgDown()
{
	if (!myModel) return;
	myModel->getPeHandler()->advanceOffset((-1)*(PREVIEW_SIZE));
}

void DisasmScrollBar::mousePressEvent(QMouseEvent *e)
{
	if (myModel){
		int count = myModel->disasmCount();
		if (value() == maximum()) pgUp();//myModel->getPeHandler()->advanceOffset(1);
		if (value() == 0) myModel->getPeHandler()->advanceOffset(-1);
	}
	
	QScrollBar::mousePressEvent(e);
}

//---

QWidget* DisasmItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
	QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
	if (!lineEdit) {
		return editor;
	}
	QPalette *palette = new QPalette();
	palette->setColor(QPalette::Text, Qt::red);
	palette->setColor(QPalette::Window, Qt::blue);
	palette->setColor(QPalette::Base, Qt::blue);
	lineEdit->setPalette(*palette);
	lineEdit->setAutoFillBackground(true);
	lineEdit->setFrame(false);
	
	QTableView *parentView = qobject_cast<QTableView*>(this->parent());
	DisasmModel *myModel = (!parentView) ? NULL : qobject_cast<DisasmModel*>(parentView->model());
	if (myModel) {
		QFont littleFont(myModel->getSettings()->myFont);
		littleFont.setPointSize(littleFont.pointSize() + 2);
		littleFont.setBold(true);
		lineEdit->setFont(littleFont);
		
		const int x = index.column();
		const int y = index.row();
		if (x == HEX_COL) {
			lineEdit->setValidator(&validator);
			lineEdit->setMaxLength(myModel->getChunkSize(y) * 2);
		}
	}
	return editor;
}
//---------------------------------------------------------

DisasmTreeView::DisasmTreeView(QWidget *parent)
	: ExtTableView(parent),
	myModel(NULL), commentsView(NULL),
	vHdr(this), 
	imgBaseA(NULL), undoAction(NULL)
{
	this->setVerticalScrollBar(&vScrollbar);
	this->setVerticalHeader(&vHdr);

	setDragEnabled(false);
	setShowGrid(false);
	setAutoFillBackground(true);
	setWordWrap(false);

	setCursor(Qt::PointingHandCursor);
	setSelectionBehavior(QTreeWidget::SelectItems);
	setSelectionMode(QTreeWidget::ExtendedSelection);

	setDragDropMode(QAbstractItemView::NoDragDrop);
	setHorizontalScrollMode(QAbstractItemView::ScrollPerItem);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	
	init();
	initHeader();
	initHeaderMenu();
	initMenu();

	enableMenu(true);
	setMouseTracking(true);
	
	DisasmItemDelegate *delegate = new DisasmItemDelegate(this);
	setItemDelegate(delegate);
}

void DisasmTreeView::init()
{
	setContentsMargins(0, 0, 0, 0);
	setIconSize(QSize(MIN_FIELD_WIDTH, MIN_FIELD_HEIGHT));
	/* init palette */
	QPalette p = this->palette();
	p.setColor(QPalette::Highlight, QColor(HEXDMP_HBG));
	p.setColor(QPalette::HighlightedText, QColor(HEXDMP_HTXT));
	
	p.setColor(QPalette::Base, QColor(DISASMDMP_BG)); 
	p.setColor(QPalette::Text, QColor(DISASMDMP_TXT));
	setPalette(p);
}

void DisasmTreeView::onSetComment(offset_t offset, Executable::addr_type aT)
{
	if (!myModel) return;
	if (this->commentsView == NULL) return;
	if (aT == Executable::RAW || aT == Executable::NOT_ADDR) return; // TODO...
	if (aT == Executable::VA) {
		offset = myModel->m_PE->VaToRva(offset);
	}
	this->commentsView->onSetComment(offset);
}

void DisasmTreeView::onSetEpAction(offset_t offset, Executable::addr_type aT)
{
	if (!myModel || !myModel->m_PE) return;
	if (aT == Executable::RAW || aT == Executable::NOT_ADDR) return; // TODO...
	if (aT == Executable::VA) {
		offset = myModel->m_PE->VaToRva(offset);
	}
	PeHandler *hndl = this->myModel->getPeHandler();
	if (!hndl) return;
	hndl->setEP(offset);
}

void DisasmTreeView::onFollowOffset(offset_t offset, Executable::addr_type aT)
{
	if (!myModel) return;

	PeHandler *hndl = this->myModel->getPeHandler();
	if (!hndl || !hndl->getPe()) return;

	try {
		offset_t raw = hndl->getPe()->toRaw(offset, aT, true);
		hndl->setDisplayed(false, raw);

	} catch (CustomException e) {
		this->lastErrorString = tr("Address: ")+ QString::number(offset, 16) + tr(" is invalid:") + "\n" + e.what();
		QMessageBox::warning(0, tr("Warning!"), this->lastErrorString );
	}
}

void DisasmTreeView::copySelected()
{
	if (!this->myModel) return;
	QItemSelectionModel *model = this->selectionModel();
	if (!model) return;

	QModelIndexList list = this->uniqOffsets(model->selectedIndexes());
	const int size = list.size();
	if (size == 0) return;

	//selected area may not be continuous
	QByteArray bytes;
	for (int i = 0; i < size; i++) {
		QModelIndex index = list.at(i);

		QVariant c = myModel->getRawContentAt(index);
		if (c.canConvert(QVariant::ByteArray)) {
			bytes.append(c.toByteArray());
		}
	}
	QMimeData *mimeData = new QMimeData;
	mimeData->setText(getSelectedText("\t", "\n"));
	mimeData->setData("application/octet-stream", bytes);
	QApplication::clipboard()->setMimeData(mimeData);
}

void DisasmTreeView::pasteToSelected()
{
	if (!this->myModel) return;
	QItemSelectionModel *model = this->selectionModel();
	if (!model) return;

	QModelIndexList list = this->uniqOffsets(model->selectedIndexes());
	if (!this->isIndexListContinuous(list)) {
		QMessageBox::warning(0, tr("Warning!"), tr("Select continuous area!"));
		return;
	}
	if (list.size() == 0) return;

	offset_t firstOffset = myModel->contentOffsetAt(list.at(0));
	int bufSize = this->blockSize(list);

	BYTE *cntntPtr = myModel->m_PE->getContent();
	offset_t cntntSize = myModel->m_PE->getRawSize();

	TempBuffer temp;
	temp.init(bufSize);
	BYTE *buf = temp.getContent();
	if (!buf) return;

	size_t clipSize = ClipboardUtil::getFromClipboard(false, buf, bufSize);
	myModel->myPeHndl->substBlock(firstOffset, clipSize, buf);
}


void DisasmTreeView::initMenu()
{
	QMenu *menu = &defaultMenu;;

	QAction *copySelAction = new QAction(tr("Copy"), menu);
	copySelAction->setShortcut(Qt::CTRL + Qt::Key_C);

	menu->addAction(copySelAction);
	connect(copySelAction, SIGNAL(triggered()), this, SLOT(copySelected()));

	QAction *pasteSelAction = new QAction(tr("Paste to selected"), menu);
	pasteSelAction->setShortcut(Qt::CTRL + Qt::Key_V);
	menu->addAction(pasteSelAction);
	connect(pasteSelAction, SIGNAL(triggered()), this, SLOT(pasteToSelected()));

	QMenu *followMenu = new QMenu(tr("Follow"), menu);
	menu->addMenu(followMenu);

	OffsetDependentAction* followRvaAction = new OffsetDependentAction(Executable::RVA, tr("Selection RVA:") + "\t", followMenu);
	followMenu->addAction(followRvaAction);
	connect(this, SIGNAL(currentRvaChanged(offset_t)), followRvaAction, SLOT(onOffsetChanged(offset_t)) );
	connect(followRvaAction, SIGNAL(triggered(offset_t, Executable::addr_type)), this, SLOT(onFollowOffset(offset_t, Executable::addr_type)));

	for ( int argNum = 0; argNum < Disasm::MAX_ARG_NUM; argNum++) {
		ArgDependentAction* followArgsAction = new ArgDependentAction(argNum, Executable::RVA, "Arg " + QString::number(argNum) + " RVA:\t", followMenu);
		followMenu->addAction(followArgsAction);
		connect(this, SIGNAL(argRvaChanged(int, offset_t)), followArgsAction, SLOT(onOffsetChanged(int, offset_t)) );
		connect(followArgsAction, SIGNAL(triggered(offset_t, Executable::addr_type)), this, SLOT(onFollowOffset(offset_t, Executable::addr_type)));
	}
	/*

	QMenu *editMenu = new QMenu("Edit", menu);
	menu->addMenu(editMenu);

	OffsetDependentAction* editAction = new OffsetDependentAction(Executable::RVA, "Selection", editMenu);
	editMenu->addAction(editAction);
	connect(this, SIGNAL(currentRvaChanged(uint64_t)), editAction, SLOT(onOffsetChanged(uint64_t)) );
	connect(editAction, SIGNAL(triggered(uint64_t, Executable::addr_type)), this, SLOT(onHexLineEdit(uint64_t, Executable::addr_type)));
	*/
	menu->addSeparator();
	// tag menu
	QPixmap tagIco(":/icons/star.ico");
	//QMenu* tagSubmenu = menu->addMenu(tagIco, "Tag");
	//---
	OffsetDependentAction* setCommentAction = new OffsetDependentAction(Executable::RVA, "Tag", menu);
	setCommentAction->setIcon(tagIco);
	menu->addAction(setCommentAction);
	connect(this, SIGNAL(currentRvaChanged(offset_t)), setCommentAction, SLOT(onOffsetChanged(offset_t)) );
	connect(setCommentAction, SIGNAL(triggered(offset_t, Executable::addr_type) ), this, SLOT(onSetComment(offset_t, Executable::addr_type)));

	//set EP at offset:
	QPixmap epIco(":/icons/arrow-right.ico");
	OffsetDependentAction* setEpAction = new OffsetDependentAction(Executable::RVA, tr("Set EP ="), menu);
	setEpAction->setIcon(epIco);
	menu->addAction(setEpAction);
	connect(this, SIGNAL(currentRvaChanged(offset_t)), setEpAction, SLOT(onOffsetChanged(offset_t)) );
	connect(setEpAction, SIGNAL(triggered(offset_t, Executable::addr_type) ), this, SLOT(onSetEpAction(offset_t, Executable::addr_type)));
}

void DisasmTreeView::initHeaderMenu()
{
	QMenu &hdrMenu = vHdr.defaultMenu;
	//------------

	QMenu *hdrSettings = new QMenu(tr("Settings"), &hdrMenu);
	hdrMenu.addSeparator();
	hdrMenu.addMenu(hdrSettings);

	imgBaseA = hdrSettings->addAction("RVA -> VA");
	imgBaseA->setCheckable(true);
	imgBaseA->setToolTip(tr("Add ImageBase"));
	imgBaseA->setChecked(false);

	/*Bitmode setting actions */
	QActionGroup *group = new QActionGroup(this);
	group->setExclusive(true);
	QMenu* archMenu = hdrSettings->addMenu(tr("Architectu&re"));

	const int MODES_NUM = 6;
	QAction *actionsI[MODES_NUM];

	actionsI[0] = group->addAction(tr("Automatic"));
	actionsI[0]->setData(0);

	for (int i = 1; i < MODES_NUM; i++) {
		if (i < 4) {
			int bitmode = 16<<(i-1);
			actionsI[i] = group->addAction(QString(tr("Intel")) + ": " + QString::number(bitmode) + tr("-bit"));
		} else {
			int val = i - 2;
			int bitmode = 16<<(val-1);
			actionsI[i] = group->addAction(QString(tr("ARM")) + ": " + QString::number(bitmode) + tr("-bit"));
		}
		actionsI[i]->setData(i);
	}

	for (int i = 0; i < MODES_NUM; i++) {
		actionsI[i]->setCheckable(true);
		archMenu->addAction(actionsI[i]);
	}
	actionsI[0]->setChecked(true);
	connect(group, SIGNAL( triggered(QAction*) ), this, SLOT( setBitMode(QAction*) ));
}

void DisasmTreeView::setBitMode(QAction* action)
{
	QVariant data = action->data();
	int flags = data.toInt();
	if (!myModel) {
		return;
	}
	int bitmode = 0;
	Executable::exe_arch arch = Executable::ARCH_UNKNOWN;
	if (flags < 4) {
		bitmode = 16<<(flags-1);
		arch = Executable::ARCH_INTEL;
	} else {
		bitmode = 16<<((flags - 2)-1);
		arch = Executable::ARCH_ARM;
	}
	myModel->resetDisasmMode(bitmode, arch);
	reset();
}

void DisasmTreeView::changeDisasmViewSettings(DisasmViewSettings &_settings)
{
	if (!myModel) return;
	
	myModel->changeDisasmSettings(_settings);
	resetFont(_settings.myFont);
	reset();
}

void DisasmTreeView::resetFont(const QFont &f)
{
	const int fontDim = ViewSettings::getIconDim(f);
	setFont(f);
	setIconSize(QSize(fontDim, fontDim));

	QHeaderView *verticalHeader = this->verticalHeader();
	if (!verticalHeader) return;

	verticalHeader->setContentsMargins(QMargins(0, 0, 0, 0));

	if (verticalHeader) {
#if QT_VERSION >= 0x050000
		verticalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
		verticalHeader->setMaximumSectionSize(fontDim);
		verticalHeader->resetDefaultSectionSize();
#else
		verticalHeader->setResizeMode(QHeaderView::ResizeToContents);
#endif
	}
}

void DisasmTreeView::setModel(DisasmModel *model)
{
	ExtTableView::setModel(model);
	vHdr.setHexModel(model);

	if (this->myModel) {
		disconnect(this->myModel, SIGNAL(modelUpdated()), this, SLOT(onModelUpdated()));
		disconnect(this->myModel, SIGNAL(scrollReset()), &vScrollbar, SLOT(onReset()));
		disconnect(imgBaseA, SIGNAL( triggered(bool) ), this->myModel, SLOT( setShowImageBase(bool)) );
	}
	this->myModel = model;
	delete this->commentsView;
	this->commentsView = NULL;

	if (this->myModel) {
		connect(this->myModel, SIGNAL(modelUpdated()), this, SLOT(onModelUpdated()));
		connect(this->myModel, SIGNAL(scrollReset()), &vScrollbar, SLOT(onReset()));
	}

	// update actions according to model
	if (this->myModel) {
		bool isImgBase = (myModel->getAddrType() == Executable::VA);
		imgBaseA->setChecked(isImgBase);
		connect(imgBaseA, SIGNAL( triggered(bool) ), this->myModel, SLOT( setShowImageBase(bool)) );

		commentsView = new CommentView(myModel->getPeHandler(), this);
		connect(commentsView, SIGNAL(commentModified()), this, SLOT(reset()));
	}

	this->resizeColumnsToContents();
	this->resizeRowsToContents();
	this->vScrollbar.setModel(myModel);
	initHeader();
}

bool DisasmTreeView::markBranching(QModelIndex index)
{
	if (!index.isValid()) return false;
	if (!this->myModel->myPeHndl || !this->myModel->m_PE) return false;

	offset_t currentRva = this->myModel->getRvaAt(index.row());
	if (this->myModel->m_PE->toRaw(currentRva, Executable::RVA) == INVALID_ADDR) return false; //unmapped area

	offset_t targetRva = this->myModel->getTargetRVA(index);
	if (this->myModel->m_PE->toRaw(targetRva, Executable::RVA) == INVALID_ADDR) return false; //not convertable target

	if (currentRva == INVALID_ADDR || targetRva == INVALID_ADDR) return false;
	this->myModel->setMarkedAddress(currentRva, targetRva);
	return true;
}

void DisasmTreeView::followBranching(QModelIndex index)
{
	if (!index.isValid()) return;
	if (!this->myModel->myPeHndl || !this->myModel->m_PE) return;

	offset_t targetRva = this->myModel->getTargetRVA(index);
	if (targetRva == INVALID_ADDR) {
		return;
	}
	onFollowOffset(targetRva, Executable::RVA);
	return;
}

void DisasmTreeView::emitArgsRVA(const QModelIndex &index)
{
	static offset_t prevTargetRVA = INVALID_ADDR;
	for (int argNum = 0; argNum < Disasm::MAX_ARG_NUM; argNum++) {
		offset_t argRVA = myModel->getArgRVA(argNum, index);
		emit argRvaChanged(argNum, argRVA);
	}
}

void DisasmTreeView::mouseMoveEvent(QMouseEvent *event)
{
	if (!myModel) return;

	QModelIndex index = this->indexAt(event->pos());
	emitArgsRVA(index);

	if (index.column() == DisasmView::DISASM_COL || index.column() == DisasmView::ICON_COL) {
		if (myModel->isClickable(index)) {
			this->setCursor(Qt::PointingHandCursor);
			return;
		}
	}
	this->setCursor(Qt::ArrowCursor);
}

void DisasmTreeView::mousePressEvent(QMouseEvent *event)
{
	QModelIndex index = this->indexAt(event->pos());
	emit currentRvaChanged(myModel->getRvaAt(index.row()));
	emitArgsRVA(index);

	int column = index.column();

	if (this->markBranching(index) && column == DisasmView::ICON_COL) {
		followBranching(index);
	}
	ExtTableView::mousePressEvent(event);
}

void DisasmTreeView::setHovered(QModelIndexList list)
{
	if (!myModel) return;

	const int num = list.size();
	if (num == 0) return;

	std::sort(list.begin(), list.end());
	QModelIndex indexStart = list[0];
	QModelIndex indexEnd = list[num - 1];
	
	offset_t rvaStart = this->myModel->getRvaAt(indexStart.row());
	offset_t rvaEnd = this->myModel->getRvaAt(indexEnd.row());
	if (rvaStart == INVALID_ADDR || rvaEnd == INVALID_ADDR) return;

	bufsize_t bytesNum = myModel->getCurrentChunkSize(indexEnd) + (rvaEnd - rvaStart);
	if (bytesNum == 0) return;

	myModel->myPeHndl->setHovered(true, rvaStart, bytesNum);
}

void DisasmTreeView::selectionChanged(const QItemSelection &newSel, const QItemSelection &prevSel)
{
	QItemSelectionModel* selModel = selectionModel();
	if (selModel == NULL) return;
	this->setHovered(selModel->selectedIndexes());
	ExtTableView::selectionChanged(newSel, prevSel);
}

QModelIndexList DisasmTreeView::uniqOffsets(QModelIndexList list)
{
	QModelIndexList uniqueList;
	std::set<offset_t> uniqueOffsets;

	for (int i = 0; i < list.size(); i++) {
		QModelIndex index = list.at(i);
		offset_t currOffset = myModel->contentOffsetAt(index);

		int sizeBefore = uniqueOffsets.size();
		uniqueOffsets.insert(currOffset);

		if (sizeBefore == uniqueOffsets.size()) {
			//not unique
			continue;
		}
		uniqueList.push_back(index);
	}
	return uniqueList;
}

bool DisasmTreeView::isIndexListContinuous(QModelIndexList &uniqList)
{
	//QModelIndexList uniqueList = uniqueRows(list);
	const int size = uniqList.size();
	if (size == 0) return true;

	std::sort(uniqList.begin(), uniqList.end());

	offset_t nextOffset = INVALID_ADDR;

	for (int i = 0; i < uniqList.size(); i++) {
		QModelIndex index = uniqList.at(i);

		offset_t currOffset = myModel->contentOffsetAt(index);
		if (nextOffset != INVALID_ADDR && nextOffset != currOffset) return false;

		nextOffset = currOffset + myModel->getCurrentChunkSize(index);
	}
	return true;
}

int DisasmTreeView::blockSize(QModelIndexList &uniqList)
{
	const int size = uniqList.size();
	if (size == 0) return 0;
	
	offset_t firstOffset = myModel->contentOffsetAt(uniqList.at(0));
	offset_t lastOffset = myModel->contentOffsetAt(uniqList.at(size - 1));
	if (firstOffset == INVALID_ADDR || lastOffset == INVALID_ADDR) {
		return 0;
	}

	if (lastOffset < firstOffset) {
		printf ("Warning: list is not sorted!\n");
		std::sort(uniqList.begin(), uniqList.end());

		firstOffset = myModel->contentOffsetAt(uniqList.at(0));
		lastOffset = myModel->contentOffsetAt(uniqList.at(size - 1));
	}

	size_t dif = (lastOffset - firstOffset);
	size_t blockSize = dif + myModel->getCurrentChunkSize(uniqList.at(size - 1));
	return blockSize;
}

void DisasmTreeView::reset()
{
	ExtTableView::reset();
	resizeColumnsToContents();
	resizeRowsToContents();
	this->horizontalHeader()->reset();
	this->verticalHeader()->reset();
}

void DisasmTreeView::initHeader()
{
	this->verticalHeader()->setContentsMargins(QMargins(0,0,0,0));
	this->verticalHeader()->setAlternatingRowColors(true);
	this->horizontalHeader()->setMinimumSectionSize(MIN_FIELD_WIDTH);

	this->verticalHeader()->setMinimumSectionSize(MIN_FIELD_HEIGHT);
	this->verticalHeader()->setMinimumWidth(MIN_OFFSET_HDR_WIDTH);

	QString styleSheet = "::section {" // "QHeaderView::section {"
		"background-color: #222222;"
	"}";

	verticalHeader()->setStyleSheet(styleSheet);
	horizontalHeader()->setStretchLastSection(true);

	this->resizeColumnsToContents();
	this->resizeRowsToContents();
}

//---

DisasmModel::DisasmModel(PeHandler *peHndl, QObject *parent)
	: HexDumpModel(peHndl, parent), startOff(0),
	isBitModeAuto(true), archAuto(true), myDisasm(peHndl->getPe())
{
	addrType = Executable::RVA;
	makeIcons(this->settings.getIconSize());
	connectSignals();
}

void DisasmModel::makeIcons(const QSize &vSize)
{
	tracerIcon = ViewSettings::makeScaledIcon(":/icons/space.ico", vSize.width(),  vSize.height());
	tracerUpIcon = ViewSettings::makeScaledIcon(":/icons/space_up.ico", vSize.width(),  vSize.height());
	tracerDownIcon = ViewSettings::makeScaledIcon(":/icons/space_down.ico", vSize.width(), vSize.height());
	tracerSelf = ViewSettings::makeScaledIcon(":/icons/space_this.ico", vSize.width(), vSize.height());

	callUpIcon = ViewSettings::makeScaledIcon(":/icons/up.ico", vSize.width(),  vSize.height());
	callDownIcon = ViewSettings::makeScaledIcon(":/icons/down.ico", vSize.width(),  vSize.height());
	callWrongIcon = ViewSettings::makeScaledIcon(":/icons/wrong_way.ico", vSize.width(),  vSize.height());
	tagIcon = ViewSettings::makeScaledIcon(":/icons/star.ico", vSize.width(),  vSize.height());
}

void DisasmModel::rebuildDisamTab()
{
	if (this->isBitModeAuto) {
		this->bitMode = (m_PE) ? m_PE->getBitMode() : 32;
	}
	if (this->archAuto) {
		this->arch = (m_PE) ? m_PE->getArch() : Executable::ARCH_INTEL;
	}
	myDisasm.init(startOff, this->arch, (Executable::exe_bits) this->bitMode);
	myDisasm.fillTable();
	reset();
	emit modelUpdated();
}

void DisasmModel::resetDisasmMode(uint8_t bitMode, Executable::exe_arch arch)
{
	if (!m_PE) return;
	if (bitMode != 16 && bitMode != 32 && bitMode != 64) {
		this->isBitModeAuto = true;
		this->bitMode = m_PE->getBitMode();
	} else {
		this->isBitModeAuto = false;
		this->bitMode = bitMode;
	}
	if (arch == Executable::ARCH_UNKNOWN) {
		this->archAuto = true;
		this->arch = m_PE->getArch();
	} else {
		this->archAuto = false;
		this->arch = arch;
	}
	rebuildDisamTab();
}

void DisasmModel::setShowImageBase(bool flag)
{ 
	this->addrType = (flag) ? Executable::VA : Executable::RVA;
	reset();
}

int DisasmModel::columnCount(const QModelIndex &parent) const
{
	return DISASM_COL_NUM;
}

QVariant DisasmModel::horizHeader(int section, int role) const
{
	if (role == Qt::FontRole) {
		QFont hdrFont = settings.myFont;
		hdrFont.setBold(true);
		hdrFont.setItalic(false);
		return hdrFont;
	}
	
	if (role == Qt::SizeHintRole) {
		return QVariant();
	}
	
	if (role != Qt::DisplayRole) return QVariant();

	switch (section) {
		case HEX_COL: return tr(" Hex ");
		case DISASM_COL: return tr(" Disasm ");
		case HINT_COL: return tr(" Hint ");
	}
	return QVariant();
}


QVariant DisasmModel::verticHeader(int section, int role) const
{
	/* independent from value */
	
	if (role == Qt::FontRole) {
		QFont hdrFont = settings.myFont;
		hdrFont.setBold(true);
		hdrFont.setItalic(false);
		return hdrFont;
	}
	if (role == Qt::SizeHintRole) {
		return settings.getVerticalSize();
	}

	/* dependend from value */
	int y = section;
	if (y >= myDisasm.chunksCount()) return QVariant();
	offset_t rva = this->getRvaAt(y);

	if (rva == INVALID_ADDR) {
		if (role == Qt::DisplayRole) return tr("<invalid>");
		if (role == Qt::ToolTipRole) {
			const offset_t raw = this->getRawAt(y);
			return tr("Not mapped. Raw = 0x") + QString::number(raw, 16);
		}
		if (role == Qt::ForegroundRole) return QColor("red");
	}

	//rva valid
	if (role == Qt::DisplayRole) {
		if (this->addrType == Executable::VA) {
			rva = m_PE->rvaToVa(rva);
		}
		return QString::number(rva, 16).toUpper();
	}

	if (role == Qt::ToolTipRole) {
		return QString::number(rva, 16).toUpper() +"\n"+ tr("Right click to follow");
	}

	if (!myDisasm.isRvaContnuous(y)) {
		if (role == Qt::ForegroundRole) return QColor("magenta");
	}

	DWORD ep = m_PE->getEntryPoint();
	size_t disChunk = myDisasm.getChunkSize(y);

	if (ep >= rva && ep < (rva + disChunk)) {
		if (role == Qt::ForegroundRole) return QColor("cyan");
		if (role == Qt::ToolTipRole) return tr("Entry Point = ") + QString::number(ep, 16).toUpper();
	}
	// normal color
	if (role == Qt::ForegroundRole) return QColor("white");

	return QVariant();
}

QVariant DisasmModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	switch (orientation) {
		case Qt::Horizontal : return horizHeader(section, role);
		case Qt::Vertical : return verticHeader(section, role);
	}
	return QVariant();
}

Qt::ItemFlags DisasmModel::flags(const QModelIndex &index) const 
{
	Qt::ItemFlags f = Qt::ItemIsEnabled;
	int col = index.column();

	if (col == HEX_COL || col == DISASM_COL || col == HINT_COL)
		f |= Qt::ItemIsSelectable;

	if (col == HINT_COL || col == HEX_COL) {
		f |=  Qt::ItemIsEditable;
	}
	return f;
}

QString DisasmModel::getAsm(int index) const
{
	QString str = myDisasm.mnemStr(index);
	
	if (myDisasm.isBranching(index)) {
		str = myDisasm.translateBranching(index);
	}
	return str;
}

QVariant DisasmModel::getHint(const QModelIndex &index) const
{
	if (!index.isValid()) return false;

	QStringList hints;
	int y = index.row();

	/* push ... ret = CALL */
	if (myDisasm.isPushRet(y) && myDisasm.isImmediate(y)) {
		int32_t val = myDisasm.getImmediateVal(y);
		hints.append("RET -> CALL 0x" + QString::number(val, 16));
	}
	if (myDisasm.isAddrOperand(y)) {
		bool isOk = false;
		offset_t targetRva = myDisasm.getTargetRVA(y, isOk);
		if (targetRva != INVALID_ADDR) {
			QString str = myDisasm.getStringAt(targetRva);
			if (str.length() > 0) {
				hints.append(str);
			}
		}
	}
	QString comment = getComment(myDisasm.getRvaAt(y));
	if (comment.length() > 0) {
		hints.append(comment);
	}
	if (hints.size()) {
		return hints.join(" ; ");
	}
	return QVariant();
}

bool DisasmModel::isClickable(const QModelIndex &index) const
{
	bool isValid = false;
	if (index.isValid() == false) return false;
	
	int y = index.row();
	return myDisasm.isFollowable(y);
}

uint32_t DisasmModel::getCurrentChunkSize(const QModelIndex &index) const
{
	bool isValid = false;
	if (index.isValid() == false) return 0;
	return myDisasm.getChunkSize(index.row());
}

offset_t DisasmModel::getTargetRVA(const QModelIndex &index) const
{
	if (index.isValid() == false)  {
		return INVALID_ADDR;
	}
	bool isOk = false;
	offset_t targetRva = myDisasm.getTargetRVA(index.row(), isOk);
	if (targetRva == INVALID_ADDR || !isOk) return INVALID_ADDR;
	return targetRva;
}

offset_t DisasmModel::getArgRVA(const int argNum, const QModelIndex &index) const
{
	if (index.isValid() == false) {
		return INVALID_ADDR;
	}
	bool isOk = false;
	offset_t argRva = myDisasm.getArgRVA(index.row(), argNum, isOk);
	if (argRva == INVALID_ADDR || !isOk) return INVALID_ADDR;
	return argRva;
}

QVariant DisasmModel::getRawContentAt(const QModelIndex &index) const
{
	offset_t indx = contentOffsetAt(index);
	if (indx == INVALID_ADDR) return QVariant();

	const size_t chunkSize = this->getCurrentChunkSize(index);
	if (!chunkSize) return QVariant();

	QByteArray bytes;
	for (size_t i = 0; i < chunkSize; i++) {
		BYTE* contentPtr = m_PE->getContentAt(indx + i, 1);
		if (!contentPtr) break;

		char c = contentPtr[0];
		bytes.append(c);
	}
	return bytes;
}

void DisasmModel::setMarkedAddress(uint64_t cRva, uint64_t tRva)
{
	if (!myPeHndl) return;
	myPeHndl->markedBranching(cRva, tRva);
	reset();
}

bool DisasmModel::setHexData(offset_t offset, const size_t bytesCount, const QString &data)
{
	if (offset == INVALID_ADDR || !myPeHndl || !m_PE) {
		return false;
	}

	BYTE* contentPtr = m_PE->getContentAt(offset, bytesCount);
	if (!contentPtr) {
		return false;
	}

	TempBuffer temp;
	const size_t chunkSize = bytesCount;
	temp.init(contentPtr, chunkSize); //alloc & fill the buffer with the previous content

	BYTE *chunk = temp.getContent();
	if (!chunk) return false;
	
	// convert hex string to bytes:
	size_t modifBytes = 0;
	for (size_t i = 0; i < bytesCount * 2 && i < data.size(); i += 2) {

		QString text = data.mid(i, 2);
		bool isConv = false;
		BYTE number = text.toUShort(&isConv, 16);
		if (!isConv) {
			return false;
		}
		if (modifBytes >= chunkSize) break;
		chunk[modifBytes++] = number;
	}
	if (memcmp(contentPtr, chunk, modifBytes) == 0) {
		// not modified
		return false;
	}
	myPeHndl->backupModification(offset, modifBytes);
	memcpy(contentPtr, chunk, modifBytes);
	myPeHndl->setBlockModified(offset, modifBytes);
	return true;
}

bool DisasmModel::setData(const QModelIndex &index, const QVariant &val, int role)
{
	if (!index.isValid() || !myPeHndl) {
		return false;
	}

	const int y = index.row();
	const int x = index.column();

	const offset_t rva = this->getRvaAt(y);
	const offset_t raw = this->getRawAt(y);

	size_t disChunk = this->getChunkSize(y);
	
	if (x == HEX_COL) {
		return setHexData(raw, disChunk, val.toString());
	}
	if (rva == INVALID_ADDR) {
		return false;
	}
	if (x == HINT_COL) {
		return setComment(rva, val.toString());
	}
	return false;
}


QVariant DisasmModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || !myPeHndl) {
		return QVariant();
	}

	if (role == Qt::FontRole) {
		return settings.myFont;
	}
	
	const int y = index.row();
	const int x = index.column();
	
	if (role == Qt::SizeHintRole) {
		if (x == TAG_COL) {
			const QSize iconSize = this->settings.getIconSize();
			return QSize(iconSize.width() + 2, iconSize.height());
		}
		return QVariant();
	}
	
	if (x >= DISASM_COL_NUM || y >= myDisasm.chunksCount()) return QVariant();

	offset_t rva = this->getRvaAt(y);
	const size_t disChunk = this->getChunkSize(y);

	/* TAG */
	if (x == TAG_COL) {
		if (role != Qt::DecorationRole && role != Qt::ToolTipRole) return QVariant();
		QString comment = this->getComment(rva);
		if (comment.size() > 0 ) {
			if (role == Qt::DecorationRole) return tagIcon;
			if (role == Qt::ToolTipRole) return comment;
		}
		return QVariant();
	}
	
	if (rva != INVALID_ADDR && x == PTR_COL && role == Qt::DecorationRole) {
		if (rva == myPeHndl->markedOrigin && rva == myPeHndl->markedTarget) {
			return tracerSelf;
		}

		if (rva > myPeHndl->markedOrigin && rva < myPeHndl->markedTarget 
			|| rva > myPeHndl->markedTarget && rva < myPeHndl->markedOrigin)
		{
			return tracerIcon;
		}

		if ((myPeHndl->markedTarget == rva && myPeHndl->markedTarget > myPeHndl->markedOrigin) 
			|| (myPeHndl->markedOrigin == rva && myPeHndl->markedTarget < myPeHndl->markedOrigin))
		{
			return tracerUpIcon;
		}

		if ((myPeHndl->markedTarget == rva && myPeHndl->markedTarget < myPeHndl->markedOrigin) 
			|| (myPeHndl->markedOrigin == rva && myPeHndl->markedTarget > myPeHndl->markedOrigin))
		{
			return tracerDownIcon;
		}
	}

	if ( myDisasm.isCallToRet(y)) {
		if (role == Qt::ForegroundRole) return settings.nopColor;
		if (role == Qt::DisplayRole && x == HINT_COL) return "(CALL -> RET) == NOP";
	}
	if (this->myDisasm.isBranching(y) && !this->myDisasm.isUnconditionalBranching(y)) {
		if (role == Qt::ForegroundRole && x != HINT_COL) return settings.conditionalColor;
	}
	
	//---------------------
	bool isOk = false;
	offset_t tRva =  myDisasm.getTargetRVA(y, isOk);

	if (isClickable(index) && isOk) {
		if (role == Qt::BackgroundRole) return settings.branchingColor;

		int32_t lval =  myDisasm.getTargetDelta(y);
		bool isDelay = false;
		// is Import Call ?
		QString funcName = myPeHndl->importDirWrapper.thunkToFuncName(tRva, false);
		if (funcName.length() == 0 ) {
			// is Delay Import Call ?
			isDelay = true;
			funcName = myPeHndl->delayImpDirWrapper.thunkToFuncName(tRva, false);
		}

		if (funcName.length() > 0) {
			if (x == HINT_COL && (role == Qt::DisplayRole || role == Qt::ToolTipRole )) {
				QString name = funcName;

				QString comment = getComment(rva);
				if (comment.size() > 0) name += " ; " + comment;
				return name;
			}
			if (role == Qt::ForegroundRole) {
				if (isDelay) return settings.delayImpColor;
				return this->settings.importColor;
			}
		}

		uint64_t imgSize = m_PE->getImageSize();
		if (x == ICON_COL) {
			
			if (role == Qt::DecorationRole) {
				if (tRva >= imgSize || myDisasm.getRawAt(y) == INVALID_ADDR) {
					return callWrongIcon;
				}
				if (lval < 0) return callUpIcon;
				return callDownIcon;
			}
			
			if (role == Qt::ToolTipRole) {
				if (tRva >= imgSize) {
					return tr("RVA out of ImageSize: 0x") + QString::number(imgSize, 16);
				}
				if (lval < 0) {
					return "$- 0x" + QString::number(~lval + 1, 16);
				}
				return "$+ 0x" + QString::number(lval, 16);
			}
		}
		
		if (role == Qt::ToolTipRole && x == DISASM_COL) {
			QString s = tr("Click arrow icon to follow  (RVA: ") + QString::number(tRva, 16).toUpper() + ")";
			return s;
		}
	}

	if (role == Qt::ForegroundRole) {
		using namespace minidis;
		minidis::mnem_type mnem = myDisasm.getMnemType(y);
		switch (mnem) {
			case MT_RET : return settings.retColor;
			case MT_NOP : return settings.nopColor;
			case MT_INT3 : return settings.int3Color;
			case MT_INTX : return settings.intXColor;
			case MT_CALL : return settings.callColor;
			case MT_JUMP: return settings.jumpColor;
			case MT_INVALID : return settings.invalidColor;
			default : return QVariant();
		}
	}
	if (role == Qt::ToolTipRole && x == HEX_COL) {
		return tr("Double-click to edit");
	}
	if (x == HEX_COL) {
		if (role == Qt::DisplayRole || role == Qt::ToolTipRole || role == Qt::EditRole) {
			return myDisasm.getHexStr(y).toUpper();
		}
	}
	if (x == DISASM_COL) {
		if (role == Qt::DisplayRole || role == Qt::ToolTipRole || role == Qt::EditRole) {
			return getAsm(y).toUpper();
		}
	}
	if (x == HINT_COL) {
		if (role == Qt::EditRole) {
			return this->getComment(rva);
		}
		if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
			return getHint(index);
		}
	}
	return QVariant();
}

//--------------------------------------------------------------------

#include "PEFileTreeModel.h"
#include "ViewSettings.h"

#define MAX_COL 1
///------------------------

PEFileTreeItem::PEFileTreeItem(PeHandler *peHndl, level_t level, enum PEFileFieldId role, PEFileTreeItem *parent)
	: PeTreeItem(peHndl, role, FIELD_NONE, parent), role(PEFILE_NONE), level(DESC),
	entryPointItem(NULL)
{
	m_parentItem = parent;
	this->level = level;
	this->role = role;

	if (!this->myPeHndl || !m_PE) {
		this->level = DESC;
		if (role != PEFILE_ROOT) this->role = PEFILE_NONE;
		return;
	}

	if (role == PEFILE_MAIN) {
		createTree();
		connect(this->myPeHndl, SIGNAL(modified()), this, SLOT(onModified()));
	}
}

PEFileTreeItem* PEFileTreeItem::findNodeByRole(int my_role)
{
	PEFileTreeItem *searchedNode = NULL;
	for (auto itr = this->m_childItems.begin(); itr != this->m_childItems.end(); ++itr) {
		PEFileTreeItem *node = dynamic_cast<PEFileTreeItem*>(*itr);
		if (!node) continue;

		if (node->role == my_role) {
			searchedNode = node;
			break;
		}
	}
	return searchedNode;
}

void PEFileTreeItem::deleteOverlayNode()
{
	PEFileTreeItem *overlayNode = findNodeByRole(PEFILE_OVERLAY);
	if (overlayNode) {
		this->detachChild(overlayNode);
		delete overlayNode;
	}
}

void PEFileTreeItem::appendOverlayNode()
{
	PEFileTreeItem *overlayNode = findNodeByRole(PEFILE_OVERLAY);
	if (overlayNode) return;

	overlayNode = new PEFileTreeItem(this->myPeHndl, PEFileTreeItem::DESC, PEFILE_OVERLAY);
	appendChild(overlayNode);
}

void PEFileTreeItem::createTree()
{
	this->fileName = this->myPeHndl->getShortName();

	for (int indx = PEFILE_IMG_DOS_HDR; indx < PEFILE_FIELD_COUNTER; indx++) {
		enum PEFileFieldId role = PEFileFieldId(indx);

		if (role == PEFILE_OVERLAY) {
			if (getOverlaySize()) {
				appendOverlayNode();
			}
			continue;
		}

		if (role == PEFILE_SECTIONS) {
			PEFileSectionsTreeItem *secRoot = new PEFileSectionsTreeItem(this->myPeHndl, PEFileTreeItem::DESC);
			appendChild(secRoot);
			connect(this->myPeHndl, SIGNAL(secHeadersModified()), secRoot, SLOT(onSectionNumChanged()));
		}
		else if (role == PEFILE_IMG_NT_HDRS)
			appendChild(new PEFileNTHdrTreeItem(this->myPeHndl, PEFileTreeItem::DESC));
		else {
			appendChild(new PEFileTreeItem(this->myPeHndl, PEFileTreeItem::DESC, role));
		}
	}
	if (entryPointItem) {
		entryPointItem->setParent(NULL);
		delete entryPointItem;
	}
	entryPointItem = new PEFileEntryPointItem(this->myPeHndl, this, NULL);
	if (!this->attachIfBelongs(entryPointItem)) {
		entryPointItem->setParent(this);
	}
}

bool PEFileTreeItem::attachIfBelongs(PEFileTreeItem *item)
{
	if (!item || !m_PE) return false;
	if (this == item) return false;

	// first try to attach the element to children:
	for (QList<TreeItem*>::iterator itr = m_childItems.begin(); itr != m_childItems.end(); ++itr) {
		PEFileTreeItem *child = dynamic_cast<PEFileTreeItem*>(*itr);
		if (!child) continue;

		if (child->attachIfBelongs(item)) {
			return true;
		}
	}

	// if failed, try with the current element:
	const offset_t from = this->getContentOffset();
	if (from == INVALID_ADDR) {
		return false;
	}
	const bufsize_t size = this->getContentSize();
	const offset_t to = from + size;

	const offset_t itemFrom = item->getContentOffset();
	if (itemFrom >= from && itemFrom < to) {
		//std::cout << "> Attached. ItemFrom: " << std::hex << itemFrom << " Searched in role: " << this->role << " bounds: " << from << " to " << to << " ( size " << size << " )" << std::endl;
		this->appendChild(item);
		return true;
	}
	return false;
}

offset_t PEFileTreeItem::getContentOffset() const
{
	if (!this->myPeHndl) return 0;
	PEFile *m_PE = this->myPeHndl->getPe();

	if (!m_PE->getContent()) return 0;

	switch (role) {
		case PEFILE_MAIN :
		{
			return 0;
		}
		case PEFILE_IMG_DOS_HDR :
			return 0;
		case PEFILE_DOS_STUB :
			return sizeof(IMAGE_DOS_HEADER);
		case PEFILE_IMG_NT_HDRS : 
			return m_PE->peNtHdrOffset();
		case PEFILE_SEC_HDRS :
			return m_PE->secHdrsOffset();
		case PEFILE_OVERLAY :
			return m_PE->getLastMapped(Executable::RAW);
	}
	return 0;
}

bufsize_t PEFileTreeItem::getContentSize() const
{
	if (!this->myPeHndl) return 0;
	PEFile *m_PE = this->myPeHndl->getPe();

	if (!m_PE || !m_PE->getContent()) return 0;
	
	const bufsize_t totalSize = m_PE->getRawSize();
	const offset_t offset = getContentOffset();
	
	if (((offset_t)totalSize) <= offset) return 0;

	switch (role) {
		case PEFILE_MAIN :
		{
			return totalSize;
		}
		case PEFILE_DOS_STUB :
		{
			const offset_t ntHdrsOffset = m_PE->peNtHdrOffset();
			if (ntHdrsOffset < offset) {
				return 0;
			}
			return ntHdrsOffset - offset;
		}
		case PEFILE_IMG_DOS_HDR :
			return sizeof(IMAGE_DOS_HEADER);
		case PEFILE_IMG_NT_HDRS :
			return  m_PE->peNtHeadersSize();
		case PEFILE_SEC_HDRS:
		{
			return m_PE->secHdrsEndOffset() - offset;
		}
		case PEFILE_OVERLAY:
			return getOverlaySize();
	}
	return totalSize;
}

bufsize_t PEFileTreeItem::getOverlaySize() const
{
	if (!this->myPeHndl) return 0;
	PEFile *m_PE = this->myPeHndl->getPe();

	if (!m_PE || !m_PE->getContent()) return 0;
	
	if (m_PE->getRawSize() > m_PE->getLastMapped(Executable::RAW)) {
		return m_PE->getRawSize() - m_PE->getLastMapped(Executable::RAW);
	}
	return 0;
}

void PEFileTreeItem::onModified()
{
	if (this->entryPointItem == NULL) {
		//printf("EP Item is NULL, recreating...\n");
		entryPointItem = new PEFileEntryPointItem(this->myPeHndl, this, this);
	}

	entryPointItem->savedEP = this->m_PE->getEntryPoint();
	if (!this->attachIfBelongs(entryPointItem)) {
		entryPointItem->setParent(this);
	}
	if (getOverlaySize()) {
		appendOverlayNode();
	} else {
		deleteOverlayNode(); // the PE has no overlay
	}
	emit needReset();
}

int PEFileTreeItem::columnCount() const
{
	return MAX_COL;
}

QVariant PEFileTreeItem::background(int column) const
{
	if (!this->myPeHndl) return QVariant();
	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return QVariant();

	if (role == PEFILE_MAIN && m_PE->isTruncated()) {
		QColor color(ERR_COLOR);
		color.setAlpha(100);
		return color;
	}
	return QVariant();
}

QVariant PEFileTreeItem::foreground(int column) const
{
	int fieldIndx = column;
	return QVariant();
}

QVariant PEFileTreeItem::toolTip(int column) const
{
	if (!this->myPeHndl) return QVariant();
	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return QVariant();

	int fieldIndx = column;
	if (this->level == DESC) {
		if (role == PEFILE_MAIN) {
			QString truncated = m_PE->isTruncated() ? "\n(truncated)" : "";
			QString resized = m_PE->isResized() ? "\n(resized)" : "";
			return myPeHndl->getFullName() + truncated + resized;
		}
		if (role == PEFILE_OVERLAY) {
			return "Overlay size: 0x" + QString::number(this->getOverlaySize(), 16);
		}
	}
	return data(column);
}

QVariant PEFileTreeItem::decoration(int column) const 
{
	if (!this->myPeHndl) return QVariant();

	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return QVariant();

	if (role == PEFILE_MAIN) {
		if (this->myPeHndl->isPacked()) {
			return ViewSettings::getScaledPixmap(":/icons/Locked.ico");
		}
		
		if (m_PE->getBitMode() == Executable::BITS_64) {
			if (myPeHndl->isPeValid()) {
				return ViewSettings::getScaledPixmap(":/icons/app64.ico");
			}
			else {
				return ViewSettings::getScaledPixmap(":/icons/app64_w.ico");
			}
		} else {
			if (myPeHndl->isPeValid()) {
				return ViewSettings::getScaledPixmap(":/icons/app32.ico");
			}
			else {
				return ViewSettings::getScaledPixmap(":/icons/app32_w.ico");
			}
		}
	}
	if (role == PEFILE_DOS_STUB) {
		return ViewSettings::getScaledPixmap(":/icons/dos.ico");
	}
	return ViewSettings::getScaledPixmap(":/icons/hdr.ico");
}

QVariant PEFileTreeItem::font(int column) const
{
	if (role != PEFILE_MAIN) {
		return QVariant();
	}
	QFont f = QApplication::font();
	f.setBold(true);
	if (this->myPeHndl && this->myPeHndl->isFileOnDiskChanged())  {
		f.setItalic(true);
	}
	return f;
}

BYTE* PEFileTreeItem::getContent()
{
	if (!this->myPeHndl) return NULL;
	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return NULL;

	offset_t offset = getContentOffset();
	bufsize_t size = getContentSize();
	
	BYTE *content = m_PE->getContentAt(offset, size);
	return content;
}

QVariant PEFileTreeItem::data(int column) const
{
	if (!this->myPeHndl) return QVariant();
	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return QVariant();

	int fieldIndx = column;
	if (fieldIndx != 0) return QVariant();

	if (this->level == DESC) {
		switch (role) {
			case PEFILE_MAIN :
			{
				bool isResized = m_PE->isResized();
				if (this->myPeHndl->isPEModified() || isResized) 
					return fileName + "*";
				return fileName;
			}
			case PEFILE_IMG_DOS_HDR: return "DOS Header";
			case PEFILE_DOS_STUB: return "DOS stub";
			case PEFILE_IMG_NT_HDRS: return "NT Headers";
			case PEFILE_SEC_HDRS: return "Section Headers";
			case PEFILE_SECTIONS: return "Sections";
			case PEFILE_OVERLAY: return "Overlay";
		}
		return QVariant();
	}
	return QVariant();
}

//-------------------

PEFileSectionsTreeItem::PEFileSectionsTreeItem(PeHandler *peHndl, level_t level, int secIndx, PEFileTreeItem *parent)
	: PEFileTreeItem(peHndl, level, role = PEFILE_SECTIONS, parent)
{
	SectionHdrWrapper* entrySec = m_PE->getEntrySection();
	this->secIndx = secIndx;
	if (this->level == DESC) {
		this->secIndx = (-1);
		//create subrecords
		int secNum = this->m_PE->getSectionsCount();
		for (int i = 0; i < secNum; i++) {
			PEFileSectionsTreeItem *sec = new PEFileSectionsTreeItem(this->myPeHndl, DETAILS, i, this);
			this->appendChild(sec);
		}
	}
}

void PEFileSectionsTreeItem::onSectionNumChanged()
{
	size_t oldNum = this->m_childItems.size();
	if (oldNum == this->m_PE->getSectionsCount()) return;
	int diff = 0;
	bool add = true;

	if (oldNum < this->m_PE->getSectionsCount()) {
		add = true;
		diff = this->m_PE->getSectionsCount() - oldNum;
	} else {
		add = false;
		diff = oldNum - this->m_PE->getSectionsCount();
	}

	for (int i = 0; i < diff; i++) {
		if (add) {
			PEFileSectionsTreeItem *sec = new PEFileSectionsTreeItem(this->myPeHndl, DETAILS, oldNum + i, this);
			this->appendChild(sec);
		} else {
			TreeItem *child = this->child((oldNum - 1) - i);
			this->detachChild(child);
			delete child;
		}
	}
}

SectionHdrWrapper* PEFileSectionsTreeItem::getMySection() const
{
	if (!m_PE) return NULL;
	return m_PE->getSecHdr(secIndx);
}

QVariant PEFileSectionsTreeItem::data(int column) const
{
	if (!m_PE) return QVariant();
	if (column != 0) return QVariant();

	if (this->level == DESC) return ("Sections");
	SectionHdrWrapper *sec = getMySection();
	if (sec == NULL) return QVariant();
	return sec->mappedName;
}

QVariant PEFileSectionsTreeItem::background(int column) const
{
	return QVariant();
}

QVariant PEFileSectionsTreeItem::decoration(int column) const
{
	SectionHdrWrapper *sec = getMySection();
	if (sec == NULL) return QVariant();
	if (!m_PE) return QVariant();

	if (this->level == DESC) return QVariant();
	
	if (sec == m_PE->getEntrySection() && sec != NULL) {
		return ViewSettings::getScaledPixmap(":/icons/EP.ico");
	}
	return ViewSettings::getScaledPixmap(":/icons/section.ico");
}

offset_t PEFileSectionsTreeItem::getContentOffset() const
{
	if (!m_PE) return 0;
	SectionHdrWrapper *sec = getMySection();

	if (sec == NULL) {
		if (m_PE->getSectionsCount() == 0)
			return 0;

		SectionHdrWrapper *firstSec = m_PE->getSecHdr(0); //TODO: get first Section by Raw Address!
		if (!firstSec)
			return 0;

		return firstSec->getRawPtr();
	}
	return sec->getRawPtr();
}

BYTE* PEFileSectionsTreeItem::getContent()
{
	if (!m_PE) return NULL;
	BYTE *content = m_PE->getContent();
	if (!content) return NULL;

	SectionHdrWrapper *sec = getMySection();
	if (!sec) {
		if (m_PE->getSectionsCount() == 0) return NULL;
		sec = m_PE->getSecHdr(0); //TODO: get first Section by Raw Address!
	}
	if (!sec) return NULL;
	return m_PE->getContentAt(sec->getRawPtr(), 1);;
}

bufsize_t PEFileSectionsTreeItem::getContentSize() const
{
	if (!m_PE) return 0;
	SectionHdrWrapper *sec = getMySection();
	if (sec) {
		return sec->getContentSize(Executable::RAW, true);
	}
	if (m_PE->getSectionsCount() == 0) return 0;
	SectionHdrWrapper *firstSec = m_PE->getSecHdr(0); //TODO: get first Section by Raw Address!
	if (!firstSec) return 0;
	
	BYTE *secContent = m_PE->getSecContent(firstSec);
	if (!secContent) {
		return 0;
	}
	offset_t contentOffset = m_PE->getOffset(secContent);
	if (contentOffset == INVALID_ADDR) {
		return 0;
	}
	const offset_t totalSize = m_PE->getRawSize();
	return totalSize - contentOffset;
}

//-----------------------------------------------------------------------------

PEFileEntryPointItem::PEFileEntryPointItem(PeHandler *peHndl, PEFileTreeItem *mainIt, PEFileTreeItem *parent)
	 : PEFileTreeItem(peHndl, level, role = PEFILE_NONE, parent), mainItem(mainIt)
{
	PEFile *pe = this->myPeHndl->getPe();
	if (!pe) return;
	this->savedEP = pe->getEntryPoint();
}

PEFileEntryPointItem::~PEFileEntryPointItem()
{
	if (mainItem) mainItem->setEntryPointItem(NULL);
}

QVariant PEFileEntryPointItem::decoration(int column) const
{
	PEFile *pe = this->myPeHndl->getPe();
	if (!pe) return QVariant();
	
	return ViewSettings::getScaledPixmap(":/icons/arrow-right.ico");
}

QVariant PEFileEntryPointItem::data(int column) const
{
	if (!this->myPeHndl) return QVariant();
	PEFile *pe = this->myPeHndl->getPe();
	if (!pe) return QVariant();
	
	offset_t offset = getContentOffset();
	if (offset == INVALID_ADDR) {
		return "Invalid EP";
	}
	return "EP = " + QString::number(offset, 16).toUpper();
}

BYTE* PEFileEntryPointItem::getContent()
{
	if (!this->myPeHndl) return NULL;
	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return NULL;

	return m_PE->getContentAt(getContentOffset(), 1);
}

offset_t PEFileEntryPointItem::getContentOffset() const
{
	if (!this->myPeHndl) return INVALID_ADDR;
	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return INVALID_ADDR;

	offset_t offset = m_PE->getEntryPoint();
	try{
		offset = m_PE->rvaToRaw(offset);
	} catch (CustomException(e)) {
		offset = INVALID_ADDR;
	}
	return offset;
}

bufsize_t PEFileEntryPointItem::getContentSize() const
{
	if (!m_PE) return 0;
	BYTE *content = m_PE->getContent();
	if (!content) return 0;
	
	offset_t totalSize = m_PE->getRawSize();
	offset_t offset = getContentOffset();
	if (offset == INVALID_ADDR) return 0;
	if (offset >= totalSize) return 0;

	bufsize_t dif = bufsize_t(totalSize - offset);
	return dif;
}
//-----------------------------------------

PEFileNTHdrTreeItem::PEFileNTHdrTreeItem(PeHandler *peHndl, level_t level, enum PEFileNTHdrFieldId subrole, PEFileTreeItem *parent)
	: PEFileTreeItem(peHndl, level, role = PEFILE_IMG_NT_HDRS, parent)
{
	this->subrole = subrole;
	
	//this->icon = QPixmap (":/icons/hdr.ico");
	if (this->level == DESC) {
		this->subrole = PEFILE_NTHDR_NONE;
		//create subrecords
		for (int i = PEFILE_NTHDR_SIGN; i < PEFILE_NTHDR_COUNTER; i++) {
			this->appendChild(new PEFileNTHdrTreeItem(this->myPeHndl, DETAILS,  PEFileNTHdrFieldId(i), this));
		}
	}
}

QVariant PEFileNTHdrTreeItem::data(int column) const
{
	if (!this->myPeHndl) return QVariant();
	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return QVariant();

	if (column != 0) return QVariant();

	if (this->level == DESC) return ("NT Headers");
	switch (subrole) {
		case PEFILE_NTHDR_SIGN : return "Signature";
		case PEFILE_NTHDR_FILEHDR : return "File Header";
		case PEFILE_NTHDR_OPTHDR : return "Optional Header";
	}
	return QVariant();
}

offset_t PEFileNTHdrTreeItem::getContentOffset() const
{
	if (!this->myPeHndl) return 0;
	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return 0;

	if (!m_PE->getContent()) return 0;
	BYTE *content = m_PE->getContent();

	offset_t offset = m_PE->peNtHdrOffset();

	if (this->level == DESC) 
		return offset;

	switch (subrole) {
		case PEFILE_NTHDR_SIGN :
			return offset;
		case PEFILE_NTHDR_FILEHDR :
			return offset + sizeof(pe::S_NT);
		case PEFILE_NTHDR_OPTHDR :
			return offset + sizeof(pe::S_NT) + sizeof(IMAGE_FILE_HEADER);
	}
	
	return offset;
}

bufsize_t PEFileNTHdrTreeItem::getContentSize() const
{
	if (!this->myPeHndl) return 0;
	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return 0;

	BYTE *content = m_PE->getContent();
	offset_t totalSize = m_PE->getRawSize();

	offset_t offset = m_PE->peNtHdrOffset();
	bufsize_t contentSize = bufsize_t(m_PE->secHdrsOffset() - offset);

	if (this->level == DESC) 
		return contentSize;
	
	switch (subrole) {
		case PEFILE_NTHDR_SIGN :
			return sizeof(pe::S_NT);

		case PEFILE_NTHDR_FILEHDR :
			return sizeof(IMAGE_FILE_HEADER);

		case PEFILE_NTHDR_OPTHDR :
			return contentSize - (sizeof(pe::S_NT) + sizeof(IMAGE_FILE_HEADER));
	}

	return contentSize;
}

BYTE* PEFileNTHdrTreeItem::getContent()
{
	if (!this->myPeHndl) return NULL;
	PEFile *m_PE = this->myPeHndl->getPe();
	if (!m_PE) return NULL;

	const offset_t offset = this->getContentOffset();
	return m_PE->getContentAt(offset, 1);
}

//-----------------------------------------------------------------------------
PEFileTreeModel::PEFileTreeModel(QObject *parent)
	: QAbstractItemModel(parent)
{
	root = new PEFileTreeItem(NULL, PEFileTreeItem::DESC, PEFILE_ROOT);
}

PEFileTreeModel::~PEFileTreeModel()
{
	delete root; root = NULL;
	loadedPeFiles.clear();
}

void PEFileTreeModel::refreshView()
{
	this->reset();
	emit modelUpdated();
}

QModelIndex PEFileTreeModel::addHandler(PeHandler *peHndl)
{
	if (!peHndl) return QModelIndex();
	PEFile *pe = peHndl->getPe();
	if (!pe) return QModelIndex();

	PEFileTreeItem *item = new PEFileTreeItem(peHndl, PEFileTreeItem::DESC, PEFILE_MAIN);
	connect(item, SIGNAL(needReset()), this, SLOT(refreshView()));
	connect(peHndl, SIGNAL(secHeadersModified()), this, SLOT(refreshView()));

	root->appendChild(item);
	loadedPeFiles[pe] = item;
	this->reset();
	emit modelUpdated();

	return createIndex(item->row(), 0, item);
}

void PEFileTreeModel::deleteHandler(PeHandler *peHndl)
{
	if (!peHndl) return;
	PEFile *pe = peHndl->getPe();
	if (!pe) return;

	PEFileTreeItem *item = loadedPeFiles[pe];
	if (!item) return;

	disconnect(item, SIGNAL(needReset()), this, SLOT(refreshView()));

	loadedPeFiles.erase(pe);
	
	TreeItem *parent = item->m_parentItem;
	if (parent) {
		parent->detachChild(item);
	}
	delete item;
	this->reset();
	emit modelUpdated();
}

int PEFileTreeModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0)
		return 0;

	PEFileTreeItem *parentItem = root;
	if (parent.isValid())
		parentItem = static_cast<PEFileTreeItem*>(parent.internalPointer());

	if (!parentItem)
		return 0;
	return parentItem->childCount();
}

int PEFileTreeModel::columnCount(const QModelIndex &parent) const
{
	if (!root) return 0;

	if (parent.isValid())
		return static_cast<PEFileTreeItem*>(parent.internalPointer())->columnCount();

	return root->columnCount();
}

QVariant PEFileTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	PEFileTreeItem *item = static_cast<PEFileTreeItem*>(index.internalPointer());
	if (item == NULL) return QVariant();
	switch (role) {
		case Qt::BackgroundColorRole : return item->background(index.column());
		case Qt::SizeHintRole :
		{
			return QVariant(); //get default
		}
		case Qt::ForegroundRole : return item->foreground(index.column());
		case Qt::DisplayRole : return item->data(index.column());
		case Qt::ToolTipRole : return item->toolTip(index.column());
		case Qt::WhatsThisRole : return item->whatsThis(index.column());
		case Qt::DecorationRole : return item->decoration(index.column());
		case Qt::FontRole : return item->font(index.column());
	}
	return QVariant();
}

QVariant PEFileTreeModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
{
	if(role != Qt::DisplayRole) return QVariant();

	return QVariant();
}

Qt::ItemFlags PEFileTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex PEFileTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	TreeItem *parentItem = root;
	if (parent.isValid()) {
		parentItem = static_cast<PEFileTreeItem*>(parent.internalPointer());
	}
	TreeItem* childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}

QModelIndex PEFileTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	PEFileTreeItem *childItem = static_cast<PEFileTreeItem*>(index.internalPointer());
	if (childItem == NULL)
		return QModelIndex();

	TreeItem* parentItem = childItem->m_parentItem;

	if (!parentItem || parentItem == root)
		return QModelIndex();
	if (parentItem->row() == (-1)) {
		//printf("Invalid row value\n");
		return QModelIndex();
	}
	return createIndex(parentItem->row(), 0, parentItem);
}
//-----------------------------------------------------------------

PEStructureView::PEStructureView(QWidget *parent)
	: TreeCpView(parent), peTreeModel(NULL)
{
	setHeaderHidden(true);
	setAutoFillBackground(true);
	setAutoScroll(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	this->setCursor(Qt::PointingHandCursor);
	this->setSelectionMode(SingleSelection);
	setFocusPolicy(Qt::WheelFocus);
}

void PEStructureView::setModel(PEFileTreeModel *model)
{ 
	if (this->peTreeModel) {
		disconnect(this->peTreeModel, SIGNAL(modelUpdated()), this, SLOT(onModelUpdated()));
	}

	TreeCpView::setModel(model);
	this->peTreeModel = model;

	if (model != NULL) {
		connect(model, SIGNAL(modelUpdated()), this, SLOT(onModelUpdated()));
	}
	this->reset();
}

void PEStructureView::selectionChanged(const QItemSelection &newSel, const QItemSelection &prevSel)
{
	QModelIndexList indexes = newSel.indexes();
	if (indexes.size() == 0) {
		return;
	}
	QModelIndex index = indexes[0];
	if (!index.isValid()) {
		emit handlerSelected(NULL);
		//----
		return;
	}

	PEFileTreeItem* item = static_cast<PEFileTreeItem*> (index.internalPointer());
	if (!item) {
		emit handlerSelected(NULL);
		return;
	}
	PeHandler* hndl = item->getPeHandler();
	if (hndl) {
		const offset_t offset = item->getContentOffset();
		const bufsize_t size  = item->getContentSize();
		hndl->setDisplayed(false, offset, size);
		hndl->setHilighted(offset, size);
	}
	emit handlerSelected(hndl);
}

bool PEStructureView::selectHandler(PeHandler *hndl)
{
	emit handlerSelected(hndl);
	return true;
}


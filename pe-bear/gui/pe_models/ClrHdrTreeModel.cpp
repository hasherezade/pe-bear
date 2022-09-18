#include "ClrHdrTreeModel.h"

enum COLS {
	COL_OFFSET = 0,
	COL_NAME,
	COL_VALUE,
	COL_MEANING,
	MAX_COL
};

ClrHdrTreeItem::ClrHdrTreeItem(PeHandler *peHndl, level_t level, ClrDirWrapper::FieldID role, ClrHdrTreeItem *parent)
	: PeTreeItem(peHndl, role, FIELD_NONE, parent), 
	clrDir(peHndl->clrDirWrapper)
{
	parentItem = parent;
	this->level = level;
	this->role = role;
	if (this->m_PE == NULL) {
		this->level = DESC;
		this->role = ClrDirWrapper::NONE;
		return;
	}
}

int ClrHdrTreeItem::columnCount() const
{
	return MAX_COL;
}

QVariant ClrHdrTreeItem::background(int column) const
{
	if (m_PE == NULL) return QVariant();
	if (level == DETAILS && column >= COL_VALUE && column < MAX_COL) {
		QColor flagsColorT = addrColors.flagsColor();
		flagsColorT.setAlpha(addrColors.flagsAlpha());
		return flagsColorT;
	}
	return QVariant();
}

QVariant ClrHdrTreeItem::foreground(int column) const
{
	if (column == COL_OFFSET) return this->offsetFontColor;
	return QVariant();
}

Qt::ItemFlags ClrHdrTreeItem::flags(int column) const
{
	Qt::ItemFlags fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if (column != COL_VALUE || level == DETAILS) return fl;
	return fl| Qt::ItemIsEditable;
}

QVariant ClrHdrTreeItem::data(int column) const
{
	if (m_PE == NULL) return QVariant();

	if (this->level != DESC) return QVariant();
	switch (column) {
		case COL_OFFSET: return QString::number(getContentOffset(),16).toUpper();
		case COL_NAME: return clrDir.getFieldName(role);
		case COL_VALUE:
		{
			bool isOk = false;
			uint64_t val = clrDir.getNumValue(role, &isOk);
			if (!isOk) return QVariant();
			return QString::number(val, 16).toUpper();
		}
	}
	return QVariant();
}

bool ClrHdrTreeItem::setDataValue(int column, const QVariant &value)
{
	if (column != COL_VALUE) return false;

	QString text = value.toString();
	bool isOk = false;
	ULONGLONG number = text.toLongLong(&isOk, 16);
	if (!isOk) return false;

	isOk = clrDir.setNumValue(role, number);
	return isOk;
}

offset_t ClrHdrTreeItem::getContentOffset() const
{
	if (!m_PE) return 0;
	return clrDir.getFieldOffset(role);
}

bufsize_t ClrHdrTreeItem::getContentSize() const
{
	if (!m_PE) return 0;
	return clrDir.getFieldSize(role);
}

//-------------------

ClrFlagsTreeItem::ClrFlagsTreeItem(PeHandler* peHndl, level_t level, DWORD _flags, ClrHdrTreeItem *parent)
    : ClrHdrTreeItem(peHndl, level, role = ClrDirWrapper::FLAGS, parent),
	flags(_flags)
{
	loadChildren();
}

DWORD ClrFlagsTreeItem::getAllFlags() const
{
	bool isOk = false;
	DWORD allFlags = this->clrDir.getNumValue(role, &isOk);
	if (!isOk) return 0;
	return allFlags;
}

void ClrFlagsTreeItem::loadChildren()
{
	if (this->level != DESC) {
		return;
	}
	removeAllChildren();
	//create subrecords
	DWORD allFlags = getAllFlags();
	std::set<DWORD> flagsSet = ClrDirWrapper::getFlagsSet(allFlags);
	std::set<DWORD>::iterator chIter;
	for (chIter = flagsSet.begin(); chIter != flagsSet.end(); chIter++) {
		DWORD splitedFlag = *chIter;
		this->appendChild(new ClrFlagsTreeItem(myPeHndl, DETAILS, splitedFlag, this));
	}
}

QVariant ClrFlagsTreeItem::data(int column) const
{
	if (m_PE == NULL) return QVariant();

	if (this->level == DESC) {
		DWORD flags = getAllFlags();
		switch (column) {
			case COL_OFFSET : return QString::number(getContentOffset(),16).toUpper();
			case COL_NAME: return ("Flags");
			case COL_VALUE: return QString::number(flags, 16).toUpper();
		}
		return QVariant();
	}
	//---
	switch (column) {
		case COL_VALUE: return QString::number(this->flags, 16).toUpper();
		case COL_MEANING: return ClrDirWrapper::translateFlag(this->flags);
	}
	return QVariant();
}
//-------------------

ClrTreeModel::ClrTreeModel(PeHandler *peHndl, QObject *parent) 
	: PeWrapperModel(peHndl, parent), flagsItem(nullptr)
{
	if (!m_PE) return;

	rootItem = new ClrHdrTreeItem(peHndl);
	const size_t fieldNum = ClrDirWrapper::FIELD_COUNTER;

	for (size_t i = 0; i < fieldNum; ++i) {
		const ClrDirWrapper::FieldID role = ClrDirWrapper::FieldID(i);

		if (role == ClrDirWrapper::FLAGS) {
			flagsItem = new ClrFlagsTreeItem(peHndl, ClrHdrTreeItem::DESC);
			rootItem->appendChild(flagsItem);
		} else {
			rootItem->appendChild(new ClrHdrTreeItem(myPeHndl, ClrHdrTreeItem::DESC, role));
		}
	}
	connect(peHndl, SIGNAL(modified()), this, SLOT(reload()));
}

void ClrTreeModel::reload()
{
	flagsItem->loadChildren();
	reset();
	emit modelUpdated();
}

int ClrTreeModel::columnCount(const QModelIndex &parent) const { return MAX_COL; }

bool ClrTreeModel::containsValue(QModelIndex index) const
{
	return (index.column()  == COL_VALUE);
}

QVariant ClrTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) return QVariant();
	if (role == Qt::ForegroundRole) return this->addrColor(index);
	if (role == Qt::ToolTipRole) return this->toolTip(index);

	ClrHdrTreeItem *item = static_cast<ClrHdrTreeItem*>(index.internalPointer());
	if (item == NULL) return QVariant();

	if (role == Qt::BackgroundColorRole)
		return item->background(index.column());

	if (role == Qt::SizeHintRole) {
		return QVariant(); //get default
	}

	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		return item->data(index.column());
	}
	return QVariant();
}

bool ClrTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	ClrHdrTreeItem *item = static_cast<ClrHdrTreeItem*>(index.internalPointer());
	if (!item) return false;

	return item->setData(index.column(), value, item->role);
}

QVariant ClrTreeModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
{
	if(role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case COL_OFFSET : return "Offset";
		case COL_NAME : return "Name";
		case COL_VALUE : return "Value";
		case COL_MEANING : return "Meaning";
	}
	return QVariant();
}

Qt::ItemFlags ClrTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	ClrHdrTreeItem *item = static_cast<ClrHdrTreeItem*>(index.internalPointer());
	if (item == NULL) return 0;
	return item->flags(index.column());
}

QString ClrTreeModel::makeDockerTitle(uint32_t upId)
{
	ExeNodeWrapper* node = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (node == NULL) {
		return "";
	}
	ExeNodeWrapper *childEntry = node->getEntryAt(0);
	if (childEntry == NULL) {
		return "";
	}
	return childEntry->getName();
}

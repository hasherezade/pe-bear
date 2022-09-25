#include "FileHdrTreeModel.h"

#include "../../DateDisplay.h"

//#define MAX_COL 3
///------------------------
enum COLS {
	COL_OFFSET = 0,
	COL_NAME,
	COL_VALUE,
	COL_MEANING,
	MAX_COL
};

FileHdrTreeItem::FileHdrTreeItem(PeHandler *peHndl, level_t level, FileHdrWrapper::FieldID role, FileHdrTreeItem *parent)
	:  PeTreeItem(peHndl, role, FIELD_NONE, parent),
	fileHdr(peHndl->fileHdrWrapper)
{
	parentItem = parent;
	this->level = level;
	this->role = role;
	if (this->m_PE == NULL) {
		this->level = DESC;
		this->role = FileHdrWrapper::NONE;
		return;
	}
}

int FileHdrTreeItem::columnCount() const
{
	return MAX_COL;
}

QVariant FileHdrTreeItem::background(int column) const
{
	if (m_PE == NULL) return QVariant();
	if (level == DETAILS && column >= COL_VALUE && column < MAX_COL) {
		QColor flagsColorT = addrColors.flagsColor();
		flagsColorT.setAlpha(addrColors.flagsAlpha());
		return flagsColorT;
	}
	return QVariant();
}

QVariant FileHdrTreeItem::foreground(int column) const
{
	if (column == COL_OFFSET) return this->offsetFontColor;
	return QVariant();
}

Qt::ItemFlags FileHdrTreeItem::flags(int column) const
{
	Qt::ItemFlags fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if (column != COL_VALUE || level == DETAILS) return fl;
	return fl| Qt::ItemIsEditable;
}

QVariant FileHdrTreeItem::data(int column) const
{
	if (m_PE == NULL) return QVariant();

	if (this->level != DESC) return QVariant();
	switch (column) {
		case COL_OFFSET: return QString::number(getContentOffset(),16).toUpper();
		case COL_NAME: return fileHdr.getFieldName(role);
		case COL_VALUE:
		{
			bool isOk = false;
			uint64_t val = fileHdr.getNumValue(role, &isOk);
			if (!isOk) return QVariant();
			return QString::number(val, 16);
		}
		case COL_MEANING:
		{
			bool isOk = false;
			uint64_t val = fileHdr.getNumValue(role, &isOk);
			if (!isOk) return QVariant();
			if (role == FileHdrWrapper::TIMESTAMP) {
				return getDateString(val);
			}
			if (role == FileHdrWrapper::MACHINE) {
				return FileHdrWrapper::translateMachine(val);
			}
			return QString::number(val);
		}
	}
	return QVariant();
}

bool FileHdrTreeItem::setDataValue(int column, const QVariant &value)
{
	if (column != COL_VALUE) return false;

	QString text = value.toString();
	bool isOk = false;
	ULONGLONG number = text.toLongLong(&isOk, 16);
	if (!isOk) return false;

	isOk = fileHdr.setNumValue(role, number);
	return isOk;
}

offset_t FileHdrTreeItem::getContentOffset() const
{
	if (!m_PE) return 0;
	return fileHdr.getFieldOffset(role);
}

bufsize_t FileHdrTreeItem::getContentSize() const
{
	if (!m_PE) return 0;
	return fileHdr.getFieldSize(role);
}

//-------------------
FileHdrCharactTreeItem::FileHdrCharactTreeItem(PeHandler* peHndl, level_t level, DWORD characteristics, FileHdrTreeItem *parent)
	: FileHdrTreeItem(peHndl, level, role = FileHdrWrapper::CHARACT, parent)
{
	if (!myPeHndl) return;

	this->characteristics = characteristics;
	if (this->level == DESC) {
		bool isOk = false;
		this->characteristics = myPeHndl->fileHdrWrapper.getNumValue(FileHdrWrapper::CHARACT, &isOk);
		if (!isOk) {
			return;
		}
		loadChildren();
	}
}

void FileHdrCharactTreeItem::loadChildren()
{
	//create subrecords
	auto charactSet = FileHdrWrapper::splitCharact(this->characteristics);
	for (auto chIter = charactSet.begin(); chIter != charactSet.end(); ++chIter) {
		DWORD splitedCharacter = *chIter;
		this->appendChild(new FileHdrCharactTreeItem(myPeHndl, DETAILS, splitedCharacter, this));
	}
	return;
}

QVariant FileHdrCharactTreeItem::data(int column) const
{
	if (this->level == DESC) {
		switch (column) {
			case COL_OFFSET : return QString::number(getContentOffset(),16).toUpper();
			case COL_NAME: return ("Characteristics");
			case COL_VALUE: return QString::number(this->characteristics, 16).toUpper();
		}
		return QVariant();
	}
	//---
	switch (column) {
		case COL_VALUE: return QString::number(this->characteristics, 16).toUpper();
		case COL_MEANING: 
			return FileHdrWrapper::translateCharacteristics(this->characteristics);
	}
	return QVariant();
}

//-----------------------------------------------------------------------------
FileHdrTreeModel::FileHdrTreeModel(PeHandler* peHndl, QObject *parent)
	: PeWrapperModel(peHndl, parent)
{
	if (!m_PE) return;

	rootItem = new FileHdrTreeItem(peHndl);
	size_t fieldNum = FileHdrWrapper::FIELD_COUNTER;

	for (size_t i = 0; i < fieldNum; i++) {
		FileHdrWrapper::FieldID role = FileHdrWrapper::FieldID(i);
		if (role == FileHdrWrapper::CHARACT) {
			charactItem = new FileHdrCharactTreeItem(peHndl, FileHdrTreeItem::DESC);
			rootItem->appendChild(charactItem);
		} else
			rootItem->appendChild(new FileHdrTreeItem(peHndl, FileHdrTreeItem::DESC, role));
	}
	connect(peHndl, SIGNAL(modified()), this, SLOT(reload()));
}

void FileHdrTreeModel::reload()
{
	this->rootItem->detachChild(charactItem);
	delete charactItem;
	charactItem = NULL;

	charactItem = new FileHdrCharactTreeItem(myPeHndl, FileHdrTreeItem::DESC);
	rootItem->appendChild(charactItem);
	reset();
	emit modelUpdated();
}


bool FileHdrTreeModel::containsValue(QModelIndex index) const
{
	return (index.column() == COL_VALUE);
}

QVariant FileHdrTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) return QVariant();
	if (role == Qt::ForegroundRole) return this->addrColor(index);
	if (role == Qt::ToolTipRole) return this->toolTip(index);

	FileHdrTreeItem *item = static_cast<FileHdrTreeItem*>(index.internalPointer());
	if (item == NULL) return QVariant();

	if (role == Qt::BackgroundColorRole)
		return item->background(index.column());

	if (role == Qt::SizeHintRole) {
		return QVariant(); //get default
	}

	if (role == Qt::DisplayRole || role == Qt::EditRole)
		return item->data(index.column());

	return QVariant();
}

bool FileHdrTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid())
		return false;

	FileHdrTreeItem *item = static_cast<FileHdrTreeItem*>(index.internalPointer());
	if (!item) return false;

	return item->setData(index.column(), value, item->role);
}


QVariant FileHdrTreeModel::headerData(int section, Qt::Orientation /* orientation */, int role) const
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

Qt::ItemFlags FileHdrTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	FileHdrTreeItem *item = static_cast<FileHdrTreeItem*>(index.internalPointer());
	if (item == NULL) return Qt::NoItemFlags;
	return item->flags(index.column());
}

offset_t FileHdrTreeModel::getContentOffset() const
{
	if (!m_PE) return 0;
	return m_PE->peNtHdrOffset();
}

bufsize_t FileHdrTreeModel::getContentSize() const
{
	if (!m_PE) return 0;
	return m_PE->peNtHeadersSize();
}

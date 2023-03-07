#include "DebugTreeModel.h"
#include "../../DateDisplay.h"

//-----------------------------------------------------------------------------

#define NOT_FILLED  "-"

int DebugTreeModel::columnCount(const QModelIndex &parent) const
{
	DebugDirWrapper* dbgWrap = dynamic_cast<DebugDirWrapper*>(wrapper());
	if (!dbgWrap) return 0;
	ExeNodeWrapper* entry = dbgWrap->getEntryAt(0);
	if (!entry) return 0;
	uint32_t cntr = entry->getFieldsCount() + ADDED_COLS_NUM;
	return cntr;
}

QVariant DebugTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case OFFSET: return "Offset";
		case NAME: return "TypeName";
	}
	DebugDirWrapper* dbgWrap = dynamic_cast<DebugDirWrapper*>(wrapper());
	if (!dbgWrap) return QVariant();

	int32_t fID = this->columnToFID(section);
	return dbgWrap->getFieldName(0, fID);
}

ExeElementWrapper* DebugTreeModel::wrapperAt(QModelIndex index) const
{
	DebugDirWrapper* dbgWrap = dynamic_cast<DebugDirWrapper*>(wrapper());
	if (!dbgWrap) return NULL;
	return dbgWrap->getEntryAt(index.row());
}

QVariant DebugTreeModel::data(const QModelIndex &index, int role) const
{
	DebugDirEntryWrapper* entry =  dynamic_cast<DebugDirEntryWrapper*>(wrapperAt(index));
	if (!entry) return QVariant();

	int column = index.column();
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	if (column != NAME && role == Qt::FontRole) return offsetFont;
	if (role == Qt::ToolTipRole) return toolTip(index);

	//if (role == Qt::BackgroundRole && !entry->isValid()) return errColor;
	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
		case NAME: return entry->getName();
	}
	bool isOk = false;
	uint64_t val = entry->getNumValue(getFID(index), &isOk);
	if (!isOk) return "UNK"; /* other type? */

	return QString::number(val, 16);
}

QVariant DebugTreeModel::toolTip(const QModelIndex &index) const
{
	DebugDirEntryWrapper* entry =  dynamic_cast<DebugDirEntryWrapper*>(wrapperAt(index));
	if (!entry) return QVariant();
	
	const int fieldID = getFID(index);
	QString translated = entry->translateFieldContent(fieldID);
	if (translated.length()) {
		return translated;
	}
	if (fieldID == DebugDirEntryWrapper::TIMESTAMP) {
		bool isOk = false;
		int val = entry->getNumValue(fieldID, &isOk);
		if (!isOk) return QVariant();
		return getDateString(val);
	}
	return QVariant();
}

bool DebugTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;

	size_t fId = index.row();
	uint32_t sID = this->columnToFID(index.column());

	if (!wrapper()) return false;
	
	DebugDirEntryWrapper* entry =  dynamic_cast<DebugDirEntryWrapper*>(wrapperAt(index));
	if (!entry) return false;

	QString text = value.toString();

	bool isModified = false;
	offset_t offset = 0;
	bufsize_t fieldSize = 0;

	bool isOk = false;
	ULONGLONG number = text.toLongLong(&isOk, 16);
	if (!isOk) return false;

	offset = entry->getFieldOffset(sID);
	fieldSize = entry->getFieldSize(sID);

	this->myPeHndl->backupModification(offset, fieldSize);
	isModified = entry->setNumValue(sID, index.column(), number);

	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}

Qt::ItemFlags DebugTreeModel::flags(const QModelIndex &index) const
{ 
	if (!index.isValid())
		return Qt::NoItemFlags;
	int column = index.column();
	if (column >= ADDED_COLS_NUM) return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

//----------------------------------------------------------------------------

int DebugRDSIEntryTreeModel::rowCount(const QModelIndex &parent) const
{
	if (!wrapper()) return 0;
	return wrapper()->getFieldsCount();
}

ExeElementWrapper* DebugRDSIEntryTreeModel::wrapper() const
{
	if (!myPeHndl) return NULL;
	
	ExeNodeWrapper* parentW = myPeHndl->debugDirWrapper.getEntryAt(this->parentId);
	if (!parentW) return NULL;
	
	return parentW->getEntryAt(0);
}

ExeElementWrapper* DebugRDSIEntryTreeModel::wrapperAt(QModelIndex index) const
{
	return wrapper();
}

QVariant DebugRDSIEntryTreeModel::data(const QModelIndex &index, int role) const
{
	DebugDirCVEntryWrapper* wrap = dynamic_cast<DebugDirCVEntryWrapper*>(wrapperAt(index));
	if (!wrap) return QVariant();
	
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	if (role == Qt::ToolTipRole) return toolTip(index);

	int row = index.row();
	int column = index.column();
	int fId = this->getFID(index);
	
	if (role == Qt::FontRole && column == OFFSET) {
		return offsetFont;
	}
	if (role != Qt::DisplayRole && role != Qt::EditRole) {
		return QVariant();
	}
	switch (column) {
		case OFFSET: 
			return QString::number(getFieldOffset(index), 16);
		case NAME: {
			const QString name = wrap->getFieldName(fId);
			return name;
		}
		case VALUE: {
			if (fId == DebugDirCVEntryWrapper::F_CVDBG_GUID) {
				return wrap->getGuidString();
			}
			else if (fId == DebugDirCVEntryWrapper::F_CVDBG_SIGN){
				return wrap->getSignature();
			}
			else if (fId == DebugDirCVEntryWrapper::F_CVDBG_PDB) {
				//TODO: make some decent parsing here
				char *str_ptr = (char*)wrap->getFieldPtr(fId);
				if (!str_ptr) return QVariant();
				return QString(str_ptr);
			}
			bool isOk = false;
			uint64_t val = wrap->getNumValue(fId, &isOk);
			if (isOk) {
				return QString::number(val, 16);
			}
		}
	}
	return QVariant();
}

bool DebugRDSIEntryTreeModel::setTextData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;

	ExeElementWrapper *w = wrapperAt(index);
	if (!w) return false;
	
	size_t fId = this->getFID(index);
	
	size_t fieldSize = w->getFieldSize(fId);
	offset_t offset = w->getFieldOffset(fId);
	QString text = value.toString();
	
	char* textPtr = (char*) w->getFieldPtr(fId);
	if (!textPtr) return false;
	
	this->myPeHndl->backupModification(offset, fieldSize);
	bool isModified =  m_PE->setTextValue(textPtr, text.toStdString(), fieldSize);
	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}

bool DebugRDSIEntryTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	ExeElementWrapper *w = wrapperAt(index);
	DebugDirCVEntryWrapper* wrap = dynamic_cast<DebugDirCVEntryWrapper*>(w);
	if (!wrap) return false;
	
	int row = index.row();
	int column = index.column();
	int fId = this->getFID(index);
	
	if (fId == DebugDirCVEntryWrapper::F_CVDBG_GUID) {
		return false;
	}
	if (fId == DebugDirCVEntryWrapper::F_CVDBG_PDB) {
		return setTextData(index, value, role);
	}
	return WrapperTableModel::setData(index, value, role);
}

QVariant DebugRDSIEntryTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	switch (section) {
		case OFFSET: return "Offset";
		case NAME: return "Name";
		case VALUE: return "Value";
	}
	return QVariant();
}

Qt::ItemFlags DebugRDSIEntryTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid()) return Qt::NoItemFlags;

	int fId = this->getFID(index);
	if (fId == DebugDirCVEntryWrapper::F_CVDBG_GUID) {
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	}
	return WrapperTableModel::flags(index);
}

#include "ImportsTreeModel.h"

//-----------------------------------------------------------------------------

#define NOT_FILLED  "-"

int ImportsTreeModel::columnCount(const QModelIndex &parent) const
{
	ImportDirWrapper* impWrap = dynamic_cast<ImportDirWrapper*>(wrapper());
	if (!impWrap) return 0;
	ExeNodeWrapper* entry = impWrap->getEntryAt(0);
	if (!entry) return 0;
	uint32_t cntr = entry->getFieldsCount() + ADDED_COLS_NUM;
	return cntr;
}

QVariant ImportsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case OFFSET: return tr("Offset");
		case NAME: return tr("Name");
		case IS_BOUND : return tr("Bound?");
		case FUNC_NUM: return tr("Func. Count");
	}
	ImportDirWrapper* impWrap = dynamic_cast<ImportDirWrapper*>(wrapper());
	if (!impWrap) return QVariant();

	int32_t fID = this->columnToFID(section);
	return impWrap->getFieldName(0, fID);
}

ExeElementWrapper* ImportsTreeModel::wrapperAt(QModelIndex index) const
{
	ImportDirWrapper* impWrap = dynamic_cast<ImportDirWrapper*>(wrapper());
	if (!impWrap) return NULL;
	return impWrap->getEntryAt(index.row());
}

QVariant ImportsTreeModel::data(const QModelIndex &index, int role) const
{
	ImportDirWrapper* impWrap = dynamic_cast<ImportDirWrapper*>(wrapper());
	if (!impWrap) return QVariant();

	int column = index.column();
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	if (column != NAME && role == Qt::FontRole) return offsetFont;
	if (role == Qt::ToolTipRole) return toolTip(index);

	ImportEntryWrapper* entry =  dynamic_cast<ImportEntryWrapper*>(wrapperAt(index));
	if (!entry) return QVariant();

	if (role == Qt::BackgroundRole && !entry->isValid()) return errColor;
	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
		case NAME: return entry->getName();
		case FUNC_NUM: return QString::number(entry->getEntriesCount());
		case IS_BOUND: return entry->isBound();
	}
	bool isOk = false;
	uint64_t val = entry->getNumValue(getFID(index), &isOk);
	if (!isOk) return "UNK"; /* other type? */

	return QString::number(val, 16);
}

bool ImportsTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;

	size_t fId = index.row();
	uint32_t sID = this->columnToFID(index.column());

	if (!wrapper()) return false;
	ImportEntryWrapper* entry =  dynamic_cast<ImportEntryWrapper*>(wrapperAt(index));
	if (!entry) return false;

	QString text = value.toString();

	bool isModified = false;
	offset_t offset = 0;
	bufsize_t fieldSize = 0;

	if (index.column() == NAME) {
		char* textPtr = entry->getLibraryName();
		if (!textPtr) return false;

		offset = entry->getOffset(textPtr);
		fieldSize = text.size() + 2;

		this->myPeHndl->backupModification(offset, fieldSize);
		isModified = m_PE->setTextValue(textPtr, text.toStdString(), fieldSize);

	} else {
		
		bool isOk = false;
		ULONGLONG number = text.toLongLong(&isOk, 16);
		if (!isOk) return false;

		offset = entry->getFieldOffset(sID);
		fieldSize = entry->getFieldSize(sID);

		this->myPeHndl->backupModification(offset, fieldSize);
		isModified = entry->setNumValue(sID, index.column(), number);
	}

	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}

Qt::ItemFlags ImportsTreeModel::flags(const QModelIndex &index) const
{ 
	if (!index.isValid())
		return Qt::NoItemFlags;
	int column = index.column();
	if (column == NAME || column >= ADDED_COLS_NUM) return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

//----------------------------------------------------------------------------
ImportedFuncModel::ImportedFuncModel(PeHandler *peHndl, QObject *parent)
	:  WrapperTableModel(peHndl, parent), 
	libraryId(0)
{
	connect(peHndl, SIGNAL(modified()), this, SLOT(onNeedReset()));
}

ImportEntryWrapper* ImportedFuncModel::wrapper() const
{
	if (!myPeHndl) return NULL;
	ImportDirWrapper& impWrap = myPeHndl->importDirWrapper;
	ImportEntryWrapper* entryWrapp = dynamic_cast<ImportEntryWrapper*>(impWrap.getEntryAt(this->libraryId));
	return entryWrapp;
}

ExeElementWrapper* ImportedFuncModel::wrapperAt(QModelIndex index) const
{
	ImportEntryWrapper* wrapper = this->wrapper();
	if (!wrapper) return NULL;
	int sId = this->getSID(index);
	return wrapper->getEntryAt(sId);
}

int ImportedFuncModel::rowCount(const QModelIndex &parent) const
{
	if (!wrapper()) return 0;
	uint32_t cntr = wrapper()->getEntriesCount();
	return cntr;
}

int ImportedFuncModel::columnCount(const QModelIndex &parent) const
{
	if (!wrapper()) return 0;
	uint32_t cntr = ImportedFuncWrapper::FIELD_COUNTER + ADDED_COLS_NUM;
	return cntr;
}

QVariant ImportedFuncModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	if (!wrapper()) return QVariant();
	size_t fId = columnToFID(section);
	ExeNodeWrapper* entry = wrapper()->getEntryAt(0);

	if (entry == NULL) return "";

	switch (section) {
		case NAME: return tr("Name");
		case ORDINAL : return tr("Ordinal");
		case CALL_VIA: return tr("Call via");
	}
	return entry->getFieldName(fId);
}

QVariant ImportedFuncModel::data(const QModelIndex &index, int role) const
{
	if (!wrapper()) return QVariant();
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	int row = index.row();
	int column = index.column();

	if (column != NAME && role == Qt::FontRole) return offsetFont;
	if (column == CALL_VIA && role == Qt::ForegroundRole) return offsetFontColor;

	if (role == Qt::ToolTipRole) return toolTip(index);
	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	ImportedFuncWrapper* entry =  dynamic_cast<ImportedFuncWrapper*>(wrapperAt(index));
	if (!entry) return QVariant();

	switch (column) {
		case NAME:
		{
			char *name = entry->getFunctionName();
			if (!name) return NOT_FILLED;
			return name;
		}
		case ORDINAL:
			if (!entry->isByOrdinal()) return NOT_FILLED;
			return QString::number(entry->getThunkValue(), 16);

		case CALL_VIA:
			uint64_t val = entry->getFieldRVA(ImportEntryWrapper::FIRST_THUNK);
			return QString::number(val, 16);
	}

	int32_t fID = columnToFID(column);
	if (!entry->getFieldPtr(fID)) return "-";
	 
	bool isOk = true;
	uint64_t val = entry->getNumValue(fID, &isOk);
	if (!isOk) return "UNK"; /* other type? */

	return QString::number(val, 16);
}

bool ImportedFuncModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;
	if (!wrapper()) return false;

	int row = index.row();
	int column = index.column();
	int32_t fID = columnToFID(column);

	ImportedFuncWrapper* entry =  dynamic_cast<ImportedFuncWrapper*>(wrapper()->getEntryAt(row));
	if (!entry)  return false;

	bool byOrd = entry->isByOrdinal();
	
	bool isModified = false;
	offset_t offset = INVALID_ADDR;
	bufsize_t fieldSize = 0;

	QString text = value.toString();
	 
	switch (column) {
		case NAME:
		{
			char* textPtr = entry->getFunctionName();
			if (!textPtr) return false;

			offset = entry->getOffset(textPtr, true);
			if (offset == INVALID_ADDR) return false;

			fieldSize = text.length() + 2;
			this->myPeHndl->backupModification(offset, fieldSize);
			isModified = m_PE->setTextValue(textPtr, text.toStdString(), fieldSize);
			break;
		}
		case ORDINAL:
			if (!entry->isByOrdinal()) return false;
			return false;

		default:
		{
			bool isOk = false;
			ULONGLONG number = text.toLongLong(&isOk, 16);
			if (!isOk) return false;

			offset = entry->getFieldOffset(fID);
			fieldSize = entry->getFieldSize(fID);

			this->myPeHndl->backupModification(offset, fieldSize);
			isModified = entry->setNumValue(fID, number);
		}
	}
	//---
	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}

Qt::ItemFlags ImportedFuncModel::flags(const QModelIndex &index) const
{ 
	if (!index.isValid())
		return Qt::NoItemFlags;

	QString myData = data(index, Qt::DisplayRole).toString();
	if (myData == NOT_FILLED) return Qt::NoItemFlags;

	static Qt::ItemFlags editable = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;

	int column = index.column();
	if (column >= ADDED_COLS_NUM) return editable;

	ImportedFuncWrapper* entry =  dynamic_cast<ImportedFuncWrapper*>(this->wrapperAt(index));
	if (!entry) return Qt::NoItemFlags;
	
	bool byOrd = entry->isByOrdinal();
	if (byOrd) {
		if (column == ORDINAL) return editable;
	} else {
		if (column == NAME) return editable;
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

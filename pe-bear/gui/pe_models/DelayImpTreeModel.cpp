#include "DelayImpTreeModel.h"

//-----------------------------------------------------------------------------

#define NOT_FILLED  "-"

int DelayImpTreeModel::columnCount(const QModelIndex &parent) const
{
	ExeNodeWrapper* impWrap = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (!impWrap) return 0;
	ExeNodeWrapper* entry = impWrap->getEntryAt(0);
	if (!entry) return 0;
	uint32_t cntr = entry->getFieldsCount() + ADDED_COLS_NUM;
	return cntr;
}


QVariant DelayImpTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case OFFSET: return tr("Offset");
		case NAME: return tr("Name");
	}
	ExeNodeWrapper* impWrap = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (!impWrap) return QVariant();

	int32_t fID = this->columnToFID(section);
	return impWrap->getSubfieldName(0, fID);
}

QVariant DelayImpTreeModel::data(const QModelIndex &index, int role) const
{
	DelayImpEntryWrapper* wrap = dynamic_cast<DelayImpEntryWrapper*>(wrapperAt(index));
	if (!wrap) return QVariant();

	int column = index.column();
	if (role == Qt::ForegroundRole) return this->addrColor(index);
	if (role == Qt::FontRole) {
		if (this->containsOffset(index) || this->containsValue(index)) return offsetFont;
	}
	if (role == Qt::ToolTipRole) return toolTip(index);

	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
	int fId = getFID(index);
	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
		case NAME: return wrap->getName();
	}
	return dataValue(index);
}

bool DelayImpTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;

	uint32_t sID = this->columnToFID(index.column());

	if (!wrapper()) return false;
	DelayImpEntryWrapper* entry =  dynamic_cast<DelayImpEntryWrapper*>(wrapperAt(index));
	if (!entry) return false;

	QString text = value.toString();

	bool isModified = false;
	uint32_t offset = 0;
	uint32_t fieldSize = 0;

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

//----------------------------------------------------------------------------

DelayImpFuncModel::DelayImpFuncModel(PeHandler *peHndl, QObject *parent)
	:  WrapperTableModel(peHndl, parent), 
	libraryId(0)
{
	connect(peHndl, SIGNAL(modified()), this, SLOT(onNeedReset()));
}

ExeNodeWrapper* DelayImpFuncModel::wrapper() const
{
	if (!myPeHndl) return NULL;
	DelayImpDirWrapper& impWrap = myPeHndl->delayImpDirWrapper;
	ExeNodeWrapper *entryWrapp = impWrap.getEntryAt(this->libraryId);
	return entryWrapp;
}

ExeElementWrapper* DelayImpFuncModel::wrapperAt(QModelIndex index) const
{
	ExeNodeWrapper* wrapper = this->wrapper();
	if (!wrapper) return NULL;
	int sId = this->getSID(index);
	return wrapper->getEntryAt(sId);
}

int DelayImpFuncModel::rowCount(const QModelIndex &parent) const
{
	if (!wrapper()) return 0;
	uint32_t cntr = wrapper()->getEntriesCount();
	return cntr;
}

int DelayImpFuncModel::columnCount(const QModelIndex &parent) const
{
	if (!wrapper()) return 0;
	uint32_t cntr = DelayImpFuncWrapper::FIELD_COUNTER + ADDED_COLS_NUM;
	return cntr;
}

QVariant DelayImpFuncModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	if (!wrapper()) return QVariant();
	size_t fId = columnToFID(section);
	ExeNodeWrapper* entry = wrapper()->getEntryAt(0);

	if (entry == NULL) return "";

	switch (section) {
		case OFFSET: return tr("Call via");
		case NAME: return tr("Name");
		case ORDINAL : return tr("Ordinal");
		case HINT: return tr("Hint");
	}
	return entry->getFieldName(fId);
}

Executable::addr_type DelayImpFuncModel::offsetAddrType() const
{
	DelayImpFuncWrapper* entry = dynamic_cast<DelayImpFuncWrapper*>(wrapperAt(QAbstractItemModel::createIndex(0,0)));
	if (!entry || !myPeHndl || !myPeHndl->getPe()) {
		return Executable::RVA;
	}
	return myPeHndl->getPe()->detectAddrType(entry->callVia(), Executable::RVA);
}

QVariant DelayImpFuncModel::data(const QModelIndex &index, int role) const
{
	if (!wrapper()) return QVariant();
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	int row = index.row();
	int column = index.column();

	if (column != NAME && role == Qt::FontRole) return offsetFont;

	if (role == Qt::ToolTipRole) return toolTip(index);
	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	DelayImpFuncWrapper* entry =  dynamic_cast<DelayImpFuncWrapper*>(wrapperAt(index));
	if (!entry) return QVariant();
	
	switch (column) {
		case OFFSET:
			return QString::number(entry->callVia(), 16);
		case NAME:
		{
			char *name = entry->getFunctionName();
			if (!name) return NOT_FILLED;
			return name;
		}
		case ORDINAL:
			if (entry->isByOrdinal() == false) return NOT_FILLED;
			return QString::number(entry->getOrdinal(), 16);
		case HINT:
			return  QString::number(entry->getHint(), 16);
	}

	int32_t fID = getFID(index);
	if (!entry->getFieldPtr(fID, 0)) return "-";
	 
	bool isOk = true;
	uint64_t val = entry->getNumValue(fID, &isOk);
	if (!isOk) return "UNK"; /* other type? */

	return QString::number(val, 16);
}

bool DelayImpFuncModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;
	if (!wrapper()) return false;

	int row = index.row();
	int column = index.column();
	int32_t fID = columnToFID(column);

	DelayImpFuncWrapper* entry = dynamic_cast<DelayImpFuncWrapper*>(wrapperAt(index));
	if (!entry)  return false;

	bool byOrd = entry->isByOrdinal();
	
	bool isModified = false;
	offset_t offset = 0;
	uint32_t fieldSize = 0;

	QString text = value.toString();
	 
	switch (column) {
		case NAME:
		{
			char* textPtr = entry->getFunctionName();
			if (!textPtr) return false;

			offset = entry->getOffset(textPtr);
			fieldSize = text.size() + 2;
			this->myPeHndl->backupModification(offset, fieldSize);
			isModified = m_PE->setTextValue(textPtr, text.toStdString(), fieldSize);
			break;
		}
		case ORDINAL:
			if (!entry->isByOrdinal()) return false;
			return false;
			//break;
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

Qt::ItemFlags DelayImpFuncModel::flags(const QModelIndex &index) const
{ 
	if (!index.isValid())
		return Qt::NoItemFlags;

	QString myData = data(index, Qt::DisplayRole).toString();
	if (myData == NOT_FILLED) return Qt::NoItemFlags;

	static Qt::ItemFlags editable = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;

	int column = index.column();
	if (column >= ADDED_COLS_NUM) return editable;
	
	DelayImpFuncWrapper* entry = dynamic_cast<DelayImpFuncWrapper*>(wrapperAt(index));
	if (!entry) return Qt::NoItemFlags;
	
	const bool byOrd = entry->isByOrdinal();
	if (byOrd) {
		if (column == ORDINAL) return editable;
	} else {
		if (column == NAME) return editable;
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

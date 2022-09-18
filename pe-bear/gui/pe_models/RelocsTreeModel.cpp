#include "RelocsTreeModel.h"

//-----------------------------------------------------------------------------

#define NOT_FILLED  "-"

int RelocsTreeModel::columnCount(const QModelIndex &parent) const
{
	RelocDirWrapper* impWrap = dynamic_cast<RelocDirWrapper*>(wrapper());
	if (!impWrap) return 0;
	ExeNodeWrapper* entry = impWrap->getEntryAt(0);
	if (!entry) return 0;

	uint32_t cntr = entry->getFieldsCount();
	return cntr + ADDED_COLS_NUM;
}

QVariant RelocsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case OFFSET: return "Offset";
	}
	RelocDirWrapper* impWrap = dynamic_cast<RelocDirWrapper*>(wrapper());
	if (!impWrap) return QVariant();
	int32_t fID = this->columnToFID(section);
	if (fID == RelocBlockWrapper::ENTRIES_PTR) {
		return "Entries Count";
	}
	return impWrap->getFieldName(0, fID);
}

ExeElementWrapper* RelocsTreeModel::wrapperAt(QModelIndex index) const
{
	RelocDirWrapper* wrap = dynamic_cast<RelocDirWrapper*>(wrapper());
	if (!wrap) return NULL;
	return wrap->getEntryAt(index.row()); //RelocBlockWrapper
}

QVariant RelocsTreeModel::data(const QModelIndex &index, int role) const
{
	RelocDirWrapper* impWrap = dynamic_cast<RelocDirWrapper*>(wrapper());
	if (!impWrap) return QVariant();

	int column = index.column();
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	if (role == Qt::FontRole) return offsetFont;
	if (role == Qt::ToolTipRole) return toolTip(index);

	RelocBlockWrapper* block =  dynamic_cast<RelocBlockWrapper*>(wrapperAt(index));
	if (!block) return QVariant();

	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
	}
	int fID = getFID(index);
	if (fID == RelocBlockWrapper::ENTRIES_PTR) {
		return QString::number(block->getEntriesNum(), 16); 
	}
	bool isOk = false;
	uint64_t val = block->getNumValue(fID, &isOk);
	if (!isOk) return "UNK"; /* other type? */

	return QString::number(val, 16);
}

bool RelocsTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;

	size_t fID = getFID(index);

	if (!wrapper()) return false;
	RelocBlockWrapper* entry =  dynamic_cast<RelocBlockWrapper*>(wrapperAt(index));
	if (!entry) return false;

	QString text = value.toString();

	bool isModified = false;
	offset_t offset = 0;
	bufsize_t fieldSize = 0;

	bool isOk = false;
	ULONGLONG number = text.toLongLong(&isOk, 16);
	if (!isOk) return false;

	offset = entry->getFieldOffset(fID);
	fieldSize = entry->getFieldSize(fID);

	this->myPeHndl->backupModification(offset, fieldSize);
	isModified = entry->setNumValue(fID, index.column(), number);

	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}
//----------------------------------------------------------------------------
int RelocEntriesModel::rowCount(const QModelIndex &parent) const
{
	if (!wrapper()) return 0;
	RelocBlockWrapper *w = dynamic_cast<RelocBlockWrapper*>(wrapper());
	if (w == NULL) return 0;

	return w->getEntriesNum();
}

QVariant RelocEntriesModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::ForegroundRole) return this->addrColor(index);
	if (role == Qt::ToolTipRole) return toolTip(index);

	int column = index.column();

	if (role == Qt::FontRole) {
		if (column != TYPE) return offsetFont;
		return QVariant();
	}
	
	int fId = this->getFID(index);

	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
	RelocEntryWrapper* w = dynamic_cast<RelocEntryWrapper*>(wrapperAt(index));
	if (!w) return QVariant(); // NO WRAPPER!

	bool isOk = false;
	uint64_t val = w->getNumValue(RelocEntryWrapper::RELOC_ENTRY_VAL, &isOk);
	if (!isOk) return "UNK"; // NO VALUE

	int type = RelocEntryWrapper::getType(val);

	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
		case TYPE: return RelocEntryWrapper::translateType(type);
		case DELTA: return QString::number(RelocEntryWrapper::getDelta(val), 16);
		case RELOC_RVA: 
		{
			if (type == 0) return QVariant(); //Skip padding
			int delta = RelocEntryWrapper::getDelta(val);
			return QString::number(w->deltaToRVA(delta), 16);
		}
	}
	return QString::number(val, 16);
}

QVariant RelocEntriesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case OFFSET : return "Offset";
		case VALUE : return "Value";
		case TYPE: return "Type";
		case DELTA: return "Offset from Page";
		case RELOC_RVA: return "Reloc RVA";
	}
	return QVariant();
}

ExeElementWrapper* RelocEntriesModel::wrapperAt(QModelIndex index) const
{
	RelocBlockWrapper *topWrapper = dynamic_cast<RelocBlockWrapper*>(this->wrapper());
	if (topWrapper == NULL) return NULL;

	return topWrapper->getEntryAt(this->getFID(index));
}

Executable::addr_type RelocEntriesModel::addrTypeAt(QModelIndex index) const
{
	if (index.column() == RELOC_RVA) return Executable::RVA;
	return WrapperTableModel::addrTypeAt(index);
}

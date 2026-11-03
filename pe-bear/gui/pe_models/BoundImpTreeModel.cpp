#include "BoundImpTreeModel.h"
#include "../../base/InPlaceText.h"
#include "../../DateDisplay.h"

//-----------------------------------------------------------------------------

#define NOT_FILLED  "-"

int BoundImpTreeModel::columnCount(const QModelIndex &parent) const
{
	ExeNodeWrapper* impWrap = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (!impWrap) return 0;
	ExeNodeWrapper* entry = impWrap->getEntryAt(0);
	if (!entry) return 0;
	const int cntr = static_cast<int>(entry->getFieldsCount() + static_cast<int>(ADDED_COLS_NUM));
	return cntr;
}


QVariant BoundImpTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant BoundImpTreeModel::data(const QModelIndex &index, int role) const
{
	BoundEntryWrapper* wrap = dynamic_cast<BoundEntryWrapper*>(wrapperAt(index));
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

QVariant BoundImpTreeModel::toolTip(const QModelIndex &index) const
{
    BoundEntryWrapper* entry =  dynamic_cast<BoundEntryWrapper*>(wrapperAt(index));
	if (!entry) return QVariant();
	
	const int fieldID = getFID(index);
	if (fieldID == BoundEntryWrapper::TIMESTAMP) {
		bool isOk = false;
		int val = entry->getNumValue(fieldID, &isOk);
		if (!isOk) return QVariant();
		return getDateString(val);
	}
	return QVariant();   
}

bool BoundImpTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) return false;

	BoundEntryWrapper* entry =  dynamic_cast<BoundEntryWrapper*>(wrapperAt(index));
	if (!entry) return false;

	uint32_t fieldID = this->columnToFID(index.column());
    
	QString text = value.toString();

	bool isModified = false;
	offset_t offset = 0;
	bufsize_t fieldSize = 0;

	if (index.column() == NAME) {
		char* textPtr = entry->getLibraryName();
		if (!textPtr) return false;

		offset = entry->getOffset(textPtr);
		fieldSize = in_place_text::capacity(m_PE, textPtr);
		/* A longer name would overwrite what follows it in the image. */
		if (!in_place_text::fits(text, fieldSize)) return false;
		this->myPeHndl->backupModification(offset, fieldSize);
		isModified = in_place_text::write(textPtr, fieldSize, text);

	} else {

		bool isOk = false;
		ULONGLONG number = text.toLongLong(&isOk, 16);
		if (!isOk) return false;

		offset = entry->getFieldOffset(fieldID);
		fieldSize = entry->getFieldSize(fieldID);

		this->myPeHndl->backupModification(offset, fieldSize);
		isModified = entry->setNumValue(fieldID, index.column(), number);
	}

	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}
//----------------------------------------------------------------------------

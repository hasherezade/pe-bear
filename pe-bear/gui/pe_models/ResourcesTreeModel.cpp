#include "ResourcesTreeModel.h"
#include "../../DateDisplay.h"

//-----------------------------------------------------------------------------

#define NOT_FILLED  "-"

Executable::addr_type ResourcesTreeModel::addrTypeAt(QModelIndex index) const
{
	int col = index.column();
	ExeElementWrapper *exeW = wrapper();

	if (exeW 
		&& index.row() >= getWrapperFieldsCount()
		&& col  >= MEANING && col  <= MEANING2) 
	{
		return Executable::RAW;
	}
	return  WrapperInterface::addrTypeAt(index);
}

int ResourcesTreeModel::rowCount(const QModelIndex &parent) const
{
	ExeNodeWrapper *w = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (w == NULL) return 0;

	size_t num = getWrapperFieldsCount() + w->getEntriesCount();
	return num;
}

QVariant ResourcesTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	switch (section) {
		case OFFSET: return "Offset";
		case NAME: return "Name";
		case VALUE : return "Value";
		case VALUE2: return "Value";
		case MEANING: return "Meaning";
		case MEANING2: return "Meaning";
		case RES_NAME: return "Type";
		case ENTRIES_NUM: return "Entries Count";
	}
	return QVariant();
}

ExeElementWrapper* ResourcesTreeModel::wrapperAt(QModelIndex index) const
{
	int row = index.row();
	ExeElementWrapper *exeW = wrapper();
	if (!exeW) return NULL;

	size_t fieldsCount = getWrapperFieldsCount();
	if (row < fieldsCount) {
		return exeW;
	}
	ExeNodeWrapper *wrap = dynamic_cast<ExeNodeWrapper*>(exeW);
	int entryId = row - fieldsCount;
	return wrap->getEntryAt(entryId);
}

int ResourcesTreeModel::getFID(const QModelIndex &index) const
{
	int row = index.row();
	ExeNodeWrapper *exeW = dynamic_cast<ExeNodeWrapper*>(wrapperAt(index));
	if (exeW == NULL ||exeW == wrapper()) return row; // main or empty wrapper

	return index.column() - VALUE;
}

QVariant ResourcesTreeModel::subdirsData(const QModelIndex &index, int role) const
{

	ResourceEntryWrapper *subW = dynamic_cast<ResourceEntryWrapper*> (wrapperAt(index));
	if (subW == NULL) return QVariant();

	const int row = index.row();
	const int column = index.column();
	const int fId = getFID(index);

	switch (column) {
		case OFFSET:
		{
			return QString::number(getFieldOffset(index), 16);
		}
		case NAME: 
		{
			QString name = subW->getFieldName(ResourceEntryWrapper::NAME_ID_ADDR);
			name += "_" + QString::number(subW->getEntryId());
			return name;
		}
		case VALUE: 
		case VALUE2: 
		{
			bool isOk = false;
			uint64_t val = subW->getNumValue(fId, &isOk);
			if (isOk) {
				return QString::number(val, 16);
			}
			return QVariant();
		}
		case MEANING: 
		{
			uint64_t val = subW->getNameOffset();
			if (val != INVALID_ADDR) {
				return QString::number(val, 16);
			}
			return QVariant();
		}
		case MEANING2:
		{
			uint64_t val = subW->getChildAddress();
			if (val != INVALID_ADDR) return QString::number(val, 16);
			return "";
		}
		case RES_NAME:
		{
			QString name;
			IMAGE_RESOURCE_DIRECTORY_STRING* ptr = subW->getNameStr();
				if (ptr == NULL) {
				bool isOk = false;
				uint64_t val = subW->getNumValue(ResourceEntryWrapper::NAME_ID_ADDR, &isOk);
				return subW->translateType(val);
			}
			const ushort *str = (ushort*)ptr->NameString;
			name = QString::fromUtf16(str, ptr->Length);
			return name;
		}
		case ENTRIES_NUM: 
		{
			ResourcesAlbum *album = subW->getAlbumPtr();
			if (album == NULL) return QVariant();
			long topId = subW->getTopEntryID();
			int entries = album->entriesCountAt(topId);
			return QString::number(entries);
		}
	}
	return QVariant();
}

QVariant ResourcesTreeModel::data(const QModelIndex &index, int role) const
{
	ExeNodeWrapper *wrap = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (!wrap) return QVariant();

	const int row = index.row();
	const int column = index.column();
	const int fId = getFID(index);
	
	if (role == Qt::ForegroundRole) return this->addrColor(index);
	if (role == Qt::FontRole) {
		if (this->containsOffset(index) || this->containsValue(index)) return offsetFont;
	}
	if (column == MARKERS 
		&& fId >= this->getWrapperFieldsCount())
	{
		if (role == Qt::DecorationRole) return ViewSettings::getScaledPixmap(":/icons/List.ico");
		if (role == Qt::ToolTipRole) return "List";
	}
	
	if (role == Qt::ToolTipRole) return toolTip(index);

	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
	if (row >= getWrapperFieldsCount()) {
		return subdirsData(index, role);
	}

	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
		case NAME: return wrap->getFieldName(fId);
		case VALUE: return dataValue(index);
		case MEANING: 
		{
			if (row == ResourceDirWrapper::TIMESTAMP && !m_PE->isReproBuild()) {
				bool isOk = false;
				int val = wrap->getNumValue(fId, &isOk);
				return (isOk) ? getDateString(val) : QVariant();
			}
		}
	}
	return QVariant();
}

//----------------------------------------------------------------------------


QVariant ResourceLeafModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	switch (section) {
		case OFFSET: return "Offset";
		case NAME: return "Name";
		case VALUE : return "Value";
	}
	return QVariant();
}

ExeElementWrapper* ResourceLeafModel::wrapper() const 
{
	return &myPeHndl->resourcesDirWrapper;
}

ExeElementWrapper* ResourceLeafModel::wrapperAt(QModelIndex index) const
{
	if (this->parentId >= myPeHndl->resourcesAlbum.dirsCount()) return NULL;
	std::vector<ResourceLeafWrapper*> *vec = myPeHndl->resourcesAlbum.entriesAt(parentId);
	if (!vec) return NULL;

	if (vec->size() <= leafId) return NULL;
	return vec->at(leafId);
}

QVariant ResourceLeafModel::data(const QModelIndex &index, int role) const
{
	ResourceLeafWrapper* wrap =  dynamic_cast<ResourceLeafWrapper*>(wrapperAt(index));
	if (!wrap) return QVariant();

	int row = index.row();
	int column = index.column();

	if (role == Qt::ForegroundRole) return this->addrColor(index);
	if (role == Qt::FontRole) {
		if (this->containsOffset(index) || this->containsValue(index)) return offsetFont;
	}
	if (role == Qt::ToolTipRole) return toolTip(index);
	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
	
	int sID = getSID(index);
	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
		case NAME : return (wrap->getFieldName(sID));
	}
	
	bool isOk = false;
	uint64_t val = wrap->getNumValue(sID, &isOk);
	if (!isOk) return "UNK"; /* other type? */

	return QString::number(val, 16);
}

Executable::addr_type ResourceLeafModel::addrTypeAt(QModelIndex index) const
{
	Executable::addr_type aType = Executable::NOT_ADDR;

	if (containsOffset(index)) {
		aType = offsetAddrType();
	} else {
		if (!containsValue(index)) return Executable::NOT_ADDR;

		ExeElementWrapper *wrapper = this->wrapperAt(index);
		if (wrapper == NULL) return Executable::NOT_ADDR;

		aType = wrapper->containsAddrType(getSID(index));
	}
	return aType;
}

offset_t ResourceLeafModel::getFieldOffset(QModelIndex index) const
{ 
	if (!index.isValid()) return 0;
	int fieldId = getSID(index);
	return wrapperAt(index) ? wrapperAt(index)->getFieldOffset(fieldId) : 0;
}

bufsize_t ResourceLeafModel::getFieldSize(QModelIndex index) const 
{
	if (!index.isValid()) return 0;
	int fieldId = getSID(index);
	return wrapperAt(index) ? wrapperAt(index)->getFieldSize(fieldId) : 0;
}

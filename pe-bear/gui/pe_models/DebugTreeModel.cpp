#include "DebugTreeModel.h"
#include "../../DateDisplay.h"

//-----------------------------------------------------------------------------

#define NOT_FILLED  "-"

QVariant DebugTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	switch (section) {
		case OFFSET: return "Offset";
		case NAME: return "Name";
		case VALUE : return "Value";
		case VALUE2: return "Meaning";
	}
	return QVariant();
}

QVariant DebugTreeModel::data(const QModelIndex &index, int role) const
{
	DebugDirWrapper* wrap = dynamic_cast<DebugDirWrapper*>(wrapper());
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
		case NAME: return wrap->getFieldName(fId);
		case VALUE2: 
		{
			bool isOk = false;
			int val = wrap->getNumValue(fId, &isOk);
			if (fId == DebugDirWrapper::TIMESTAMP) {
				if (!isOk) return "";
				return getDateString(val);
			}
			if (fId == DebugDirWrapper::TYPE) {
				if (!isOk) return "";
				return wrap->translateType(val);
			}
			return "";
		}
	}
	return dataValue(index);
}

QString DebugTreeModel::makeDockerTitle(uint32_t upId)
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

//-----------------------------------------------------------------------------------

int DebugRDSIEntryTreeModel::rowCount(const QModelIndex &parent) const
{
	if (!wrapper()) return 0;
	return wrapper()->getFieldsCount();
}

ExeElementWrapper* DebugRDSIEntryTreeModel::wrapper() const
{
	if (!myPeHndl) return NULL;
	
	ExeNodeWrapper* subWr = myPeHndl->debugDirWrapper.getEntryAt(0);
	return subWr;
}

ExeElementWrapper* DebugRDSIEntryTreeModel::wrapperAt(QModelIndex index) const
{
	return wrapper();
}

QVariant DebugRDSIEntryTreeModel::data(const QModelIndex &index, int role) const
{
	ExeElementWrapper *w = wrapperAt(index);
	DebugDirCVEntryWrapper* wrap = dynamic_cast<DebugDirCVEntryWrapper*>(w);
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
				char *str_ptr = (char*)w->getFieldPtr(fId);
				if (!str_ptr) return QVariant();
				return QString(str_ptr);
			}
			bool isOk = false;
			uint64_t val = w->getNumValue(fId, &isOk);
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

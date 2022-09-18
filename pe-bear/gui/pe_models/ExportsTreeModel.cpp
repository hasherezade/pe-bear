#include "ExportsTreeModel.h"
#include "../../DateDisplay.h"

//-----------------------------------------------------------------------------
QVariant ExportsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant ExportsTreeModel::data(const QModelIndex &index, int role) const
{
	ExeElementWrapper* wrap = wrapper();
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
			if (fId == ExportDirWrapper::TIMESTAMP) {
				bool isOk = false;
				int val = wrap->getNumValue(fId, &isOk);
				if (!isOk) return "";
				return getDateString(val);
			}
			if (fId == ExportDirWrapper::NAME_RVA) {
				ExportDirWrapper* expW = dynamic_cast<ExportDirWrapper*>(wrap);
				if (expW) {
					return expW->getLibraryName();
				}
				return wrap->getName();
			}
			return "";
		}
	}
	return dataValue(index);
}

QString ExportsTreeModel::makeDockerTitle(uint32_t upId)
{
	ExeNodeWrapper* node = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (node == NULL) {
		return "-";
	}
	bool isOk = false;
	int funcNum = node->getNumValue(ExportDirWrapper::FUNCTIONS_NUM, &isOk);
	if (!isOk) return "-";

	std::string name = "Exported Functions";
	QString numDesc = funcNum == 1 ? " entry" : " entries";
	QString desc = QString::fromStdString(name) + "   [ " + QString::number(funcNum) + numDesc + " ]"; 
	return desc;
}

//----------------------------------------------------------------------

int ExportedFuncTreeModel::rowCount(const QModelIndex &parent) const
{
	if (!wrapper()) return 0;
	ExeNodeWrapper *w = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (w == NULL) return 0;

	return w->getEntriesNum();
}

QVariant ExportedFuncTreeModel::data(const QModelIndex &index, int role) const
{
	int row = index.row();
	int column = index.column();

	if (role == Qt::ForegroundRole) return this->addrColor(index);
	if (role == Qt::ToolTipRole) return toolTip(index);

	if (role == Qt::FontRole) {
		if (column >= NAME) return QVariant();
		return offsetFont;
	}

	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	ExportEntryWrapper* w = dynamic_cast<ExportEntryWrapper*>(wrapperAt(index));
	if (w == NULL) return QVariant();

	int fId = this->getFID(index);

	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
		case ORDINAL : return QString::number(w->getOrdinal(), 16);
		case NAME_RVA:
		{
			uint64_t nameRva = w->getFuncNameRva();
			if (nameRva == INVALID_ADDR) return "-";
			return QString::number(nameRva, 16);
		}
		case NAME:
		{
			char *name = w->getFuncName();
			if (name == NULL) return QVariant();
			return QString(name);
		}
		case FORWARDER:
		{
			char *name = w->getForwarder();
			if (name == NULL) return QVariant();
			return QString(name);
		}
	}
	bool isOk = false;
	uint64_t val = w->getNumValue(0, &isOk);
	if (!isOk) return "UNK"; 

	return QString::number(val, 16);
}

bool ExportedFuncTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role != Qt::DisplayRole && role != Qt::EditRole) return false;

	ExportEntryWrapper* entry = dynamic_cast<ExportEntryWrapper*>(wrapperAt(index));
	if (!entry) return false;

	int fID = this->getFID(index);
	QString text = value.toString();
	int column = index.column();

	bool isModified = false;
	offset_t offset = INVALID_ADDR;
	size_t fieldSize = 0;
	
	switch (index.column()) {
		case ORDINAL:
			return false;
		case NAME:
		{
			char *textPtr = entry->getFuncName();
			if (!textPtr) return false;

			offset = entry->getOffset(textPtr);
			fieldSize = text.size() + 2;

			this->myPeHndl->backupModification(offset, fieldSize);
			isModified = m_PE->setTextValue(textPtr, text.toStdString(), fieldSize);
			break;
		}
		default:
		{
			bool isOk = false;
			ULONGLONG number = text.toLongLong(&isOk, 16);
			if (!isOk) return false;

			offset = entry->getFieldOffset(fID);
			fieldSize = entry->getFieldSize(fID);

			this->myPeHndl->backupModification(offset, fieldSize);
			isModified = entry->setNumValue(fID, number);
			break;
		}
	}

	if (isModified) {
		this->myPeHndl->setBlockModified(offset, fieldSize);
		return true;
	}
	this->myPeHndl->unbackupLastModification();
	return false;
}


QVariant ExportedFuncTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case OFFSET : return "Offset";
		case ORDINAL : return "Ordinal";
		case VALUE : return "Function RVA";
		case NAME_RVA : return "Name RVA";
		case NAME : return "Name";
		case FORWARDER: return "Forwarder";
	}
	return QVariant();
}

Executable::addr_type ExportedFuncTreeModel::addrTypeAt(QModelIndex index) const
{
	if (index.column() == NAME_RVA) {
		ExportEntryWrapper* w = dynamic_cast<ExportEntryWrapper*>(wrapperAt(index));
		if (w == NULL) return Executable::NOT_ADDR;

		if ( w->getFuncNameRva() == INVALID_ADDR) {
			return Executable::NOT_ADDR;
		}
		return Executable::RVA;
	}
	return  WrapperInterface::addrTypeAt(index);
}


Qt::ItemFlags ExportedFuncTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;

	static Qt::ItemFlags editable = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	static const  Qt::ItemFlags selectable = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	const int column = index.column();
	if (column == ORDINAL) return selectable;
	if (column >= 0) return editable;

	return selectable;
}

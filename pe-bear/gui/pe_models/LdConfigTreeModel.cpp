#include "LdConfigTreeModel.h"

//-----------------------------------------------------------------------------

#define NOT_FILLED  "-"


QVariant LdConfigTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();

	switch (section) {
		case OFFSET: return "Offset";
		case NAME: return "Name";
		case VALUE : return "Value";
		case MEANING: return "";
	}
	return QVariant();
}

QVariant LdConfigTreeModel::toolTip(QModelIndex index) const
{
	if (!index.isValid()) return "";

	const size_t fId = getFID(index);
	if (fId == LdConfigDirWrapper::GUARD_FLAGS) {
		LdConfigDirWrapper* wrap = dynamic_cast<LdConfigDirWrapper*>(wrapper());
		if (!wrap) return QVariant();
		return wrap->translateGuardFlagsContent("\n");
	}
	return PeTreeModel::toolTip(index);
}


QVariant LdConfigTreeModel::data(const QModelIndex &index, int role) const
{
	ExeElementWrapper* wrap = wrapper();
	if (!wrap) return QVariant();
	
	size_t fId = getFID(index);
	
	int column = index.column();
	if (role == Qt::ForegroundRole) return this->addrColor(index);
	if (role == Qt::FontRole) {
		if (this->containsOffset(index) || this->containsValue(index)) return offsetFont;
	}
	if (column == MEANING) {
		if (wrap->hasSubfieldWrapper(fId)) {
			if (role == Qt::DecorationRole) return ViewSettings::getScaledPixmap(":/icons/List.ico");
			if (role == Qt::ToolTipRole) return "List";
		}
		if (fId == LdConfigDirWrapper::GUARD_FLAGS) {
			if (role == Qt::DecorationRole) return ViewSettings::getScaledPixmap(":/icons/information.ico");
		}
	}
	if (role == Qt::ToolTipRole) return toolTip(index);
	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();
	
	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
		case NAME: return wrap->getFieldName(fId);
		case MEANING: 
		{
			return QVariant();
		}
	}
	return dataValue(index);
}

QString LdConfigTreeModel::makeDockerTitle(uint32_t upId)
{
	if (!wrapper()) return "";
	QString name = this->wrapper()->getFieldName(upId);

	LdConfigDirWrapper* node = dynamic_cast<LdConfigDirWrapper*>(wrapper());
	if (node == NULL) {
		return "";
	}
	if (!wrapper()->hasSubfieldWrapper(upId)) {
		return "";
	}
	uint32_t funcNum = node->getSubfieldWrapperCount(upId);
	QString numDesc = funcNum == 1 ? " entry" : " entries";
	QString desc = name + "   [ " + QString::number(funcNum) + numDesc + " ]";
	return desc;
}

//-----------------------------------------------------------------------------------

int LdEntryTreeModel::rowCount(const QModelIndex &parent) const
{
	if (!wrapper()) return 0;

	LdConfigDirWrapper *w = dynamic_cast<LdConfigDirWrapper*>(wrapper());
	if (w == NULL) return 0;
	
	// count wrappers in the parent directory
	return w->getSubfieldWrapperCount(parentId);
}

int LdEntryTreeModel::columnCount(const QModelIndex &parent) const
{
	LdConfigEntryWrapper *elW = getEntryWrapperAtID(0);
	if (elW == NULL) return 0;
	
	// count fields in child wrapper:
	return elW->getFieldsCount() + 1;
}

ExeElementWrapper* LdEntryTreeModel::wrapperAt(QModelIndex index) const
{
	if (!wrapper()) return NULL;

	LdConfigDirWrapper *w = dynamic_cast<LdConfigDirWrapper*>(wrapper());
	if (w == NULL) return NULL;
	// get wrapper from the parent directory
	return w->getSubfieldWrapper(parentId, this->getFID(index));
}

QVariant LdEntryTreeModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	if (role == Qt::ToolTipRole) return toolTip(index);
	if (role == Qt::FontRole) return offsetFont;

	int column = index.column();
	int sId = this->getSID(index);

	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	if (column == OFFSET) {
		return QString::number(getFieldOffset(index), 16);
	}
	LdConfigEntryWrapper *elW = dynamic_cast<LdConfigEntryWrapper*>(wrapperAt(index));
	if (elW == NULL) {
		return QVariant();
	}
	bool isOk = false;
	const uint64_t val = elW->getNumValue(sId, &isOk);
	if (!isOk) return QVariant();

	return QString::number(val, 16);
}

QVariant LdEntryTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole) return QVariant();
	
	LdConfigEntryWrapper *elW = getEntryWrapperAtID(0);
	if (elW == NULL) return QVariant();
	if (section == OFFSET) {
		return "Offset";
	}
	return elW->getFieldName(section - 1);
}

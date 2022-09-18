#include "TLSTreeModel.h"

///------------------------

QVariant TLSTreeModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	if (role == Qt::ToolTipRole) return toolTip(index);
	if (role == Qt::FontRole) {
		if (index.column() != COL_NAME) return offsetFont;
		return QVariant();
	}

	int row = index.row();
	int column = index.column();
	int fId = this->getFID(index);

	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	switch (column) {
		case COL_OFFSET: return QString::number(getFieldOffset(index), 16);
		case COL_NAME: return (wrapper()->getFieldName(fId));
	}
	bool isOk = false;
	uint64_t val = wrapper()->getNumValue(fId, &isOk);
	if (!isOk) return "UNK"; 

	return QString::number(val, 16);
}

QVariant TLSTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case COL_OFFSET : return "Offset";
		case COL_NAME : return "Name";
		case COL_VALUE : return "Value";
	}
	return QVariant();
}

QString TLSTreeModel::makeDockerTitle(uint32_t upId)
{
	ExeNodeWrapper *w = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (w == NULL) return "";
	size_t entriesNum = w->getEntriesNum();

	std::string name = "TLS Callbacks";
	QString numDesc = entriesNum == 1 ? " entry" : " entries";
	QString desc = QString::fromStdString(name) + "   [ " + QString::number(entriesNum) + numDesc + " ]"; 
	return desc;
}

//-----------------------------------------------------------------------------------

int TLSCallbacksModel::rowCount(const QModelIndex &parent) const
{
	ExeNodeWrapper *w = dynamic_cast<ExeNodeWrapper*>(wrapper());
	if (w == NULL) return 0;

	return w->getEntriesNum();
}

QVariant TLSCallbacksModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::ForegroundRole) return this->addrColor(index);

	if (role == Qt::ToolTipRole) return toolTip(index);
	if (role == Qt::FontRole) return offsetFont;

	int row = index.row();
	int column = index.column();
	int fId = this->getFID(index);

	if (role != Qt::DisplayRole && role != Qt::EditRole) return QVariant();

	switch (column) {
		case OFFSET: return QString::number(getFieldOffset(index), 16);
	}
	bool isOk = false;
	uint64_t val = wrapperAt(index)->getNumValue(0, &isOk);
	if (!isOk) return "UNK"; 

	return QString::number(val, 16);
}

QVariant TLSCallbacksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role != Qt::DisplayRole) return QVariant();
	switch (section) {
		case OFFSET : return "Offset";
		case VALUE : return "Callback";
	}
	return QVariant();
}

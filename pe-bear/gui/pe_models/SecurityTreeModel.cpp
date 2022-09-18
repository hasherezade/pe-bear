#include "SecurityTreeModel.h"

//-----------------------------------------------------------------------------

#define NOT_FILLED  "-"

QVariant SecurityTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant SecurityTreeModel::data(const QModelIndex &index, int role) const
{
	SecurityDirWrapper* wrap = dynamic_cast<SecurityDirWrapper*>(wrapper());
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
		case NAME: return (wrap->getFieldName(fId));
		case VALUE2: {
			if (fId == SecurityDirWrapper::TYPE) {
				bool isOk = false;
				int type = wrap->getNumValue(fId, &isOk);
				if (!isOk) return "";

				return wrap->translateType(type);
			}
			if (fId == SecurityDirWrapper::REVISION) {
				bool isOk = false;
				int type = wrap->getNumValue(fId, &isOk);
				if (!isOk) return "";
				return "";
				//return wrap->translateRevision(type);
			}
			return "";
		}
	}
	return dataValue(index);
}

//----------------------------------------------------------------------------

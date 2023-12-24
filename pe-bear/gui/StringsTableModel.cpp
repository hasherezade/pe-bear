#include "StringsTableModel.h"

StringsTableModel::StringsTableModel(PeHandler *peHndl, QObject *parent)
	: QAbstractTableModel(parent), m_PE(peHndl)
{
}

QVariant StringsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole) return QVariant();
	if (orientation == Qt::Horizontal) {
		switch (section) {
			case COL_OFFSET: return "Offset";
			case COL_TYPE: return "Type";
			case COL_STRING : return tr("String");
		}
	}
	return QVariant();
}

Qt::ItemFlags StringsTableModel::flags(const QModelIndex &index) const
{	
	if (!index.isValid()) return Qt::NoItemFlags;
	const Qt::ItemFlags fl = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	return fl;
}

int StringsTableModel::rowCount(const QModelIndex &parent) const 
{
	if (!m_PE) return 0;
	return m_PE->stringsMap.size();
}

QVariant StringsTableModel::data(const QModelIndex &index, int role) const
{
	int row = index.row();
	int column = index.column();
	
	if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole) return QVariant();

	StringsCollection &stringsMap = m_PE->stringsMap;
	QList<offset_t> stringsOffsets = stringsMap.getOffsets();

	if ((size_t)row >= stringsOffsets.size()) return QVariant();
	
	offset_t strOffset = stringsOffsets[row];
	switch (column) {
		case COL_OFFSET:
			return QString::number(strOffset, 16);
		case COL_TYPE:
			return stringsMap.isWide(strOffset) ? "W" : "A";
		case COL_STRING : 
			return stringsMap.getString(strOffset);
	}
	return QVariant();
}


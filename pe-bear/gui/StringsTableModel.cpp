#include "StringsTableModel.h"

StringsTableModel::StringsTableModel(PeHandler *peHndl, QObject *parent)
	: QAbstractTableModel(parent), m_PE(peHndl), page(1)
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

	const size_t stringsTotalCount = m_PE->stringsMap.size();
/*	if (stringsTotalCount < STRINGS_MAX) return stringsTotalCount;

	size_t _page = page != 0 ? page : 1;
	const size_t totalPages = (stringsTotalCount / STRINGS_MAX) + ((m_PE->stringsMap.size() % STRINGS_MAX) ? 1 : 0);
	if (page > totalPages) return 0;
	if (page == (totalPages - 1)) return (m_PE->stringsMap.size() % STRINGS_MAX); //last page, display reminder
	*/
	return stringsTotalCount;
}

QVariant StringsTableModel::data(const QModelIndex &index, int role) const
{
	if (!m_PE || m_PE->stringsMap.size() == 0) {
		return QVariant();
	}

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


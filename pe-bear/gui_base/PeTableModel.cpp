#include "PeTableModel.h"

#define COL_OFFSET 0

//---------------------------------------

PeTableModel::PeTableModel(PeHandler *peHndl, QObject *parent)
	: PeTreeModel(peHndl, parent, false)
{
}

Executable::addr_type PeTableModel::addrTypeAt(QModelIndex index) const
{
	if (!index.isValid() || !m_PE) return Executable::NOT_ADDR;
	if (index.column() == COL_OFFSET) return Executable::RAW;
	return Executable::NOT_ADDR;
}

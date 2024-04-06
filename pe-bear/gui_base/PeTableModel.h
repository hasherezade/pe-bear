#pragma once
#include <QtGlobal>

#include <bearparser/bearparser.h>

#include "../QtCompat.h"
#include "PeGuiItem.h"
#include "TreeCpView.h"
#include "../gui/PeTreeModel.h"

//--------------------------------------------------------------------------

class PeTableModel : public PeTreeModel
{
	Q_OBJECT

public:
	PeTableModel(PeHandler *peHndl, QObject *parent);

	virtual ~PeTableModel()
	{
	}

	QModelIndex index(int row, int column, const QModelIndex &parent) const
	{
		return createIndex(row, column);
	}

	virtual Executable::addr_type addrTypeAt(QModelIndex index) const;

	virtual offset_t getFieldOffset(QModelIndex index) const { return 0; }
	virtual bufsize_t getFieldSize(QModelIndex index) const { return 0; }

friend class PeTreeView;
};

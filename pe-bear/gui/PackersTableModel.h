#pragma once

#include <QtGlobal>

#include <map>
#include <set>

#include "../PEBear.h"
#include "../gui_base/FollowablePeTreeView.h"

class PackersTableModel : public PeTableModel
{
	Q_OBJECT

public:
	PackersTableModel(PeHandler *peHndl, QObject *parent = 0);
	
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	
	int columnCount(const QModelIndex &parent) const;
	int rowCount(const QModelIndex &parent) const;
	
	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int) { return false; } //external modifications not allowed

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const { return createIndex(row, column); } //no index item pointer
	QModelIndex parent(const QModelIndex &index) const { return QModelIndex(); } // no parent

	offset_t getFieldOffset(QModelIndex index) const;
	bufsize_t getFieldSize(QModelIndex index) const;

protected:
	QFont offsetFont;
};

#pragma once

#include <bearparser/bearparser.h>
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif


#include "../base/PeHandler.h"

class StringsTableModel : public QAbstractTableModel
{
	Q_OBJECT

signals:
	void modelUpdated();

protected slots:
	virtual void onNeedReset() { reset(); emit modelUpdated(); }

public:
	enum COLS {
		COL_OFFSET = 0,
		COL_TYPE,
		COL_STRING,
		MAX_COL
	};

	StringsTableModel(PeHandler *peHndl, QObject *parent = 0);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	int rowCount(const QModelIndex &parent) const;//{ return INFO_COUNTER; }

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &, const QVariant &, int) { return false; }

	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const { return createIndex(row, column); } //no index item pointer
	QModelIndex parent(const QModelIndex &index) const { return QModelIndex(); } // no parent

	void reset()
	{
		//>
		this->beginResetModel();
		this->endResetModel();
		//<
	}

protected:
	PeHandler *m_PE;
};
//----

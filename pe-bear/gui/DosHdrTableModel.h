#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include <map>
#include <set>

#include "PeWrapperModel.h"
#include "../gui_base/WrapperTableModel.h"

class DosHdrTableModel : public WrapperTableModel
{
	Q_OBJECT

public:
	enum COLS {
		COL_OFFSET = 0,
		COL_NAME,
		COL_VALUE,
		MAX_COL
	};

	DosHdrTableModel(PeHandler *peHndl, QObject *parent = 0);
	virtual ~DosHdrTableModel() { }

	virtual int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

	bool containsValue(QModelIndex index) const { return (index.column() == COL_VALUE); }

protected:
	virtual int getSID(const QModelIndex &index) const { return index.column() - COL_VALUE; }
	virtual bool isComplexValue(const QModelIndex &index) const;

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->dosHdrWrapper; }
};

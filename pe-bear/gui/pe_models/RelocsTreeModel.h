#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"

class RelocsTreeModel  : public WrapperTableModel
{
	Q_OBJECT

public:
	RelocsTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent) {}

	virtual int columnCount(const QModelIndex &parent) const;

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

	virtual bool containsValue(QModelIndex index) const { return (getFID(index) < RelocBlockWrapper::ENTRIES_PTR); }

protected:
	virtual int getFID(const QModelIndex &index) const { return index.column() - ADDED_COLS_NUM;  }
	virtual int getSID(const QModelIndex &index) const { return index.column(); }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->relocDirWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;

	inline int32_t columnToFID(int column) const { return column - ADDED_COLS_NUM; }
	inline int32_t FIDtoColumn(int fId) const { return (fId + ADDED_COLS_NUM); }

	virtual bool isComplexValue(const QModelIndex &index) const { if (index.column() >= ADDED_COLS_NUM) return true; return false; }

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET,
		ADDED_COLS_NUM
	};
friend class RelocsTreeView;
};


class RelocEntriesModel : public WrapperTableModel
{
	Q_OBJECT

public slots:
	void setParentId(size_t parentId) { this->parentId = parentId; reset(); }

public:
	RelocEntriesModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent), parentId(0) {}

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const { return MAX_COL; }
	
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	
	virtual bool containsValue(QModelIndex index) const { return index.column() == VALUE; }
	virtual ExeElementWrapper* wrapper() const { return myPeHndl->relocDirWrapper.getEntryAt(parentId);}
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;//

	Executable::addr_type addrTypeAt(QModelIndex index) const;

protected:
	virtual int getFID(const QModelIndex &index) const { return index.row();  }
	virtual int getSID(const QModelIndex &index) const { return index.column();  }

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET = 0,
		VALUE,
		TYPE,
		DELTA,
		RELOC_RVA,
		MAX_COL
	};

	size_t parentId;
};

#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/WrapperTableModel.h"


class ImportsTreeModel  : public WrapperTableModel
{
	Q_OBJECT

public:
	ImportsTreeModel(PeHandler *peHndl, QObject *parent = 0)
		: WrapperTableModel(peHndl, parent)
	{
		this->connectSignals();
	}

	virtual int columnCount(const QModelIndex &parent) const;

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;

	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual bool containsValue(QModelIndex index) const { return true; }

protected:
	virtual int getFID(const QModelIndex &index) const { return index.column() - ADDED_COLS_NUM;  }
	virtual int getSID(const QModelIndex &index) const { return index.row(); }

	virtual ExeElementWrapper* wrapper() const { return &myPeHndl->importDirWrapper; }
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;

	inline int32_t columnToFID(int column) const { return column - ADDED_COLS_NUM; }
	inline int32_t FIDtoColumn(int fId) const { return (fId + ADDED_COLS_NUM); }

	virtual bool isComplexValue(const QModelIndex &index) const { if (index.column() >= ADDED_COLS_NUM) return true; return false; }

	enum FieldID {
		NONE = FIELD_NONE,
		OFFSET,
		NAME,
		FUNC_NUM,
		IS_BOUND,
		ADDED_COLS_NUM
	};
};

//----------------------------------------------------------------------

class ImportedFuncModel  : public WrapperTableModel
{
	Q_OBJECT

public slots:
	void setParentId(size_t libraryId) { this->libraryId = libraryId; reset(); }

public:
	ImportedFuncModel(PeHandler *peHndl, QObject *parent = 0);

	virtual int columnCount(const QModelIndex &parent) const;
	virtual int rowCount(const QModelIndex &parent) const;

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual QVariant data(const QModelIndex &index, int role) const;

	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual Executable::addr_type offsetAddrType() const { return Executable::RVA; }
	virtual bool containsValue(QModelIndex index) const { return true; }

protected:
	virtual ImportEntryWrapper* wrapper() const;
	virtual ExeElementWrapper* wrapperAt(QModelIndex index) const;

	virtual bool isComplexValue(const QModelIndex &index) const { return false; }

	virtual int getFID(const QModelIndex &index) const { return index.column() - ADDED_COLS_NUM; }
	virtual int getSID(const QModelIndex &index) const { return index.row(); }

	inline int32_t columnToFID(int column) const { return (column - ADDED_COLS_NUM); }
	inline int32_t FIDtoColumn(int fId) const { return (fId + ADDED_COLS_NUM); }

	enum FieldID {
		NONE = FIELD_NONE,
		CALL_VIA,
		NAME,
		ORDINAL,
		ADDED_COLS_NUM
	};
	uint32_t libraryId;
};

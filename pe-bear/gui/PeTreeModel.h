#pragma once

#include "TreeModel.h"
#include "../gui_base/PeGuiItem.h"

class PeTreeItem : public TreeItem, public PeViewItem
{
	Q_OBJECT

public:
	PeTreeItem(PeHandler *peHndl, int fID, int sID, PeTreeItem *parent = NULL);
	
	virtual QVariant data(int column) const = 0;

	virtual bool setData(int column, const QVariant &value, int role);
	virtual QVariant toolTip(int column) const;
	
	virtual bool canFollow(int column, int row) const;
	virtual bool containsRVA(int column, int row) const;

	virtual Qt::ItemFlags flags(int column) const
	{
		return Qt::NoItemFlags;
	}

	virtual int getFID() const { return this->fID; }
	virtual int getSID() const { return this->sID; }
	
protected:
	virtual bool setDataValue(int column, const QVariant &value)
	{
		return false;
	}

	int fID;
	int sID;

	QFont offsetFont;
	QColor offsetFontColor;
	QColor errColor;
};

//--------------------------------------------------------------------------

class PeTreeModel : public TreeModel, public PeViewItem
{
	Q_OBJECT

signals:
	void modelUpdated();

public slots:
	virtual void onNeedReset()
	{
		reset();
		emit modelUpdated();
	}

protected: 
	virtual void connectSignals();
	
	void reset()
	{
		//>
		this->beginResetModel();
		this->endResetModel();
		//<
	}

public :
	PeTreeModel(PeHandler *peHndl, QObject *parent, bool isExpandable = true);

	virtual ~PeTreeModel()
	{
		PeTreeModel::counter--;
	}

	virtual bool isExpandable() { return m_isExpandable; }
	virtual Executable::addr_type addrTypeAt(QModelIndex index) const;

	virtual QVariant addrColor(const QModelIndex &index) const;
	virtual QVariant toolTip(QModelIndex index) const;

	virtual offset_t getFieldOffset(QModelIndex index) const; // TODO: refactor it
	virtual bufsize_t getFieldSize(QModelIndex index) const; // TODO: refactor it

	QFont offsetFont;
	QColor offsetFontColor, errColor;
	
protected:
	bool m_isExpandable;
	static size_t counter;
};

//--------------------------------------------------------------------------

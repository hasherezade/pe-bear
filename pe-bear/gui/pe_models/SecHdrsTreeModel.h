#pragma once

#include <QtGui>
#include <map>
#include <set>

#include "../../gui_base/PeTreeView.h"
#include "../TreeModel.h"
#include "../PeTreeModel.h"

class SecHdrsTreeModel;
class SecTreeItem : public PeTreeItem
{
	Q_OBJECT

public:
	static QStringList fetchSecHdrCharact(DWORD characteristics);

//---
	SecTreeItem(PeHandler* peHndl, int secIndx = (-1), int level = 0, SecTreeItem *parent = NULL);

	int columnCount() const;

	QVariant foreground(int column) const;
	QVariant background(int column) const;
	QVariant toolTip(int column) const;

	QVariant edit(int column) const;
	Qt::ItemFlags flags(int column) const;
	QVariant data(int column) const;

public slots:
/* PEGuiItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;

protected:
	void updateSectionsList();
	virtual bool isChildOk(TreeItem* child) { return (dynamic_cast<SecTreeItem*>(child) != 0); }
	virtual bool setDataValue(int column, const QVariant &value);

	inline SectionHdrWrapper* getMySection() const;

private:
	int secIndx;
	int level;

friend class SecHdrsTreeModel;
};


class SecHdrsTreeModel : public PeTreeModel
{
	Q_OBJECT

signals:
	void sectionHdrsModified(PEFile* m_PE);
	
public slots:
	void onSectionNumChanged();

public:
	SecHdrsTreeModel(PeHandler* peHndl, QObject *parent = 0);
	~SecHdrsTreeModel() { }

	virtual QVariant data(const QModelIndex &index, int role) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual Executable::addr_type addrTypeAt(QModelIndex index) const;

/* PEGuiItem interface */
	virtual offset_t getContentOffset() const;
	virtual bufsize_t getContentSize() const;
};

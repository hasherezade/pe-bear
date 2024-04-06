#pragma once
#include <QtGlobal>

#include "../QtCompat.h"

//---

class TreeItem : public QObject
{
	Q_OBJECT
public:
	explicit TreeItem(TreeItem *parentItem = nullptr);
	explicit TreeItem(const QList<QVariant> &data, TreeItem *parentItem = nullptr);
	virtual ~TreeItem();

	TreeItem *child(int row);
	virtual int childCount() const;
	virtual int columnCount() const;
	virtual QVariant data(int column) const;
	virtual int row() const;
	TreeItem *parentItem();

	virtual void appendChild(TreeItem *child);
	virtual void detachChild(TreeItem *child);
	virtual void removeAllChildren();

protected:
	QList<TreeItem*> m_childItems;
	QList<QVariant> m_itemData;
	TreeItem *m_parentItem;
};

//---

class TreeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit TreeModel(QObject *parent = nullptr);
	~TreeModel();

	virtual QVariant data(const QModelIndex &index, int role) const override;
	
	virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

	virtual QVariant headerData(int section, Qt::Orientation orientation,
		int role = Qt::DisplayRole) const override;

	virtual QModelIndex index(int row, int column,
		const QModelIndex &parent = QModelIndex()) const override;

	virtual QModelIndex parent(const QModelIndex &index) const override;

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;

	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;

protected:

	TreeItem *rootItem;
};

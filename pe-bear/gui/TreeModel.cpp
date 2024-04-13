#include "TreeModel.h"
#include <iostream>

//class appendChild;
TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent)
	: m_itemData(data), m_parentItem(parent), QObject(parent)
{
}

TreeItem::TreeItem(TreeItem *parent)
	: QObject(parent),
	m_parentItem(parent)
{
}

TreeItem::~TreeItem()
{
	qDeleteAll(m_childItems);
}

void TreeItem::appendChild(TreeItem *item)
{
	if (!item) {
		return;
	}
	if (item->parentItem()) {
		item->parentItem()->detachChild(item);
	}

	// connect the child to the parent:
	item->m_parentItem = this;
	item->setParent(this);

	// add the item to the children:
	this->m_childItems.append(item);
}

void TreeItem::detachChild(TreeItem *child)
{
	if (!child) {
		return;
	}
	int index = this->m_childItems.indexOf(child, 0);
	if (index == -1) {
		/* not attached */
		return;
	}

	// remove the item from children
	this->m_childItems.removeAt(index);

	// detach the child from the parent
	child->m_parentItem = NULL;
	child->setParent(NULL);
}

void TreeItem::removeAllChildren()
{
	for (auto chIter = m_childItems.begin(); chIter != m_childItems.end(); ++chIter) {
		TreeItem* child = *chIter;
		if (child) {
			delete child;
		}
	}
	m_childItems.clear();
}

TreeItem *TreeItem::child(int row)
{
	if (row < 0 || row >= m_childItems.size())
		return nullptr;
	return m_childItems.at(row);
}

int TreeItem::childCount() const
{
	return m_childItems.count();
}

int TreeItem::row() const
{
	if (m_parentItem)
		return m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this));

	return 0;
}

int TreeItem::columnCount() const
{
	return m_itemData.count();
}

QVariant TreeItem::data(int column) const
{
	if (column < 0 || column >= m_itemData.size()) {
		return QVariant();
	}
	return "demo";
}

TreeItem *TreeItem::parentItem()
{
	return m_parentItem;
}

//----

TreeModel::TreeModel(QObject *parent)
	: QAbstractItemModel(parent), rootItem(nullptr)
{
}

TreeModel::~TreeModel()
{
	delete rootItem;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!rootItem || !hasIndex(row, column, parent)) {
		return QModelIndex();
	}
	TreeItem *parentItem = nullptr;

	if (!parent.isValid()) {
		parentItem = rootItem;
	}
	else {
		parentItem = static_cast<TreeItem*>(parent.internalPointer());
	}
	TreeItem *childItem = parentItem->child(row);
	if (childItem) {
		return createIndex(row, column, childItem);
	}
	return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
	if (!rootItem || !index.isValid()) {
		return QModelIndex();
	}

	TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
	TreeItem *parentItem = childItem->parentItem();

	if (parentItem == rootItem) {
		return QModelIndex();
	}
	return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0) {
		return 0;
	}
	TreeItem *parentItem;
	if (!parent.isValid()) {
		parentItem = rootItem;
	}
	else {
		parentItem = static_cast<TreeItem*>(parent.internalPointer());
	}
	if (!parentItem) {
		return 0;
	}
	const int childCount = parentItem->childCount();
	return childCount;
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
	TreeItem *parentItem = nullptr;
	if (parent.isValid()) {
		parentItem = static_cast<TreeItem*>(parent.internalPointer());
	}
	else {
		parentItem = rootItem;
	}
	if (!parentItem) {
		return 0;
	}
	const int colCount = parentItem->columnCount();
	return colCount;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}
	if (role != Qt::DisplayRole) {
		QVariant();
	}
	TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
	if (!item) {
		return QVariant();
	}
	return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}
	return QAbstractItemModel::flags(index);
}


QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (!rootItem) {
		return QVariant();
	}
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		return rootItem->data(section);
	}
	return QVariant();
}

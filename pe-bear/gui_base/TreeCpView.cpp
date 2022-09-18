#include "TreeCpView.h"


TreeCpView::TreeCpView(QWidget *parent)
	: QTreeView(parent), defaultMenu(this),
	myMenu(&defaultMenu)
{
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuEvent(QPoint)) );
	enableMenu(false);
}

void TreeCpView::resizeColsToContent()
{
	QAbstractItemModel *model = this->model();
	if (!model) return;

	int count = model->columnCount(QModelIndex());
	for (int i = 0; i < count; i++) {
		resizeColumnToContents(i);
	}
}

void TreeCpView::enableMenu(bool enable)
{
	if (enable) this->setContextMenuPolicy(Qt::CustomContextMenu);
	else this->setContextMenuPolicy(Qt::DefaultContextMenu);
}

void TreeCpView::customMenuEvent(QPoint p)
{
	if (myMenu == NULL) return;
	QPoint p2 = this->mapToGlobal(p); 
	this->myMenu->exec(p2);
}

void TreeCpView::keyPressEvent(QKeyEvent *keyEv)
{
	if (keyEv->matches(QKeySequence::Copy)) {
		copySelected();
		return;
	}
	QTreeView::keyPressEvent(keyEv);
}

void TreeCpView::setMenu(QMenu *menu)
{
	myMenu = menu;
}

void TreeCpView::removeAllActions()
{
	if (myMenu != NULL) {
		QList<QAction*> actList = myMenu->actions();
		for (int i = 0; i < actList.size(); i++) {
			myMenu->removeAction(actList[i]);
		}
	}
}

void TreeCpView::copySelected()
{
	QItemSelectionModel *model = this->selectionModel();
	QModelIndexList list = model->selectedIndexes();
	std::sort(list.begin(), list.end());
	int row = -1;
	int col = -1;
	QString separator = " ";
	const int size = list.size();
	QStringList strings;
	
	for (int i = 0; i < size; i++) {
		QModelIndex index = list.at(i);
		if (index.row() != row && row != (-1))
			strings << "\n";
		else if (col != -1) strings << separator;

		strings << index.data().toString();
		row = index.row();
		col = index.column();
	}
	QApplication::clipboard()->clear();
	QApplication::clipboard()->setText(strings.join(""));
}
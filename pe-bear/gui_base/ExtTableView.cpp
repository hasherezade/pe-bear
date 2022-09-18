#include "ExtTableView.h"
#include <QtGlobal>

ExtTableView::ExtTableView(QWidget *parent)
	: QTableView(parent), 
	defaultMenu(this), myMenu(&defaultMenu)
{
	init();
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuEvent(QPoint)) );
	enableMenu(false);
}

void ExtTableView::init()
{
	QHeaderView *verticalHdr = this->verticalHeader();
	if (verticalHdr) {
#if QT_VERSION >= 0x050000
		verticalHdr->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
		verticalHdr->setResizeMode(QHeaderView::ResizeToContents);
#endif
	}
	QHeaderView *horizontalHdr = this->horizontalHeader();
	if (horizontalHdr) {
#if QT_VERSION >= 0x050000
		horizontalHdr->setSectionResizeMode(QHeaderView::ResizeToContents);
#else
		horizontalHdr->setResizeMode(QHeaderView::ResizeToContents);
#endif
	}
	this->setWordWrap(false);
}

void ExtTableView::enableMenu(bool enable)
{
	if (enable) this->setContextMenuPolicy(Qt::CustomContextMenu);
	else this->setContextMenuPolicy(Qt::DefaultContextMenu);
}

void ExtTableView::customMenuEvent(QPoint p)
{
	if (myMenu == NULL) return;
	QPoint p2 = this->mapToGlobal(p); 
	this->myMenu->exec(p2);
}

void ExtTableView::setMenu(QMenu *menu)
{
	myMenu = menu;
}

void ExtTableView::removeAllActions()
{
	if (myMenu != NULL) {
		QList<QAction*> actList = myMenu->actions();
		for (int i = 0; i < actList.size(); i++) {
			myMenu->removeAction(actList[i]);
		}
	}
}

void ExtTableView::keyPressEvent(QKeyEvent *kEvent)
{
	//the default table copy operation copy only one selected index
	if (kEvent->matches(QKeySequence::Copy)) {
		copySelected();
		return;
	} else if (kEvent->matches(QKeySequence::Paste)) {
		pasteToSelected();
		return;
	}
	QTableView::keyPressEvent(kEvent);
}

QString ExtTableView::getSelectedText(QString hSeparator, QString vSeperator)
{
	QItemSelectionModel *model = this->selectionModel();
	if (!model) return "";

	QModelIndexList list = model->selectedIndexes();
	std::sort(list.begin(), list.end());

	int prevRow = -1;
	int prevCol = -1;

	const int size = list.size();
	QStringList strings;
	
	for (int i = 0; i < size; i++) {

		QModelIndex index = list.at(i);

		if (prevRow != (-1) && index.row() != prevRow) {
			strings << vSeperator;
		} else if (prevCol != (-1)) {
			strings << hSeparator;
		}
		strings << index.data().toString();

		prevRow = index.row();
		prevCol = index.column();
	}
	return strings.join("");
}

void ExtTableView::copyText(QString hSeparator, QString vSeperator)
{
	QApplication::clipboard()->clear();
	QApplication::clipboard()->setText(getSelectedText(hSeparator, vSeperator));
}

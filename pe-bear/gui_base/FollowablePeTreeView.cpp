#include "FollowablePeTreeView.h"

#define COL_OFFSET 0

//---------------------------------------------------------
FollowablePeTreeView::FollowablePeTreeView(QWidget *parent)
	: MainSettingsHolder(), PeTreeView(parent)
{
	selectedOffset = INVALID_ADDR;
	enableMenu(true);

	setSelectionBehavior(QAbstractItemView::SelectItems);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setAlternatingRowColors(true);

	followOffsetAction = new QAction("Follow offset", this);
	this->defaultMenu.addAction(followOffsetAction);
	connect(followOffsetAction, SIGNAL(triggered()), this, SLOT(followOffset()));
	
	onClick = new QAction("Follow on click", this);
	onClick->setCheckable(true);
	MainSettings* set =  getSettings();
	if (set) {
		onClick->setChecked(set->isFollowOnClick());
		connect(set, SIGNAL(settingsChanged()), this, SLOT(onSettingsChanged()) );
	}
	
	this->defaultMenu.addAction(onClick);
	connect(onClick, SIGNAL(triggered(bool)), this, SLOT(setFollowOnClick(bool)));
}

void FollowablePeTreeView::onSettingsChanged()
{
	MainSettings* set =  getSettings();
	if (set) {
		onClick->setChecked(set->isFollowOnClick());
	}
}

void FollowablePeTreeView::setFollowOnClick(bool isEnabled)
{
	MainSettings* set =  getSettings();
	if (set == NULL) {
		printf("Settings == NULL\n");
		return;
	}
	if (set->isFollowOnClick() == isEnabled) return; //nothing changed
	set->setFollowOnClick(isEnabled);
	onClick->setChecked(set->isFollowOnClick());
}

void FollowablePeTreeView::mousePressEvent(QMouseEvent *event)
{
	if (!myModel) return;
	PeTreeView::mousePressEvent(event);
	QModelIndex index = this->indexAt(event->pos());
	this->addrType = myModel->addrTypeAt(index);

	if (addrType == Executable::NOT_ADDR) {
		this->selectedOffset = INVALID_ADDR;
		followOffsetAction->setEnabled(false);
		onClick->setEnabled(false);
		return;
	}
	QVariant data = index.data();
	bool isOk = false;
	offset_t offset = data.toString().toLongLong(&isOk, 16);
	if (!isOk) {
		offset = INVALID_ADDR;
		addrType = Executable::NOT_ADDR;
	}
	this->selectedOffset = offset;
	followOffsetAction->setEnabled(isOk);
	onClick->setEnabled(isOk);
	MainSettings* set =  getSettings();
	if (isOk && set != NULL && set->isFollowOnClick() == true) {
		followOffset();
	}
}

void FollowablePeTreeView::mouseMoveEvent(QMouseEvent *event)
{
	if (!myModel) return;
	PeTreeView::mouseMoveEvent(event);
	QModelIndex index = this->indexAt(event->pos());
	this->addrType = myModel->addrTypeAt(index);

	if (this->addrType == Executable::NOT_ADDR) {
		this->setCursor(Qt::ArrowCursor);
		return;
	}
	this->setCursor(Qt::PointingHandCursor);
}

bool FollowablePeTreeView::hasAnyActionEnabled()
{
	QList<QAction*> actions = defaultMenu.actions();
	QList<QAction*>::Iterator itr;
	for (itr = actions.begin(); itr != actions.end(); ++itr) {
		if ( (*itr)->isEnabled() == true) return true;
	}
	return false;
}

void FollowablePeTreeView::customMenuEvent(QPoint p)
{
	if (!pe()) return;
	
	bool canFollow = this->addrType != Executable::NOT_ADDR;
	followOffsetAction->setEnabled(canFollow);
	if (!canFollow) {
		if (hasAnyActionEnabled() == false) {
			return;
		}
	}
	QString type;
	switch (this->addrType) {
	case Executable::RAW : 
		type = "raw"; 
		break;
	case Executable::RVA : 
		type = "RVA"; 
		break;
	case Executable::VA : 
		type = "VA"; 
		break;
	}
	followOffsetAction->setText("Follow " + type + ": " + QString::number(this->selectedOffset, 16).toUpper());
	QPoint p2 = this->mapToGlobal(p); 
	this->defaultMenu.exec(p2);
}

void FollowablePeTreeView::followOffset()
{
	if (selectedOffset == INVALID_ADDR) return;
	if (this->addrType == Executable::NOT_ADDR) return;
	if (!pe() || !peHndl()) return;

	try {
		offset_t raw = pe()->toRaw(this->selectedOffset, this->addrType, true);
		if (raw == INVALID_ADDR) {
			QMessageBox::warning(this, "Failed!", "Cannot follow - invalid address!");
			return;
		}
		peHndl()->setDisplayed(false, raw);

	} catch (CustomException &e) {
		QMessageBox::warning(this, "Cannot follow!", e.getInfo());
	}
}

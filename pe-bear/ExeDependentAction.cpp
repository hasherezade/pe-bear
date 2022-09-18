#include "ExeDependentAction.h"

void ExeDependentAction::updateEnabled()
{
	bool isSet = this->currHndl != NULL;
	this->setEnabled(this->currHndl != NULL);
}

void ExeDependentAction::init()
{
	connect(this, SIGNAL(triggered()), this, SLOT(onTriggered()));
	updateEnabled();
}

void ExeDependentAction::onHandlerSelected(PeHandler *hndl)
{
	if (this->currHndl == hndl) return; 
	this->currHndl = hndl; 
	
	updateEnabled();
	emit currHndlChanged();
}
//-------------------------

void ExeDependentMenu::addAction(QAction *action)
{
	if (!action) return;

	ExeDependentAction *exeAction = dynamic_cast <ExeDependentAction*> (action);
	if (exeAction) {
		connect(this, SIGNAL(handlerSet(PeHandler*)), action, SLOT(onHandlerSelected(PeHandler*)) );
	}
	QMenu::addAction(action);
}

void ExeDependentMenu::removeAction(QAction *action)
{
	if (!action) return;

	ExeDependentAction *exeAction = dynamic_cast <ExeDependentAction*> (action);
	if (exeAction) {
		disconnect(this, SIGNAL(handlerSet(PeHandler*)), action, SLOT(onHandlerSelected(PeHandler*)) );
	}
	QMenu::removeAction(action);
}

void ExeDependentMenu::onExeChanged(PeHandler* hndl) 
{
	if (this->myHndl == hndl) {
		return; //no changes
	}
	this->myHndl = hndl;

	bool isEnabled = this->myHndl != NULL;
	this->setEnabled(isEnabled);
	
	emit handlerSet(this->myHndl);
}

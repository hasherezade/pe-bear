#include "OffsetDependentAction.h"


OffsetDependentAction::OffsetDependentAction(const Executable::addr_type addrT, const QString &text, QObject* parent) 
		: QAction(text, parent), 
		addrType(addrT), title(text), offset(INVALID_ADDR) 
{ 
	onOffsetChanged(offset); 
	connect(this, SIGNAL(triggered()), this, SLOT(onTriggered()));
}

void OffsetDependentAction::onOffsetChanged(offset_t offset)
{
	this->offset = offset;
	if (offset == INVALID_ADDR) {
		this->setEnabled(false);
		this->setVisible(false);
		this->setText("-");
		return;
	}
	this->setEnabled(true);
	this->setVisible(true);
	QString text = title + " "+ QString::number(offset, 16).toUpper();
	this->setText(text); 
}

void OffsetDependentAction::onOffsetChanged(offset_t offset, Executable::addr_type addrT)
{
	this->addrType = addrT;
	onOffsetChanged(offset);
}

//----------------------------------------------

void OffsetDependentMenu::addAction(QAction *action)
{
	if (!action) return;

	OffsetDependentAction *offAction = dynamic_cast <OffsetDependentAction*> (action);
	if (offAction) {
		connect(this, SIGNAL(offsetUpdated(offset_t, Executable::addr_type)), action, SLOT(onOffsetChanged(offset_t, Executable::addr_type)) );
	}
	QMenu::addAction(action);
}

void OffsetDependentMenu::removeAction(QAction *action)
{
	if (!action) return;

	OffsetDependentAction *offAction = dynamic_cast <OffsetDependentAction*> (action);
	if (offAction) {
		disconnect(this, SIGNAL(offsetUpdated(offset_t, Executable::addr_type)), action, SLOT(onOffsetChanged(offset_t, Executable::addr_type)) );
	}
	QMenu::removeAction(action);
}

void OffsetDependentMenu::onOffsetChanged(offset_t offset, Executable::addr_type addrType)
{
	if (this->offset == offset && this->addrType == addrType) {
		return; //no changes
	}
	emit offsetUpdated(offset, addrType);

	this->addrType = addrType;
	this->offset = offset;

	if (offset == INVALID_ADDR) {
		this->setEnabled(false);
		//this->setVisible(false);
		this->setTitle("-");
		return;
	}
	this->setEnabled(true);
	QString text = title + " "+ QString::number(offset, 16).toUpper();
	this->setTitle(text); 
}

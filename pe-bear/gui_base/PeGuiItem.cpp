#include "PeGuiItem.h"

#define DBG_LVL 0

uint64_t PeGuiItem::counter = 0;

PeGuiItem::PeGuiItem(PeHandler *peHndl)
	: m_PE(NULL), myPeHndl(NULL)
{
	//if (!peHndl || !peHndl->getPe()) throw CustomException("Invalid initialization: PE Handler is NULL!");
	if (peHndl) {
		peHndl->incRefCntr();
		m_PE = peHndl->getPe();
	}
	myPeHndl = peHndl;
	PeGuiItem::counter++;
}

PeGuiItem::~PeGuiItem()
{
	if (myPeHndl) {
		myPeHndl->release();
	}
	myPeHndl = NULL;
	m_PE = NULL;
	PeGuiItem::counter--;
}

//---------------------------------------------------------------------

MainSettings* MainSettingsHolder::mainSettings = NULL;

MainSettings* MainSettingsHolder::getSettings()
{
	if (MainSettingsHolder::mainSettings != NULL) {
		return MainSettingsHolder::mainSettings;
	}
	if (this->mySettings == NULL) {
		mySettings = new MainSettings();
	}
	return mySettings;
}

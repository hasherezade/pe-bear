#pragma once

#include <QtCore>
#include <bearparser/bearparser.h>

#include "../base/MainSettings.h"
#include "../base/PeHandlersManager.h"

#include "../ViewSettings.h"

class PeGuiItem
{
public:
	/*throws Exception if PeHandler or PeFile is NULL! */
	PeGuiItem(PeHandler *peHndl);
	virtual ~PeGuiItem();

	virtual PeHandler* getPeHandler() const { return myPeHndl; }
	virtual PEFile* getPE() const { if (!myPeHndl) return NULL; return myPeHndl->getPe(); }
	
	ColorSettings addrColors;
	
protected:
	static uint64_t counter;
	PeHandler* myPeHndl;
	PEFile* m_PE;
};

//----

class PeViewItem : public PeGuiItem
{
public:
	PeViewItem(PeHandler *peHndl) : PeGuiItem(peHndl) {}
	virtual ~PeViewItem() {}

	virtual offset_t getContentOffset() const { return 0; }
	virtual bufsize_t getContentSize() const { return (m_PE) ? m_PE->getRawSize() : 0; }
};

class MainSettingsHolder 
{
public:
	static void setMainSettings(MainSettings* settings) { MainSettingsHolder::mainSettings = settings; }

	MainSettingsHolder() : mySettings(NULL) {}
	virtual ~MainSettingsHolder() { delete mySettings; }

protected:
	MainSettings* getSettings();
	static MainSettings* mainSettings;
	//---
	MainSettings* mySettings;
};

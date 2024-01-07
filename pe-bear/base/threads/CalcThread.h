#pragma once
#include "CollectorThread.h"
#include "SupportedHashes.h"
#include "../PeHandler.h"

class CalcThread : public CollectorThread
{
	Q_OBJECT
	
public:

	CalcThread(SupportedHashes::hash_type hType, PEFile* pe, offset_t checksumOffset)
		: CollectorThread(pe), hashType(hType), checksumOff(checksumOffset)
	{
	}

signals:
	void gotHash(QString hash, int type);

private:
	void run();
	QString makeImpHash();
	QString makeRichHdrHash();

	SupportedHashes::hash_type hashType;
	offset_t checksumOff;
};


///----

class HashCalcThreadManager : public CollectorThreadManager
{
public:
	HashCalcThreadManager(PeHandler *peHndl, SupportedHashes::hash_type hType)
		: m_peHndl(peHndl), m_hashType(hType)
	{
	}
	
	bool setupThread()
	{
		if (!m_peHndl) return false;
		
		offset_t checksumOffset = m_peHndl->optHdrWrapper.getFieldOffset(OptHdrWrapper::CHECKSUM);
		CalcThread *calcThread = new CalcThread(m_hashType, m_peHndl->getPe(), checksumOffset);
		this->myThread = calcThread;
		QObject::connect(calcThread, SIGNAL(gotHash(QString, int)), m_peHndl, SLOT(onHashReady(QString, int)));
		return true;
	}
	
protected:
	SupportedHashes::hash_type m_hashType;
	PeHandler *m_peHndl;
};

///----


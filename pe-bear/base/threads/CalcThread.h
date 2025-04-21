#pragma once
#include "CollectorThread.h"
#include "SupportedHashes.h"
#include "../PeHandler.h"

class CalcThread : public CollectorThread
{
	Q_OBJECT
	
public:

	CalcThread(AbstractByteBuffer* buf, offset_t checksumOffset)
		: CollectorThread(buf), checksumOff(checksumOffset)
	{
	}

signals:
	void gotHash(QString hash, int type);

private:
	void run();
	QString makeImpHash(PEFile* pe);
	QString makeRichHdrHash(PEFile* pe);

	offset_t checksumOff;
};

///----

class HashCalcThreadManager : public CollectorThreadManager
{
public:
	HashCalcThreadManager(PeHandler *peHndl)
		: m_peHndl(peHndl)
	{
	}
	
	bool setupThread()
	{
		if (!m_peHndl) return false;
		PEFile* pe = m_peHndl->getPe();
		if (!pe) return false;

		offset_t checksumOffset = m_peHndl->optHdrWrapper.getFieldOffset(OptHdrWrapper::CHECKSUM);
		CalcThread *calcThread = new CalcThread(pe->getFileBuffer(), checksumOffset);
		this->myThread = calcThread;

		QObject::connect(calcThread, SIGNAL(gotHash(QString, int)), m_peHndl, SLOT(onHashReady(QString, int)));
		return true;
	}
	
protected:
	PeHandler *m_peHndl;
};

///----


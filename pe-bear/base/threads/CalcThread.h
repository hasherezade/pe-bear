#pragma once
#include "CollectorThread.h"
#include "SupportedHashes.h"
#include "../PeHandler.h"

class CalcThread : public CollectorThread
{
	Q_OBJECT
	
public:

	CalcThread(SupportedHashes::hash_type hType, ByteBuffer* buf, offset_t checksumOffset)
		: CollectorThread(buf), hashType(hType), checksumOff(checksumOffset)
	{
	}

signals:
	void gotHash(QString hash, int type);

private:
	void run();
	QString makeImpHash(PEFile* pe);
	QString makeRichHdrHash(PEFile* pe);

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
		PEFile* pe = m_peHndl->getPe();
		
		ByteBuffer* tmpBuf = new ByteBuffer(pe->getContent(), pe->getContentSize());
		offset_t checksumOffset = m_peHndl->optHdrWrapper.getFieldOffset(OptHdrWrapper::CHECKSUM);
		CalcThread *calcThread = new CalcThread(m_hashType, tmpBuf, checksumOffset);
		ByteBuffer::release(tmpBuf);

		this->myThread = calcThread;
		QObject::connect(calcThread, SIGNAL(gotHash(QString, int)), m_peHndl, SLOT(onHashReady(QString, int)));
		return true;
	}
	
protected:
	SupportedHashes::hash_type m_hashType;
	PeHandler *m_peHndl;
};

///----


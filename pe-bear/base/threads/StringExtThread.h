#pragma once
#include "CollectorThread.h"
#include "../StringsCollection.h"
#include "../PeHandler.h"

class StringExtThread : public CollectorThread
{
	Q_OBJECT

public:
	StringExtThread(ByteBuffer *buf, size_t _minStrLen)
		: CollectorThread(buf), minStrLen(_minStrLen), mapToFill(nullptr)
	{
		mapToFill = new StringsCollection();
	}

	~StringExtThread()
	{
		this->mapToFill->release();
	}
	
signals:
	void gotStrings(StringsCollection* mapToFill);
	void loadingStrings(int progress);
	
private:
	void run();
	size_t extractStrings(StringsCollection &mapToFill, const size_t minStr, const size_t maxStr = 0, bool acceptNonTerminated = true);

	StringsCollection *mapToFill;
	size_t minStrLen;
};

///----

class StringThreadManager : public CollectorThreadManager
{
public:
	StringThreadManager(PeHandler *peHndl, size_t minStrLen)
		: m_peHndl(peHndl), m_minStrLen(minStrLen)
	{
	}
	
	bool setupThread()
	{
		if (!m_peHndl) return false;
		
		PEFile* pe = m_peHndl->getPe();
		if (!pe) return false;

		ByteBuffer* tmpBuf = new ByteBuffer(pe->getContent(), pe->getContentSize());
		StringExtThread *stringThread = new StringExtThread(tmpBuf, m_minStrLen);
		ByteBuffer::release(tmpBuf);

		this->myThread = stringThread;
		QObject::connect(stringThread, SIGNAL(gotStrings(StringsCollection* )), m_peHndl, SLOT(onStringsReady(StringsCollection* )));
		QObject::connect(stringThread, SIGNAL(loadingStrings(int)), m_peHndl, SLOT(onStringsLoadingProgress(int)));
		return true;
	}
protected:
	PeHandler *m_peHndl;
	size_t m_minStrLen;
};

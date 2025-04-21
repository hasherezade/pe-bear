#pragma once
#include "CollectorThread.h"
#include "../StringsCollection.h"
#include "../PeHandler.h"

class StringExtThread : public CollectorThread
{
	Q_OBJECT

public:
	StringExtThread(AbstractByteBuffer *inpBuf, size_t _minStrLen)
		: CollectorThread(inpBuf), minStrLen(_minStrLen), mapToFill(nullptr)
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

		StringExtThread* stringThread = new StringExtThread(pe->getFileBuffer(), m_minStrLen);
		this->myThread = stringThread;
		QObject::connect(stringThread, SIGNAL(gotStrings(StringsCollection*)), m_peHndl, SLOT(onStringsReady(StringsCollection*)));
		QObject::connect(stringThread, SIGNAL(loadingStrings(int)), m_peHndl, SLOT(onStringsLoadingProgress(int)));
		return true;
	}
protected:
	PeHandler *m_peHndl;
	size_t m_minStrLen;
};

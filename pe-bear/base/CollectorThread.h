#pragma once

#include <QtCore>
#include <bearparser/bearparser.h>
#include "StringsCollection.h"

///----

class CollectorThread : public QThread
{
	Q_OBJECT
public:
	CollectorThread(PEFile* pe)
		: m_PE(pe), stopRequested(false)
	{
	}
	
	bool isByteArrInit() { return (m_PE && m_PE->getContent()); }
	
	void stop()
	{
		stopRequested = true;
	}
	
protected:
	PEFile* m_PE;
	QMutex myMutex;
	bool stopRequested;
};

//--- 
class CollectorThreadManager : public QObject
{
	Q_OBJECT

public:
	CollectorThreadManager() : QObject(), myThread(nullptr), isQueued(false)
	{
	}
	
	~CollectorThreadManager()
	{
		deleteThread();
	}
	
	void stopThread()
	{
		if (this->myThread) {
			this->myThread->stop();
		}
	}
	
	void deleteThread()
	{
		if (this->myThread) {
			this->myThread->stop();
			while (myThread->isFinished() == false) {
				myThread->wait();
			}
			delete myThread;
			myThread = nullptr;
		}
	}
	
	bool recreateThread()
	{
		if (this->myThread) {
			this->myThread->stop();
			isQueued = true;
			return false; //previous thread didn't finished
		}
		if (setupThread()) {
			runThread();
			return true;
		}
		return false;
	}
	
	
protected slots:
	bool resetOnFinished()
	{
		if (myThread && myThread->isFinished()) {
			delete myThread;
			myThread = nullptr;
		}
		if (isQueued) {
			if (setupThread()) {
				runThread();
				return true;
			}
		}
		return false;
	}
	
protected:
	virtual bool setupThread() = 0;
	
	virtual void runThread()
	{
		if (!myThread) return;
		QObject::connect(myThread, SIGNAL(finished()), this, SLOT(resetOnFinished()));
		myThread->start();
		this->isQueued = false;
	}
	
	bool isQueued;
	CollectorThread *myThread;
	QMutex myMutex;
};

///----

class CalcThread : public CollectorThread
{
	Q_OBJECT
	
public:
	enum hash_type {
		MD5 = 0,
		SHA1 = 1,
		SHA256,
		CHECKSUM,
		RICH_HDR_MD5,
		IMP_MD5,
		HASHES_NUM
	};

	CalcThread(hash_type hType, PEFile* pe, offset_t checksumOffset)
		: CollectorThread(pe), hashType(hType), checksumOff(checksumOffset)
	{
	}

signals:
	void gotHash(QString hash, int type);

private:
	void run();
	QString makeImpHash();
	QString makeRichHdrHash();

	hash_type hashType;
	offset_t checksumOff;
};

///----

class StringExtThread : public CollectorThread
{
	Q_OBJECT

public:
	StringExtThread(PEFile* pe, size_t _minStrLen)
		: CollectorThread(pe), minStrLen(_minStrLen), mapToFill(nullptr)
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

#include <sig_finder.h>


class MatchesCollection : public QObject, public Releasable
{
	Q_OBJECT

public:
	MatchesCollection() : Releasable() {}
	
	QList<sig_ma::FoundPacker> packerAtOffset;
};

#include <iostream>
class SignFinderThread : public CollectorThread
{
	Q_OBJECT
public:
	SignFinderThread(PEFile* pe, offset_t offset)
		: CollectorThread(pe), startOffset(offset)
	{
	}
	
	~SignFinderThread()
	{
		std::cout << __FUNCTION__ << std::endl;
	}
	
	void setStartOffset(offset_t _startOffset)
	{
		QMutexLocker lock(&myMutex);
		this->startOffset = _startOffset;
	}
	
	sig_ma::SigFinder signFinder;
	QList<sig_ma::FoundPacker> packerAtOffset;
	
signals:
	void gotMatches(SignFinderThread* matched);

private:
	void run();
	void findInBuffer();
	void addFoundPackers(offset_t startingRaw, sig_ma::matched &matchedSet);
	bool findPackerSign(offset_t startingRaw);

	offset_t startOffset;
};

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

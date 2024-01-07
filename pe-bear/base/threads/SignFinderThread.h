#pragma once
#include "CollectorThread.h"
#include <sig_finder.h>
#include "../Releasable.h"

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


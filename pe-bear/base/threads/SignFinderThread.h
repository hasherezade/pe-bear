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
	SignFinderThread(PEFile* pe, sig_ma::SigFinder &signFinder, MatchesCollection &matched, offset_t offset)
		: CollectorThread(pe), 
		m_signFinder(signFinder), m_matched(matched), startOffset(offset)
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

signals:
	void gotMatches(MatchesCollection* matched);
	void progressUpdated(int progress);
	
private:
	void run();
	void findInBuffer();
	void addFoundPackers(offset_t startingRaw, sig_ma::matched &matchedSet);
	bool findPackerSign(offset_t startingRaw);

	offset_t startOffset;
	sig_ma::SigFinder &m_signFinder;
	MatchesCollection &m_matched;
};

class SignFinderThreadManager : public CollectorThreadManager
{
	Q_OBJECT
public:
	SignFinderThreadManager(PEFile* pe, offset_t offset)
		: m_PE(pe), startOffset(0)
	{
	}
	
	void setStartOffset(offset_t _startOffset)
	{
		this->startOffset = _startOffset;
		SignFinderThread *thread = dynamic_cast<SignFinderThread*>(this->myThread);
		if (thread) {
			thread->setStartOffset(_startOffset);
		}
	}
	
	bool setupThread()
	{
		if (!m_PE) return false;
		
		SignFinderThread *thread = new SignFinderThread(m_PE, m_signFinder, m_matched, startOffset);
		this->myThread = thread;

		QObject::connect(thread, SIGNAL(gotMatches(MatchesCollection* )), 
			this, SLOT(onGotMatches(MatchesCollection *)), Qt::UniqueConnection);

		QObject::connect(thread, SIGNAL(progressUpdated(int)), 
			this, SLOT(onProgressUpdated(int)), Qt::UniqueConnection);

		return true;
	}
	
	bool loadSignature(const QString &label, const QString &text)
	{
		m_signFinder.clear();
		return m_signFinder.loadSignature("Searched", text.toStdString());
	}
	
signals:
	void gotMatches(MatchesCollection* matched);
	void progressUpdated(int progress);
	
protected slots:

	void onGotMatches(MatchesCollection* matched)
	{
		emit gotMatches(matched);
	}
	
	void onProgressUpdated(int progress)
	{
		emit progressUpdated(progress);
	}

protected:

	PEFile* m_PE;
	offset_t startOffset;
	sig_ma::SigFinder m_signFinder;
	MatchesCollection m_matched;
};

///----


#pragma once
#include "CollectorThread.h"
#include "../Releasable.h"

#include <sig_finder.h>
using namespace sig_finder;

struct MatchedSign
{
	MatchedSign() : offset(0), len(0) {}
	MatchedSign(size_t _offset, size_t _len)
	: offset(_offset), len(_len) {}
	
	size_t offset;
	size_t len;
};

class MatchesCollection : public QObject, public Releasable
{
	Q_OBJECT

public:
	MatchesCollection() : Releasable() {}
	
	QList<MatchedSign> packerAtOffset;
};

class SignFinderThread : public CollectorThread
{
	Q_OBJECT
public:
	SignFinderThread(ByteBuffer* buf, sig_finder::Node &signFinder, MatchesCollection &matched, offset_t offset)
		: CollectorThread(buf),
		m_signFinder(signFinder), m_matched(matched), startOffset(offset)
	{
	}
	
	~SignFinderThread()
	{
	}
	
	void setStartOffset(offset_t _startOffset)
	{
		QMutexLocker lock(&myMutex);
		this->startOffset = _startOffset;
	}

signals:
	void searchStarted(bool isStarted);
	void gotMatches(MatchesCollection* matched);
	void progressUpdated(int progress);
	
private:
	void run();
	void findInBuffer();
	size_t addFoundPackers(offset_t startingRaw, std::vector<sig_finder::Match> &matchedSet);
	size_t findPackerSign(offset_t startingRaw);

	offset_t startOffset;
	sig_finder::Node &m_signFinder;
	MatchesCollection &m_matched;
};

class SignFinderThreadManager : public CollectorThreadManager
{
	Q_OBJECT
public:
	SignFinderThreadManager(PEFile* pe, offset_t offset=0)
		: m_PE(pe), startOffset(offset)
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
#ifdef SIGN_THREAD
		SignFinderThread *thread = new SignFinderThread(m_PE, m_patternFinder, m_matched, startOffset);
		this->myThread = thread;

		QObject::connect(thread, SIGNAL(gotMatches(MatchesCollection* )), 
			this, SLOT(onGotMatches(MatchesCollection *)), Qt::UniqueConnection);

		QObject::connect(thread, SIGNAL(progressUpdated(int)), 
			this, SLOT(onProgressUpdated(int)), Qt::UniqueConnection);
		
		QObject::connect(thread, SIGNAL(searchStarted(bool)), 
			this, SLOT(onSearchStarted(bool)), Qt::UniqueConnection);
#endif //SIGN_THREAD
		return true;
	}
	
	bool loadSignature(const QString &label, const QString &text)
	{
		m_patternFinder.clear();
		Signature *sign = Signature::loadFromByteStr("Searched", text.toStdString());
		if (!sign) {
			return false;
		}
		bool isOk = m_patternFinder.addPattern(*sign);
		delete sign;
		return isOk;
	}
	
signals:
	void gotMatches(MatchesCollection* matched);
	void progressUpdated(int progress);
	void searchStarted(bool isStarted);

protected slots:

	void onGotMatches(MatchesCollection* matched)
	{
		emit gotMatches(matched);
	}
	
	void onProgressUpdated(int progress)
	{
		emit progressUpdated(progress);
	}
	
	void onSearchStarted(bool isStarted)
	{
		emit searchStarted(isStarted);
	}
	
protected:

	PEFile* m_PE;
	offset_t startOffset;
	sig_finder::Node m_patternFinder;
	MatchesCollection m_matched;
};

///----


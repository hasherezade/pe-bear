#include "SignFinderThread.h"

void SignFinderThread::run()
{
	QMutexLocker lock(&myMutex);
	this->m_matched.packerAtOffset.clear();
	if (!isByteArrInit()) {
		return;
	}
	findInBuffer();
	emit gotMatches(&m_matched);
}

void SignFinderThread::findInBuffer()
{
	using namespace sig_ma;
	if (!m_PE || startOffset == INVALID_ADDR) return;
	
	offset_t offset = startOffset;
	offset_t fullSize = m_PE->getContentSize();
	int proc = 0;
	int maxProgress = 1000;
	bool found = false;

	{ //scope0
		QMutexLocker stopLock(&this->stopMutex);
		//if (this->stopRequested) break;
		if (findPackerSign(offset)) {
			found = true;;
		}
	} //!scope0
	double curr = offset;
	double max = fullSize;
	const int progress = int((curr/max) * maxProgress);
	if (progress > proc) {
		proc = progress;
		emit progressUpdated(proc);
	}
	if (found) return;
	emit progressUpdated(maxProgress);
}

bool SignFinderThread::findPackerSign(offset_t startingRaw)
{
	using namespace sig_ma;
	
	if (!m_PE && startingRaw == INVALID_ADDR) {
		return false;
	}
	BYTE* content = m_PE->getContent();
	if (!content) {
		return false;
	}
	const size_t contentSize = m_PE->getRawSize();
	if (startingRaw >= contentSize) {
		return false;
	}
	std::vector<pattern_tree::Match> matchedSet;
	this->m_signFinder.getMatching(content + startingRaw, contentSize - startingRaw, matchedSet, true);
	if (addFoundPackers(startingRaw, matchedSet)) {
		return true;
	}
	return false;
}

size_t SignFinderThread::addFoundPackers(offset_t startingRaw, std::vector<pattern_tree::Match> &matchedSet)
{
	size_t foundCount = matchedSet.size();
	if (!foundCount) {
		return 0;
	}
	size_t count = 0;
	using namespace pattern_tree;
	for (auto sItr = matchedSet.begin(); sItr != matchedSet.end(); ++sItr) {
		pattern_tree::Match match = *sItr;
		if (!match.sign) {
			continue;
		}
		count++;
		this->m_matched.packerAtOffset.append(MatchedSign(startingRaw + match.offset, match.sign->size()));
	}
	return count;
}

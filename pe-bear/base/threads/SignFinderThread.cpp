#include "SignFinderThread.h"

void SignFinderThread::run()
{
	emit searchStarted(true);
	QMutexLocker lock(&myMutex);
	this->m_matched.packerAtOffset.clear();
	if (!isByteArrInit()) {
		return;
	}
	findInBuffer();
	emit gotMatches(&m_matched);
	emit searchStarted(false);
}

void SignFinderThread::findInBuffer()
{
	if (!m_PE || startOffset == INVALID_ADDR) return;
	
	offset_t offset = startOffset;
	offset_t fullSize = m_PE->getContentSize();
	int proc = 0;
	int maxProgress = 1000;
	bool found = false;
	offset_t currOffset = offset;
	{ //scope0
		QMutexLocker stopLock(&this->stopMutex);
		if (this->stopRequested) return;
		size_t processed = findPackerSign(offset);
		currOffset += processed;

	} //!scope0
	
	if (currOffset == (fullSize - 1)) {
		emit progressUpdated(maxProgress);
		return;
	}
	const double max = fullSize;
	const double curr = currOffset;
	const int progress = int((curr/max) * maxProgress);
	if (progress > proc) {
		proc = progress;
		emit progressUpdated(proc);
	}
}

size_t SignFinderThread::findPackerSign(offset_t startingRaw)
{
	if (!m_PE && startingRaw == INVALID_ADDR) {
		return 0;
	}
	BYTE* content = m_PE->getContent();
	if (!content) {
		return 0;
	}
	const size_t contentSize = m_PE->getRawSize();
	if (startingRaw >= contentSize) {
		return 0;
	}
	std::vector<sig_finder::Match> matchedSet;
	size_t processed = this->m_signFinder.getMatching(content + startingRaw, contentSize - startingRaw, matchedSet, true);
	addFoundPackers(startingRaw, matchedSet);
	return processed;
}

size_t SignFinderThread::addFoundPackers(offset_t startingRaw, std::vector<sig_finder::Match> &matchedSet)
{
	size_t foundCount = matchedSet.size();
	if (!foundCount) {
		return 0;
	}
	size_t count = 0;
	for (auto sItr = matchedSet.begin(); sItr != matchedSet.end(); ++sItr) {
		sig_finder::Match match = *sItr;
		if (!match.sign) {
			continue;
		}
		count++;
		this->m_matched.packerAtOffset.append(MatchedSign(startingRaw + match.offset, match.sign->size()));
	}
	return count;
}

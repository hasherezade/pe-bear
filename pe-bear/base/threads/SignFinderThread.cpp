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
	for (offset = startOffset; offset < fullSize; offset++) {
		{ //scope0
			QMutexLocker stopLock(&this->stopMutex);
			if (this->stopRequested) break;
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
	}
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
	sig_ma::matched matchedSet = m_signFinder.getMatching(content, contentSize, startingRaw, sig_ma::FIXED);
	addFoundPackers(startingRaw, matchedSet);
	return matchedSet.signs.size() ? true : false;
}


void SignFinderThread::addFoundPackers(offset_t startingRaw, sig_ma::matched &matchedSet)
{
	size_t foundCount = matchedSet.signs.size();
	if (!foundCount) {
		return;
	}
	using namespace sig_ma;
	PckrSign* packer = nullptr;
	for (auto sItr = matchedSet.signs.begin(); sItr != matchedSet.signs.end(); ++sItr) {
		packer = *sItr;
		if (!packer) continue;
		
		FoundPacker pckr(startingRaw + matchedSet.match_offset, packer);
		auto itr = std::find(this->m_matched.packerAtOffset.begin(), this->m_matched.packerAtOffset.end(), pckr);

		if (itr != this->m_matched.packerAtOffset.end()) { //already exist
			FoundPacker &found = *itr;
			packer = found.signaturePtr;
		} else {
			this->m_matched.packerAtOffset.append(pckr);
		}
	}
}


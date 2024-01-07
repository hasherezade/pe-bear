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
	size_t fullSize = m_PE->getContentSize();

	for (offset = startOffset; offset < fullSize; offset++) {
		if (findPackerSign(offset)) {
			break;
		}
	}
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
	using namespace sig_ma;
	
	size_t foundCount = matchedSet.signs.size();
	if (!foundCount) return;
	
	std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
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


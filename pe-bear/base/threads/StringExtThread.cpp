#include "StringExtThread.h"

size_t StringExtThread::extractStrings(StringsCollection &mapToFill, const size_t minStr, const size_t maxStr, bool acceptNonTerminated)
{
	if (!m_PE) return 0;

	int progress = 0;
	emit loadingStrings(progress);
	
	offset_t step = 0;
	size_t maxSize = m_PE->getContentSize();
	for (step = 0; step < maxSize; step++) {

		{ //scope0
			QMutexLocker stopLock(&this->stopMutex);
			if (this->stopRequested) break;
			
			bool isWide = false;
			char *ptr = (char*) m_PE->getContentAt(step, 1);
			if (!ptr || !IS_PRINTABLE(*ptr) || isspace(*ptr) ) {
				continue;
			}
			const size_t remainingSize = maxSize - step;
			const size_t maxLen = (maxStr != 0 && maxStr < remainingSize) ? maxStr : remainingSize;
			QString str = m_PE->getStringValue(step, maxLen, acceptNonTerminated);
			if (str.length() == 1) {
				isWide = true;
				str = m_PE->getWAsciiStringValue(step, maxLen / 2, acceptNonTerminated);
			}
			if (!str.length() || str.length() < minStr) {
				continue;
			}
			mapToFill.insert(step, str, isWide);
			step += util::getStringSize(str, isWide);
			step--;
		} //!scope0
		
		int proc = int(((float)step / (float)maxSize) * 100);
		if ((proc - progress) > 1) {
			progress = proc;
			emit loadingStrings(progress);
		}

	}
	return mapToFill.size();
}

void StringExtThread::run()
{
	QMutexLocker lock(&myMutex);

	this->mapToFill->clear();
	
	if (!isByteArrInit()) {
		emit gotStrings(nullptr);
		return;
	}
	const size_t minLen = this->minStrLen > 2 ? this->minStrLen : 2;
	extractStrings(*mapToFill, minLen, 0, true);
	if (!this->stopRequested) {
		// emit strings only if the thread finished
		emit gotStrings(mapToFill);
	}
}

//-------------------------------------------------


#include "StringExtThread.h"

size_t StringExtThread::extractStrings(StringsCollection &mapToFill, const size_t minStr, const size_t maxStr, bool acceptNonTerminated)
{
	if (!m_buf) return 0;

	int progress = 0;
	emit loadingStrings(progress);
	
	const size_t contentSize = m_buf->getContentSize();
	size_t maxSize = contentSize;

	// remove null padding at the end
	for (; maxSize > 0; maxSize--) {
		QMutexLocker stopLock(&this->stopMutex);
		if (this->stopRequested) break;

		const char* ptr = (char*)m_buf->getContentAt((maxSize - 1), 1);
		const char nextC = ptr ? (*ptr) : 0;
		if (nextC != 0) break;
	}

	for (offset_t step = 0; step < maxSize; step++) {

		{ //scope0
			QMutexLocker stopLock(&this->stopMutex);
			if (this->stopRequested) break;
			
			bool isWide = false;
			const char *ptr = (char*)m_buf->getContentAt(step, 1);
			const char nextC = ptr ? (*ptr) : 0;
			if (!nextC || !IS_PRINTABLE(nextC) || isspace(nextC) ) {
				continue;
			}
			const size_t remainingSize = maxSize - step;
			const size_t maxLen = (maxStr != 0 && maxStr < remainingSize) ? maxStr : remainingSize;
			QString str = m_buf->getStringValue(step, maxLen, acceptNonTerminated);
			if (str.length() == 1) {
				isWide = true;
				str = m_buf->getWAsciiStringValue(step, maxLen / 2, acceptNonTerminated);
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


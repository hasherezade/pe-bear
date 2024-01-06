#include "CollectorThread.h"


inline QString stripExtension(const QString & fileName)
{
    return fileName.left(fileName.lastIndexOf("."));
}

QString CalcThread::makeImpHash()
{
	static CommonOrdinalsLookup lookup;
	ImportDirWrapper* imports = m_PE->getImports();
	if (!imports) return QString();

	QStringList exts;
	exts.append(".ocx");
	exts.append(".sys");
	exts.append(".dll");

	const size_t librariesCount = imports->getEntriesCount();

	QList<offset_t> thunks = imports->getThunksList();
	const size_t functionsCount = thunks.size();

	QStringList impsBlock;
	for (int i = 0; i < thunks.size(); i++) {
		offset_t thunk = thunks[i];
		if (thunk == 0 || thunk == INVALID_ADDR) continue;

		QString lib =  imports->thunkToLibName(thunk).toLower();
		for (QStringList::iterator itr = exts.begin(); itr != exts.end(); ++itr) {
			if (lib.endsWith(*itr)) {
				lib = stripExtension(lib);
				break;
			}
		}
		ImportBaseFuncWrapper* func = imports->thunkToFunction(thunk);
		if (!func) continue;
		QString funcName;
		if (!func->isByOrdinal()) {
			funcName = func->getShortName();
		} else {
			int ord = func->getOrdinal();
			funcName = lookup.findFuncName(lib, ord);
			if (!funcName.length()) {
				funcName = "ord" + QString::number(ord, 10);
			}
		}
		impsBlock << QString(lib + "." + funcName.toLower());
	}

	const QString allImps = impsBlock.join(",");
	//std::cout << allImps.toStdString() << "\n";
	QCryptographicHash calcHash(QCryptographicHash::Md5);
	calcHash.addData((char*) allImps.toStdString().c_str(), allImps.length());
	return QString(calcHash.result().toHex());
}

QString CalcThread::makeRichHdrHash()
{
	pe::RICH_SIGNATURE* sign = m_PE->getRichHeaderSign();
	pe::RICH_DANS_HEADER* dans = NULL;
	if (sign) {
		dans = m_PE->getRichHeaderBgn(sign);
	}
	if (!dans) {
		return QString();
	}
	const size_t diff = (ULONGLONG)sign - (ULONGLONG)dans;
	ByteBuffer tmpBuf((BYTE*)dans, diff, true);
	DWORD* dw_ptr = (DWORD*)tmpBuf.getContent();
	size_t dw_size = tmpBuf.getContentSize() / sizeof(DWORD);
	for (int i = 0; i < dw_size; i++) {
		dw_ptr[i] ^= sign->checksum;
	}
	QCryptographicHash calcHash(QCryptographicHash::Md5);
	calcHash.addData((char*) tmpBuf.getContent(), tmpBuf.getContentSize());
	return QString(calcHash.result().toHex());
}

void CalcThread::run()
{
	QMutexLocker lock(&myMutex);
	if (this->stopRequested) {
		return;
	}
	QString fileHash = "Cannot calculate!";
	if (!m_PE || !m_PE->getContent()) {
		emit gotHash(fileHash, hashType);
		return;
	}
	QCryptographicHash::Algorithm qHashType = QCryptographicHash::Md5;
	if (hashType == MD5) {
		qHashType = QCryptographicHash::Md5;
	} else if (hashType == SHA1) {
		qHashType = QCryptographicHash::Sha1;
	} 
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) //the feature was introduced in Qt5.0
	else if (hashType == SHA256) {
		qHashType = QCryptographicHash::Sha256;
	}
#endif
	try {
		if (hashType == CHECKSUM) {
			long checksum = PEFile::computeChecksum((BYTE*) m_PE->getContent(), m_PE->getContentSize(), checksumOff);
			fileHash = QString::number(checksum, 16);
		}
		else if (hashType == RICH_HDR_MD5) {
			fileHash = makeRichHdrHash();
		}
		else if (hashType == IMP_MD5) {
			fileHash = makeImpHash();
		}
		else {
			QCryptographicHash calcHash(qHashType);
			calcHash.addData((char*) m_PE->getContent(), m_PE->getContentSize());
			fileHash = QString(calcHash.result().toHex());
		}
	} catch (...) {
		fileHash = "Cannot calculate!";
	}
	emit gotHash(fileHash, hashType);
}

//-------------------------------------------------

size_t StringExtThread::extractStrings(StringsCollection &mapToFill, const size_t minStr, const size_t maxStr, bool acceptNonTerminated)
{
	if (!m_PE) return 0;

	int progress = 0;
	emit loadingStrings(progress);
	
	offset_t step = 0;
	size_t maxSize = m_PE->getContentSize();
	for (step = 0; step < maxSize && !this->stopRequested; step++) {
		bool isWide = false;
		char *ptr = (char*) m_PE->getContentAt(step, 1);
		if (!IS_PRINTABLE(*ptr) || isspace(*ptr) ) {
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

void SignFinderThread::run()
{
	QMutexLocker lock(&myMutex);
	this->packerAtOffset.clear();
	if (!isByteArrInit()) {
		return;
	}
	findInBuffer();
	emit gotMatches(this);
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
	sig_ma::matched matchedSet = signFinder.getMatching(content, contentSize, startingRaw, sig_ma::FIXED);
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
		auto itr = std::find(this->packerAtOffset.begin(), this->packerAtOffset.end(), pckr);

		if (itr != this->packerAtOffset.end()) { //already exist
			FoundPacker &found = *itr;
			packer = found.signaturePtr;
		} else {
			this->packerAtOffset.append(pckr);
		}
	}
}

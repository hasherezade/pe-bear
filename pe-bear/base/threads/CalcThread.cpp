#include "CalcThread.h"

//--- 
inline QString stripExtension(const QString & fileName)
{
	return fileName.left(fileName.lastIndexOf("."));
}

QString CalcThread::makeImpHash(PEFile* pe)
{
	static CommonOrdinalsLookup lookup;
	ImportDirWrapper* imports = pe->getImportsDir();
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
	calcHash.addData((const char*) allImps.toStdString().c_str(), allImps.length());
	return QString(calcHash.result().toHex());
}

QString CalcThread::makeRichHdrHash(PEFile* pe)
{
	if (!pe) return QString();
	pe::RICH_SIGNATURE* sign = pe->getRichHeaderSign();
	pe::RICH_DANS_HEADER* dans = nullptr;
	if (sign) {
		dans = pe->getRichHeaderBgn(sign);
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
	for (int i = 0; i < SupportedHashes::HASHES_NUM; i++) {
		SupportedHashes::hash_type hashType = static_cast<SupportedHashes::hash_type>(i);
		emit gotHash("Calculating...", hashType);
	}

	QMutexLocker lock(&myMutex);
	if (this->isStopRequested()) {
		return;
	}
	const QString hashFailed = "Cannot calculate!";
	QString fileHash = hashFailed;

	if (!m_buf || !m_buf->getContent()) {
		for (int i = 0; i < SupportedHashes::HASHES_NUM; i++) {
			SupportedHashes::hash_type hashType = static_cast<SupportedHashes::hash_type>(i);
			emit gotHash(fileHash, hashType);
		}
	}
	// calculate all types:
	for (int i = 0; i < SupportedHashes::HASHES_NUM; i++) {
		if (this->isStopRequested()) {
			return;
		}
		const SupportedHashes::hash_type hashType = static_cast<SupportedHashes::hash_type>(i);
		fileHash = hashFailed;

		QCryptographicHash::Algorithm qHashType = QCryptographicHash::Md5;
		if (hashType == SupportedHashes::MD5) {
			qHashType = QCryptographicHash::Md5;
		}
		else if (hashType == SupportedHashes::SHA1) {
			qHashType = QCryptographicHash::Sha1;
		}
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) //the feature was introduced in Qt5.0
		else if (hashType == SupportedHashes::SHA256) {
			qHashType = QCryptographicHash::Sha256;
		}
#endif
		try {
			BYTE* buf = m_buf->getContent();
			size_t bufSize = m_buf->getContentSize();

			if (hashType == SupportedHashes::CHECKSUM) {
				ulong checksum = PEFile::computeChecksum((BYTE*)buf, bufSize, checksumOff);
				fileHash = QString::number(checksum, 16);
			}
			else if (hashType == SupportedHashes::RICH_HDR_MD5) {
				PEFile pe(m_buf);
				fileHash = makeRichHdrHash(&pe);
			}
			else if (hashType == SupportedHashes::IMP_MD5) {
				PEFile pe(m_buf);
				fileHash = makeImpHash(&pe);
			}
			else {
				QCryptographicHash calcHash(qHashType);
				for (size_t i = 0; i < bufSize; i++) {
					if (this->isStopRequested()) {
						return;
					}
					calcHash.addData((char*)buf + i, 1);
				}
				fileHash = QString(calcHash.result().toHex());
			}
		}
		catch (...) {
			fileHash = hashFailed;
		}
		emit gotHash(fileHash, hashType);
	}
}


#include "ClipboardUtil.h"
#include "../PEBear.h"

int ClipboardUtil::byteArrayToBuffer(QByteArray bytes, uchar *buf, int bufSize)
{
	int size = bytes.size();

	int clipIndx = 0;
	for (; clipIndx < size && clipIndx < bufSize; clipIndx++) {
		uchar b = bytes.at(clipIndx);
		if (buf) buf[clipIndx] = b;
	}
	return clipIndx;
}

QByteArray ClipboardUtil::getTextClip()
{
	QString text = QApplication::clipboard()->text();
#if QT_VERSION >= 0x050000
	return text.toLatin1();
#else
	return text.toAscii();
#endif
}

QByteArray ClipboardUtil::getHexClip()
{
	QString text = QApplication::clipboard()->text();
	return parseBytesString(text, " ");
}

QByteArray ClipboardUtil::getOctetStreamClip()
{
	const QMimeData *mimeData = QApplication::clipboard()->mimeData();
	return mimeData->data("application/octet-stream");
}

int ClipboardUtil::getFromClipboard(uchar *buf, int bufSize)
{
	QByteArray bytes = getOctetStreamClip();
	if (bytes.size() == 0 ) { //no octet stream provided, try to parse text...
		bytes = getHexClip();
		if (bytes.size() == 0) bytes = getTextClip();
	}
	if (bytes.size() == 0) return 0;
	return byteArrayToBuffer(bytes, buf, bufSize);
}

int ClipboardUtil::getFromClipboard(bool isHex, uchar *buf, int bufSize)
{
	QByteArray bytes = getOctetStreamClip();
	if (bytes.size() == 0 ) { //no octet stream provided, try to parse text...
		bytes = (isHex) ? getHexClip() : getTextClip();
	}
	if (bytes.size() == 0) return 0;
	return byteArrayToBuffer(bytes, buf, bufSize);
}

QByteArray ClipboardUtil::parseBytesString(QString text, QString separator)
{
	QByteArray emptyArr;
	if (text.length() == 0) return emptyArr;

	QStringList chunks = text.split(separator, QT_SkipEmptyParts);
	int size = chunks.size();
	
	QByteArray bytes;
	//validate
	for (int i = 0; i < size; i++ ) {
		QString chunk = chunks[i];
		if (chunk.size() != 2) {
			return emptyArr; // validation failed
		}

#if QT_VERSION >= 0x050000
		char c = chunk[0].toLatin1();
#else
		char c = chunk[0].toAscii();
#endif
		if (!pe_util::isHexChar(c)) {
			return emptyArr; // validation failed
		}
		//validateion ok!
		bool isOk;
		uchar b = chunks[i].toShort(&isOk, 16);
		if (!isOk) return emptyArr; 
		bytes.append(b);
	}
	return bytes;
}


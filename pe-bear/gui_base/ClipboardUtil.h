#pragma once

#include <QtGlobal>
#include <QtCore>
#include <QClipboard>

#if QT_VERSION >= 0x050000
	#include <QApplication>
#endif

#include <bearparser/bearparser.h>

class ClipboardUtil 
{
public: 
	/* Fills given buffer; in: allocated buffer of size bufSize; out: filledSize */
	static int byteArrayToBuffer(QByteArray inArr, uchar *outBuf, int outBufSize);

	static int getFromClipboard(uchar *buf, int bufSize);
	static int getFromClipboard(bool isHex, uchar *buf, int bufSize);

	static inline QByteArray getTextClip();
	static inline QByteArray getHexClip();
	static inline QByteArray getOctetStreamClip();

	static QByteArray parseBytesString(QString string, QString separator);
//----
};

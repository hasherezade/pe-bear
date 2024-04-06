#include "HexMimeSource.h"


HexMimeSource::HexMimeSource(BYTE *buf, uint64_t bufSize)
{
	this->buf = buf;
	this->bufSize = bufSize;
}

bool HexMimeSource::provides(const char* typeName) const
{
	if (strstr(typeName,"application/octet-stream")) {
		return true;
	}
	if (strstr(typeName,"text/plain")) {
		return true;
	}
	return false;
}

QByteArray HexMimeSource::encodedData(const char* typeName) const
{
	QByteArray arr;
	if (this->buf == NULL || this->bufSize == 0) return arr;

	if (strstr(typeName,"application/octet-stream")) {
		for (int i = 0; i < bufSize; i++) {
			arr.append(buf[i]);
		}
		return arr;
	}
	if (strstr(typeName,"text/plain")) {
		for (int i = 0; i < bufSize; i++) {
			static char charVal[4];
			memset(charVal,0,sizeof(charVal));
			snprintf(charVal, sizeof(charVal) - 1, "%02X", buf[i]);
			arr.append(charVal);
			if (i < (bufSize - 1)) arr.append(' ');
		}
		return arr;
	}
	return arr;
}

const char* HexMimeSource::format(int n) const
{
	if (n == 0) return "application/octet-stream";
	if (n == 1) return "text/plain";
	return NULL;
}

//--------------------------------------------------------------------
/*
HexConverter::HexConverter(QObject *parent)
    : QObject(parent)
{
	validator = new QRegularExpressionValidator(QRegularExpression("[0-9A-Fa-f]{1,8}"), this);
}

QString HexConverter::textFromValue(int value) const
{
	return QString::number(value, 16).toUpper();
}

int HexConverter::valueFromText(const QString &text) const
{
	bool ok;
	int val = text.toInt(&ok, 16);
	if (ok) return val;
	return 0;
}

bool HexConverter::validate(QString &text, int pos) const
{
	QValidator::State state = validator->validate(text, pos);
	//Invalid, Intermediate,Acceptable
	if (state == QValidator::Acceptable) return true;
	return false;
}
*/
//--------------------------------------------------------------------
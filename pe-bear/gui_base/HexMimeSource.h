#pragma once

#include <QtGui>
#include "../Util.h"

class HexMimeSource : public QMimeSource
{
public:
	HexMimeSource(BYTE *buf, uint64_t bufSize);

	virtual const char* format(int n = 0) const;
	virtual bool provides(const char*) const;
	virtual QByteArray encodedData(const char*) const;

protected:
	BYTE *buf;
	uint64_t bufSize;
};

//--------------------------------------------------------------------
/*class HexConverter: public QObject
{
	Q_OBJECT
public:
	HexConverter(QObject *parent = 0);

	bool validate(QString &text, int pos = 0) const;
	int valueFromText(const QString &text) const;
	QString textFromValue(int value) const;

private:
	QRegExpValidator *validator;
};
*/
//--------------------------------------------------------------------

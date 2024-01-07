#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

class HexSpinBox : public QSpinBox
{
	Q_OBJECT
public:
	HexSpinBox(QWidget *parent = 0);

protected:
	QValidator::State validate(QString &text, int &pos) const;
	int valueFromText(const QString &text) const;
	QString textFromValue(int value) const;

private:
	QRegExpValidator *validator;
};


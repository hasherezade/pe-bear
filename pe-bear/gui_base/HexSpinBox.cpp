#include "HexSpinBox.h"

HexSpinBox::HexSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
	validator = new QRegularExpressionValidator(QRegularExpression("0x[0-9A-Fa-f]{1,8}"), this);
	setPrefix("0x");
}

QString HexSpinBox::textFromValue(int value) const
{
	return QString::number(value, 16).toUpper();
}

int HexSpinBox::valueFromText(const QString &text) const
{
	bool ok;
	int val = text.toInt(&ok, 16);
	if (ok) return val;
	return 0;
}

QValidator::State HexSpinBox::validate(QString &text, int &pos) const
{
	return validator->validate(text, pos);
}


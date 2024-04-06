#pragma once
#include <QtGlobal>

#include "../QtCompat.h"

class HexInputDialog : public QDialog
{
    Q_OBJECT

public:
	HexInputDialog(QString title, QString caption, QWidget *parent = 0);

	~HexInputDialog()
	{
		delete validator;
		delete le;
		delete captionLabel;
		delete layout_middleBox;
		delete vbox;
	}

	qulonglong getNumValue(bool *isValid = NULL);
	void setDefaultValue(qulonglong number);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	void setRegex(const QRegularExpression &regex) { validator->setRegularExpression(regex); }
#else
	void setRegex(const QRegularExpression &regex) { validator->setRegExp(regex); }
#endif

	void setMaxLength(int len) { le->setMaxLength(len); }

public slots:
	void rejected();
	void accepted();

protected:

	QVBoxLayout *vbox, *layout_middleBox;
	QLabel *captionLabel;
	QLineEdit *le;
	QRegularExpressionValidator *validator;
};

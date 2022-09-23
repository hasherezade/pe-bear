#pragma once
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

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

	void setRegex(const QRegExp &regex) { validator->setRegExp(regex); }
	void setMaxLength(int len) { le->setMaxLength(len); }

public slots:
	void rejected();
	void accepted();

protected:

	QVBoxLayout *vbox, *layout_middleBox;
	QLabel *captionLabel;
	QLineEdit *le;
	QRegExpValidator *validator;
};

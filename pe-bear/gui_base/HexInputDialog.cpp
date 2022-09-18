#include "HexInputDialog.h"
#include <QtGlobal>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

HexInputDialog::HexInputDialog(QString title, QString caption, QWidget *parent)
	: QDialog(parent), le(NULL)
{
	this->setModal(false);
	this->setWindowTitle(title);

	layout_middleBox = new QVBoxLayout();

	vbox = new QVBoxLayout();
	this->captionLabel = new QLabel(caption);
	layout_middleBox->addWidget(this->captionLabel);

	le = new QLineEdit(this);
	layout_middleBox->addWidget(le);

	validator = new QRegExpValidator(QRegExp("[0-9A-Fa-f]{1,}"));
	le->setValidator(validator);
	vbox->addLayout(layout_middleBox);

	QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	vbox->addWidget(buttonBox);
	this->setLayout(vbox);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accepted()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(rejected()));
}

long long HexInputDialog::getNumValue(bool *isValid)
{
	bool is_converted = false;
	QString textVal = le->text();
	long long value = textVal.toLongLong(&is_converted, 16);

	if (isValid != nullptr) {
		(*isValid) = is_converted;
	}
	return value;
}

void HexInputDialog::setDefaultValue(long long number)
{
	le->setText(QString::number(number, 16).toUpper());
}

void HexInputDialog::accepted()
{
	bool isValid = false;
	getNumValue(&isValid);
	
	if (isValid == false) {
		QMessageBox::warning(this, "Warning!", "Wrong number format supplied!");
		return;
	}
	QDialog::accept();
}

void HexInputDialog::rejected()
{
	QDialog::reject();
}

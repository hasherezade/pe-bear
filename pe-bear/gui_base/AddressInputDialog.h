#pragma once

#include <QtGui>

#include "HexInputDialog.h" 
#include <bearparser/Executable.h>

#include "../base/MainSettings.h"

class AddressInputDialog : public HexInputDialog
{
	Q_OBJECT
protected slots:
	void onAddrTypeChanged();
	void onAddrChanged();

public:
	AddressInputDialog(Executable *exe, bool isRaw, ColorSettings &addrColors, QWidget *parent);
	~AddressInputDialog()
	{
		delete otherEdit;
		delete otherCaptionLabel;
		delete cbox_isRva;
	}

	Executable::addr_type getAddrType();

protected:
	offset_t convertToOther(offset_t val, Executable::addr_type aT);
	void validateAddr();
	void setTextColor();
	void setDescriptions();

	Executable *myExe;
	ColorSettings &addrColors;
	
	QCheckBox *cbox_isRva;
	QLabel *otherCaptionLabel;
	QLineEdit *otherEdit;
	
	bool isRawAddr;
};


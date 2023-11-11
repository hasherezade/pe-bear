#include "MainSettings.h"

#include "../gui/DarkStyle.h"


//--------------------


QString ColorSettings::defaultRawColor = "green";
QString ColorSettings::defaultRvaColor = "blue";
QString ColorSettings::defaultVaColor = "purple";

QString ColorSettings::defaultFlagsColor = "#E3E4FA"; // lavender
QString ColorSettings::defaultDataDirColor = "#e3e4fa"; // lavender
QString ColorSettings::defaultDataDirNameColor = "#4169e1"; // RoyalBlue

QString ColorSettings::defaultStyle = QString() + 
	"ColorSettings { qproperty-rawColor: " + ColorSettings::defaultRawColor
	+ "; qproperty-rvaColor: " + ColorSettings::defaultRvaColor
	+ "; qproperty-vaColor: " + ColorSettings::defaultVaColor
	+ "; qproperty-flagsColor: " + ColorSettings::defaultFlagsColor
	+ "; qproperty-dataDirColor: " + ColorSettings::defaultDataDirColor
	+ "; qproperty-dataDirNameColor: " + ColorSettings::defaultDataDirNameColor
	+ "; }";

QColor ColorSettings::addrTypeToColor(const Executable::addr_type &aT) const
{
	QColor addrColor = QColor();
	switch (aT) {
		case Executable::RAW:
			addrColor = rawColor();
			break;
		case Executable::RVA:
			addrColor = rvaColor();
			break;
		case Executable::VA:
			addrColor = vaColor();
			break;
	}
	return addrColor;
}

//--------------------

t_reload_mode intToReloadMode(int val)
{
	if (val < 0 || val >= RELOAD_MODES_COUNT) {
		return RELOAD_ASK;
	}
	return (t_reload_mode) val;
}

////

const QString MainSettings::languageDir = "Language";

bool MainSettings::readPersistent()
{
	QSettings settings(COMPANY_NAME, APP_NAME);
	this->autoSaveTags = settings.value("AutoSaveTags", true).toBool();
	this->autoReloadOnFileChange = intToReloadMode(settings.value("AutoReloadOnChage", RELOAD_ASK).toInt());
	this->uDataDir = settings.value("UDD", "").toString();
	this->followOnClick = settings.value("FollowOnClick", false).toBool();
	this->lExePath = settings.value("LastOpened", false).toString();
	this->dirDump = settings.value("LastDumpDir", false).toString();
	this->language = settings.value("language", false).toString();
	if (settings.status() != QSettings::NoError ) {
		return false;
	}
	return true;
}

bool MainSettings::writePersistent()
{
	QSettings settings(COMPANY_NAME, APP_NAME);
	settings.setValue("UDD", uDataDir);
	settings.setValue("AutoSaveTags", autoSaveTags);
	settings.setValue("AutoReloadOnChage", autoReloadOnFileChange);
	settings.setValue("FollowOnClick", followOnClick);
	settings.setValue("LastOpened", this->lExePath);
	settings.setValue("LastDumpDir", this->dirDump);
	settings.setValue("language", this->language);
	if ( settings.status() == QSettings::NoError ) {
		return true;
	}
	return false;
}
//--------------------

bool writeFontProperties(QSettings &settings, QFont &font, QString propertyName)
{
	settings.setValue(propertyName, font.toString());
	settings.setValue(propertyName + ".size", font.pointSize());
	
	if ( settings.status() == QSettings::NoError ) {
		return true;
	}
	return false;
}

QFont readFontProperties(QSettings &settings, QString propertyName, QFont defaultFont)
{
	const QString defaultFontString = defaultFont.toString();
	const int defaultFontSize = defaultFont.pointSize();

	QString fontStr = settings.value(propertyName, defaultFontString).toString();
	if (settings.status() != QSettings::NoError ) {
		return defaultFont;
	}
	QFont font(fontStr);
	font.fromString(fontStr);
	font.setPointSize(settings.value(propertyName + ".size", defaultFontSize).toInt());
	if (settings.status() != QSettings::NoError ) {
		return defaultFont;
	}
	return font;
}

bool GuiSettings::readPersistent()
{
	QSettings settings(COMPANY_NAME, APP_NAME);

	QFont gFont = readFontProperties(settings, "globalFont", this->defaultGlobalFont);
	QFont hFont =  readFontProperties(settings, "hexFont", HexViewSettings::defaultFont());
	QFont dFont = readFontProperties(settings, "disasmFont", DisasmViewSettings::defaultFont());
	QString styleName = settings.value("style", "").toString();

	this->_setGlobalFont(gFont);
	this->_setDisasmViewFont(dFont);
	this->_setHexViewFont(hFont);

	this->setStyleByName(styleName);

	emit globalFontChanged();
	emit hexViewSettingsChanged(hexVSettings);
	emit disasmViewSettingsChanged(disasmVSettings);

	if (settings.status() != QSettings::NoError ) {
		return false;
	}
	return true;
}

bool GuiSettings::writePersistent()
{
	QSettings settings(COMPANY_NAME, APP_NAME);
	
	QFont currFont = QApplication::font();
	writeFontProperties(settings, currFont, "globalFont");
	writeFontProperties(settings, this->hexVSettings.myFont, "hexFont");
	writeFontProperties(settings, this->disasmVSettings.myFont, "disasmFont");
	
	// save the style:
	settings.setValue("style", this->currentStyle);
	
	if ( settings.status() == QSettings::NoError ) {
		return true;
	}
	return false;
}

void GuiSettings::initStyles()
{
	this->defaultStylesheet = qApp->styleSheet();
	this->defaultStyleName = tr("*System Default*");

	nameToStyle[this->defaultStyleName] = this->defaultStylesheet;
	nameToStyle["Dark"] = g_DarkStyle;
}

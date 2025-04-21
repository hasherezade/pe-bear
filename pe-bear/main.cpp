#include <iostream>
#include <QtGlobal>
#include <QtCore>
#include <QApplication>
#include <QTranslator>

#include "QtCompat.h"
#include "gui/windows/MainWindow.h"
#include "base/MainSettings.h"

QStringList collectSuppliedFiles()
{
	QStringList args = QCoreApplication::arguments();
	QStringList fNames;
	for (int i = 1; i < args.length(); i++) {
		fNames << args[i];
	}
	return fNames;
}

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(application);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

	QApplication app(argc, argv);
	// workaround for a bug in Qt (not setting default font properly)
	QApplication::setFont(QApplication::font("QMessageBox"));

	// Load the settings
	MainSettings mainSettings;
	mainSettings.readPersistent();

	// Load language file
	QTranslator translator;
	QString currLanguage = mainSettings.language;
	if (currLanguage.length() == 0) {
		currLanguage = QLocale::system().name();
	}
	QString trPath = QDir::separator() + MainSettings::languageDir + QDir::separator() + currLanguage + QDir::separator() + "PELanguage.qm";
	if (translator.load(QCoreApplication::applicationDirPath() + trPath) 
		|| translator.load(mainSettings.userDataDir() + trPath))
	{
		app.installTranslator(&translator);
		mainSettings.language = currLanguage;
	} else {
		mainSettings.language = ""; //reset to default
	}

	app.setApplicationName(TITLE);
	app.setWindowIcon(QIcon(":/main_ico.ico"));
	app.setQuitOnLastWindowClosed(true);

	MainWindow mainWin(mainSettings);
	mainWin.setIconSize(QSize(48, 48));
	mainWin.resize(950, 650);

	QStringList fileNames = collectSuppliedFiles();
	if (fileNames.length()) {
		mainWin.openMultiplePEs(fileNames);
	}
	mainWin.show();
	int ret = app.exec();
	return ret;
}


#include <iostream>
#include <QtGlobal>
#include <QtCore>

#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#else
	#include <QtGui>
#endif

#include "gui/windows/MainWindow.h"

QStringList collectSuppliedFiles(int argc, char *argv[])
{
	QStringList fNames;
	if (argc <= 1) {
		return fNames;
	}
	for (int i = 1; i < argc; i++) {
		QString fileName = argv[i];
		fNames << fileName;
	}
	return fNames;
}

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(application);

	QApplication app(argc, argv);

	// workaround for a bug in Qt (not setting default font properly)
	QApplication::setFont(QApplication::font("QMessageBox"));

	app.setApplicationName(TITLE);
	app.setWindowIcon(QIcon(":/main_ico.ico"));
	app.setQuitOnLastWindowClosed(true);

	MainWindow mainWin;
	mainWin.setIconSize(QSize(48, 48));
	mainWin.resize(950, 650);

	if (argc > 1) {
		QStringList fileNames = collectSuppliedFiles(argc, argv);
		mainWin.openMultiplePEs(fileNames);
	}
	mainWin.show();
	int ret = app.exec();
	return ret;
}


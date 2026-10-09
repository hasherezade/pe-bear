#pragma once

#include "../../QtCompat.h"
#include "../UpdateChecker.h"
#include "../ReleaseClient.h"
#include "UpdateDialog.h"

/**
 * Connects the update check to the application: owns the checker and the
 * dialog, runs the once-a-day background check after the main window is up,
 * and shows the dialog when there is something worth showing.
 */
class UpdateCoordinator : public QObject
{
	Q_OBJECT
public:
	/** Delay before the automatic check, so startup stays unaffected. */
	static const int AUTO_CHECK_DELAY_MS = 5000;

	/** @param settings borrowed updater settings, owned by MainSettings */
	UpdateCoordinator(pe_bear::updater::UpdateSettings *settings, QWidget *parentWindow);

	pe_bear::updater::UpdateChecker* checker() { return m_checker; }

public slots:
	/** Called once the main window is visible. Never blocks startup. */
	void onApplicationReady();
	/** Help -> Check for Updates. */
	void checkManually();

private slots:
	void onAutoCheckTimeout();
	void onStateChanged(int state);

private:
	pe_bear::updater::UpdateSettings *m_settings;
	pe_bear::updater::UpdateChecker *m_checker;
	UpdateDialog *m_dialog;
	bool m_started;
};

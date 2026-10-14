#include "UpdateCoordinator.h"

using namespace pe_bear::updater;

const int UpdateCoordinator::AUTO_CHECK_DELAY_MS;

UpdateCoordinator::UpdateCoordinator(UpdateSettings *settings, QWidget *parentWindow)
	: QObject(parentWindow), m_settings(settings), m_checker(NULL), m_dialog(NULL), m_started(false)
{
	m_checker = new UpdateChecker(new ReleaseClient(), m_settings, this);
	m_dialog = new UpdateDialog(m_checker, parentWindow);
	connect(m_checker, SIGNAL(stateChanged(int)), this, SLOT(onStateChanged(int)));
}

void UpdateCoordinator::onApplicationReady()
{
	if (m_started) return;
	m_started = true;
	if (!m_settings || !m_settings->isAutoCheckEnabled()) return;

	/* Deliberately late and asynchronous: no network work happens on the path
	   between launching PE-bear and having a usable window, and a machine with
	   no connectivity notices nothing at all. */
	QTimer::singleShot(AUTO_CHECK_DELAY_MS, this, SLOT(onAutoCheckTimeout()));
}

void UpdateCoordinator::onAutoCheckTimeout()
{
	if (!m_checker) return;
	m_checker->checkForUpdatesIfDue();
}

void UpdateCoordinator::checkManually()
{
	if (!m_checker) return;
	m_dialog->present();
	/* A manual check ignores both the 24 h interval and a skipped version. */
	m_checker->checkForUpdates(true);
}

void UpdateCoordinator::onStateChanged(int state)
{
	/* An automatic check stays out of the way until it has something worth
	   interrupting for. Failures and "up to date" are silent unless the user
	   asked, in which case the dialog is already open. */
	if (static_cast<UpdateState>(state) == StateUpdateAvailable) {
		if (m_dialog && !m_dialog->isVisible()) {
			m_dialog->present();
		}
	}
}

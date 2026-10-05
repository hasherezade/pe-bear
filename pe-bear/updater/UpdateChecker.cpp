#include "UpdateChecker.h"

using namespace pe_bear::updater;

UpdateChecker::UpdateChecker(IReleaseSource *source, UpdateSettings *settings, QObject *parent)
	: QObject(parent), m_source(source), m_settings(settings),
	m_state(StateIdle), m_lastError(ErrorNone), m_userInitiated(false)
{
	qRegisterMetaType<pe_bear::updater::ReleaseInfo>("pe_bear::updater::ReleaseInfo");
	if (m_source) {
		m_source->setParent(this);
		connect(m_source, SIGNAL(releaseReady(pe_bear::updater::ReleaseInfo)),
			this, SLOT(onReleaseReady(pe_bear::updater::ReleaseInfo)));
		connect(m_source, SIGNAL(failed(int, QString)), this, SLOT(onSourceFailed(int, QString)));
	}
}

void UpdateChecker::setState(UpdateState state)
{
	if (m_state == state) return;
	m_state = state;
	emit stateChanged(static_cast<int>(state));
}

void UpdateChecker::failWith(UpdateError error, const QString &detail)
{
	m_lastError = error;
	m_lastErrorDetail = detail;
	/* A failed check must never get in the way of using PE-bear: it records
	   the reason, returns to rest, and that is all. */
	setState((error == ErrorCancelled) ? StateIdle : StateFailed);
	emit errorOccurred(static_cast<int>(error), detail);
}

void UpdateChecker::checkForUpdatesIfDue()
{
	if (m_settings && !m_settings->isAutomaticCheckDue(QDateTime::currentDateTime())) return;
	checkForUpdates(false);
}

void UpdateChecker::checkForUpdates(bool userInitiated)
{
	if (isBusy()) return;
	if (!m_source) {
		failWith(ErrorNetwork, QLatin1String("no release source is configured"));
		return;
	}
	m_userInitiated = userInitiated;
	m_lastError = ErrorNone;
	m_lastErrorDetail.clear();
	m_release = ReleaseInfo();
	setState(StateChecking);
	m_source->fetchLatest();
}

void UpdateChecker::onSourceFailed(int error, const QString &detail)
{
	failWith(static_cast<UpdateError>(error), detail);
}

void UpdateChecker::onReleaseReady(const pe_bear::updater::ReleaseInfo &release)
{
	if (m_state != StateChecking) return;
	if (m_settings) {
		m_settings->setLastCheck(QDateTime::currentDateTime());
	}
	if (!release.isValid()) {
		failWith(ErrorInvalidResponse, QLatin1String("the release metadata is incomplete"));
		return;
	}

	if (!(release.version > Version::current())) {
		setState(StateUpToDate);
		emit upToDate();
		return;
	}
	/* A skipped version stays silent for automatic checks, but a user who
	   asked explicitly always gets an answer. */
	if (!m_userInitiated && m_settings && m_settings->isVersionSkipped(release.version)) {
		setState(StateUpToDate);
		emit upToDate();
		return;
	}

	m_release = release;
	setState(StateUpdateAvailable);
	emit updateAvailable(m_release);
}

void UpdateChecker::skipCurrentVersion()
{
	if (m_state != StateUpdateAvailable || !m_settings) return;
	m_settings->setSkippedVersion(m_release.version);
	m_release = ReleaseInfo();
	setState(StateIdle);
}

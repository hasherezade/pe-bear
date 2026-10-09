#pragma once

#include <QtCore>
#include "UpdateTypes.h"
#include "ReleaseClient.h"
#include "UpdateSettings.h"

namespace pe_bear {
namespace updater {

/**
 * Asks the release source for the latest stable release and compares it with
 * the running build. That is all it does: nothing is downloaded, nothing is
 * installed, and the result is a state the UI can show.
 *
 * Takes any IReleaseSource so that the logic -- skipped versions, "no newer
 * than mine", failures that must not get in the way -- is tested without a
 * network.
 */
class UpdateChecker : public QObject
{
	Q_OBJECT
public:
	/**
	 * @param source   takes ownership (re-parented)
	 * @param settings borrowed; may be NULL, in which case every check is due
	 *                 and no version is ever skipped
	 */
	UpdateChecker(IReleaseSource *source, UpdateSettings *settings, QObject *parent = NULL);

	UpdateState state() const { return m_state; }
	UpdateError lastError() const { return m_lastError; }
	QString lastErrorDetail() const { return m_lastErrorDetail; }
	/** The newer release, valid only in StateUpdateAvailable. */
	ReleaseInfo release() const { return m_release; }
	bool isBusy() const { return m_state == StateChecking; }

public slots:
	/**
	 * @param userInitiated true for Help -> Check for Updates: a version the
	 *        user asked not to be reminded about is still reported then, and
	 *        so is "up to date".
	 */
	void checkForUpdates(bool userInitiated);
	/** The automatic check: only when the settings say it is due. */
	void checkForUpdatesIfDue();
	/** Stop offering the release currently shown; later ones are still offered. */
	void skipCurrentVersion();

signals:
	void stateChanged(int state);
	void updateAvailable(const pe_bear::updater::ReleaseInfo &release);
	void upToDate();
	void errorOccurred(int error, const QString &detail);

private slots:
	void onReleaseReady(const pe_bear::updater::ReleaseInfo &release);
	void onSourceFailed(int error, const QString &detail);

private:
	void setState(UpdateState state);
	void failWith(UpdateError error, const QString &detail);

	IReleaseSource *m_source;
	UpdateSettings *m_settings;
	UpdateState m_state;
	UpdateError m_lastError;
	QString m_lastErrorDetail;
	ReleaseInfo m_release;
	bool m_userInitiated;
};

}; // namespace updater
}; // namespace pe_bear

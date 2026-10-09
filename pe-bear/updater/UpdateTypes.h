#pragma once

#include <QtCore>
#include "Version.h"

namespace pe_bear {
namespace updater {

/** States of the update check. Nothing here downloads or installs. */
enum UpdateState {
	StateIdle = 0,
	StateChecking,
	StateUpToDate,
	StateUpdateAvailable,
	StateFailed,
	UPDATE_STATES_COUNT
};

enum UpdateError {
	ErrorNone = 0,
	ErrorNetwork,
	ErrorTimeout,
	ErrorRateLimited,
	ErrorTls,
	ErrorForbiddenRedirect,
	ErrorInvalidResponse,
	ErrorResponseTooLarge,
	ErrorNoStableRelease,
	ErrorInvalidVersion,
	ErrorCancelled,
	UPDATE_ERRORS_COUNT
};

QString updateStateToString(UpdateState s);
QString updateErrorToString(UpdateError e);
/** Human-readable, translated message for the given error. */
QString updateErrorMessage(UpdateError e);

//----------------------------------------------------------------------

/** One downloadable file of a release, as reported by the GitHub API. */
struct ReleaseAsset
{
	ReleaseAsset() : size(0), digestMalformed(false) {}

	QString name;
	QUrl downloadUrl;
	qint64 size;
	/** Lowercase 64-hex SHA-256, without the "sha256:" prefix; empty when not published. */
	QString sha256;
	/** True when a digest was published but could not be parsed. */
	bool digestMalformed;

	bool hasDigest() const { return sha256.length() == 64; }
	bool isValid() const { return !name.isEmpty() && downloadUrl.isValid() && size > 0; }
};

/** A validated stable release. */
struct ReleaseInfo
{
	ReleaseInfo() {}

	QString tagName;
	Version version;
	QUrl htmlUrl;
	QString publishedAt;
	QList<ReleaseAsset> assets;

	bool isValid() const { return version.isValid() && !tagName.isEmpty(); }
};

}; // namespace updater
}; // namespace pe_bear

Q_DECLARE_METATYPE(pe_bear::updater::ReleaseAsset)
Q_DECLARE_METATYPE(pe_bear::updater::ReleaseInfo)

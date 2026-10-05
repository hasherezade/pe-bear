#pragma once

#include <QtCore>
#include <QtNetwork>
#include "UpdateTypes.h"

namespace pe_bear {
namespace updater {

/**
 * Source of release metadata. Abstract so that the update state machine can be
 * driven by a stub in tests without touching the network.
 */
class IReleaseSource : public QObject
{
	Q_OBJECT

public:
	explicit IReleaseSource(QObject *parent = NULL) : QObject(parent) {}
	virtual ~IReleaseSource() {}

	virtual void fetchLatest() = 0;
	virtual void cancel() = 0;
	virtual bool isBusy() const = 0;

signals:
	void releaseReady(const pe_bear::updater::ReleaseInfo &release);
	void failed(int error, const QString &detail);
};

//----------------------------------------------------------------------

/**
 * Fetches the latest stable release from the GitHub Releases API.
 *
 * Everything about the exchange is bounded: the response size, the connection
 * and overall timeouts, the number of retries, the hosts a redirect may lead
 * to, and the shape of the JSON that is accepted. TLS errors are fatal and are
 * never ignored. The request carries no information about the user or about
 * anything they are analysing -- only the application version, in User-Agent.
 */
class ReleaseClient : public IReleaseSource
{
	Q_OBJECT

public:
	/** Repository whose releases are offered, as "owner/name". */
	static const char* DEFAULT_REPOSITORY;

	static const qint64 MAX_RESPONSE_BYTES = 2 * 1024 * 1024;
	static const int CONNECT_TIMEOUT_MS = 10 * 1000;
	static const int OVERALL_TIMEOUT_MS = 30 * 1000;
	static const int MAX_RETRIES = 2;

	/** Hosts a release request or its redirects may legitimately reach. */
	static QStringList allowedApiHosts();
	/** Hosts an asset download may legitimately reach. */
	static QStringList allowedDownloadHosts();
	static bool isAllowedHost(const QUrl &url, const QStringList &allowed);

	/** Builds the API URL for the latest release of @p repository. */
	static QUrl latestReleaseUrl(const QString &repository);

	/**
	 * Builds the outgoing request. Public so a test can assert that no
	 * identifying header is ever attached.
	 */
	static QNetworkRequest buildRequest(const QUrl &url);

	/**
	 * Parses and validates a "releases/latest" payload. Pure function: no
	 * network, no clock, no filesystem.
	 *
	 * Drafts and prereleases are rejected outright. Assets whose download URL
	 * points somewhere unexpected are dropped. A malformed digest is recorded
	 * on the asset rather than failing the whole release, so that one bad asset
	 * cannot hide a good one.
	 */
	static bool parseLatestRelease(const QByteArray &payload, ReleaseInfo &release,
		UpdateError &error, QString &detail);

	/** Parses "sha256:<64 hex>". Returns the lowercase hex, or empty on failure. */
	static QString parseSha256Digest(const QString &raw, bool &malformed);

	explicit ReleaseClient(QObject *parent = NULL);
	ReleaseClient(const QString &repository, QObject *parent);
	virtual ~ReleaseClient();

	virtual void fetchLatest();
	virtual void cancel();
	virtual bool isBusy() const { return m_reply != NULL; }

	QString repository() const { return m_repository; }

private slots:
	void onReadyRead();
	void onFinished();
	void onSslErrors(const QList<QSslError> &errors);
	void onRedirected(const QUrl &url);
	void onOverallTimeout();

private:
	void startRequest();
	void abortWith(UpdateError error, const QString &detail);
	void teardown();
	bool shouldRetry(UpdateError error) const;

	QString m_repository;
	QNetworkAccessManager m_manager;
	QNetworkReply *m_reply;
	QByteArray m_buffer;
	QTimer m_overallTimer;
	int m_attempt;
	bool m_cancelled;
};

}; // namespace updater
}; // namespace pe_bear

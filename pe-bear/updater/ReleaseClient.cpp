#include "ReleaseClient.h"
#include "Version.h"

using namespace pe_bear::updater;

#ifdef PEBEAR_UPDATE_REPOSITORY
const char* ReleaseClient::DEFAULT_REPOSITORY = PEBEAR_UPDATE_REPOSITORY;
#else
const char* ReleaseClient::DEFAULT_REPOSITORY = "hasherezade/pe-bear";
#endif

/* Definitions for the in-class initialised constants above, needed wherever
   they are odr-used (for example by QCOMPARE, which takes a reference). */
const qint64 ReleaseClient::MAX_RESPONSE_BYTES;
const int ReleaseClient::CONNECT_TIMEOUT_MS;
const int ReleaseClient::OVERALL_TIMEOUT_MS;
const int ReleaseClient::MAX_RETRIES;

namespace {

	const char* GITHUB_API_HOST = "api.github.com";
	const char* GITHUB_ACCEPT = "application/vnd.github+json";
	const char* GITHUB_API_VERSION = "2022-11-28";
	const char* SHA256_PREFIX = "sha256:";

	bool isLowerHex(const QString &s)
	{
		for (int i = 0; i < s.length(); i++) {
			const QChar c = s.at(i);
			const bool digit = (c >= QLatin1Char('0') && c <= QLatin1Char('9'));
			const bool hex = (c >= QLatin1Char('a') && c <= QLatin1Char('f'));
			if (!digit && !hex) return false;
		}
		return true;
	}

}; // namespace

QStringList ReleaseClient::allowedApiHosts()
{
	QStringList hosts;
	hosts << QLatin1String(GITHUB_API_HOST);
	return hosts;
}

QStringList ReleaseClient::allowedDownloadHosts()
{
	QStringList hosts;
	hosts << QLatin1String("github.com")
		<< QLatin1String("objects.githubusercontent.com")
		<< QLatin1String("release-assets.githubusercontent.com")
		<< QLatin1String(GITHUB_API_HOST);
	return hosts;
}

bool ReleaseClient::isAllowedHost(const QUrl &url, const QStringList &allowed)
{
	if (!url.isValid()) return false;
	/* plain HTTP would take the digest off an untrusted channel */
	if (url.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) != 0) {
		return false;
	}
	const QString host = url.host().toLower();
	if (host.isEmpty()) return false;

	QStringList::const_iterator itr;
	for (itr = allowed.begin(); itr != allowed.end(); ++itr) {
		if (host == (*itr).toLower()) return true;
	}
	return false;
}

QUrl ReleaseClient::latestReleaseUrl(const QString &repository)
{
	return QUrl(QLatin1String("https://") + QLatin1String(GITHUB_API_HOST)
		+ QLatin1String("/repos/") + repository + QLatin1String("/releases/latest"));
}

QNetworkRequest ReleaseClient::buildRequest(const QUrl &url)
{
	QNetworkRequest request(url);
	request.setRawHeader("Accept", GITHUB_ACCEPT);
	request.setRawHeader("X-GitHub-Api-Version", GITHUB_API_VERSION);
	/* The only thing we disclose is which build is asking. No user name, no
	   machine name, nothing about the files being analysed. */
	const QByteArray agent = QByteArray("PE-bear/") + Version::current().toString().toLatin1();
	request.setRawHeader("User-Agent", agent);

#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
	/* Follow redirects, but never from https down to http. */
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::NoLessSafeRedirectPolicy);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	request.setTransferTimeout(CONNECT_TIMEOUT_MS);
#endif

	QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
	ssl.setPeerVerifyMode(QSslSocket::VerifyPeer);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	ssl.setProtocol(QSsl::TlsV1_2OrLater);
#endif
	request.setSslConfiguration(ssl);
	return request;
}

QString ReleaseClient::parseSha256Digest(const QString &raw, bool &malformed)
{
	malformed = false;
	const QString trimmed = raw.trimmed();
	if (trimmed.isEmpty()) {
		return QString();
	}
	const QString prefix = QLatin1String(SHA256_PREFIX);
	if (!trimmed.startsWith(prefix, Qt::CaseInsensitive)) {
		/* a digest in some other algorithm, or a shape we do not understand */
		malformed = true;
		return QString();
	}
	const QString hex = trimmed.mid(prefix.length()).toLower();
	if (hex.length() != 64 || !isLowerHex(hex)) {
		malformed = true;
		return QString();
	}
	return hex;
}

bool ReleaseClient::parseLatestRelease(const QByteArray &payload, ReleaseInfo &release,
	UpdateError &error, QString &detail)
{
	error = ErrorNone;
	detail.clear();
	release = ReleaseInfo();

	if (payload.size() > MAX_RESPONSE_BYTES) {
		error = ErrorResponseTooLarge;
		detail = QLatin1String("payload exceeds the response limit");
		return false;
	}

	QJsonParseError parseError;
	const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
	if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
		error = ErrorInvalidResponse;
		detail = QLatin1String("response is not a JSON object");
		return false;
	}
	const QJsonObject root = doc.object();

	if (root.value(QLatin1String("draft")).toBool(false)) {
		error = ErrorNoStableRelease;
		detail = QLatin1String("latest release is a draft");
		return false;
	}
	if (root.value(QLatin1String("prerelease")).toBool(false)) {
		error = ErrorNoStableRelease;
		detail = QLatin1String("latest release is a prerelease");
		return false;
	}

	const QJsonValue tagValue = root.value(QLatin1String("tag_name"));
	if (!tagValue.isString()) {
		error = ErrorInvalidResponse;
		detail = QLatin1String("tag_name is missing");
		return false;
	}
	const QString tag = tagValue.toString().trimmed();
	if (tag.isEmpty()) {
		error = ErrorInvalidResponse;
		detail = QLatin1String("tag_name is empty");
		return false;
	}
	const Version version = Version::fromString(tag);
	if (!version.isValid()) {
		error = ErrorInvalidVersion;
		detail = QLatin1String("tag '") + tag + QLatin1String("' is not a stable version");
		return false;
	}

	release.tagName = tag;
	release.version = version;
	release.publishedAt = root.value(QLatin1String("published_at")).toString();

	const QUrl htmlUrl(root.value(QLatin1String("html_url")).toString());
	if (isAllowedHost(htmlUrl, allowedDownloadHosts())) {
		release.htmlUrl = htmlUrl;
	}

	const QJsonValue assetsValue = root.value(QLatin1String("assets"));
	if (!assetsValue.isArray()) {
		error = ErrorInvalidResponse;
		detail = QLatin1String("assets is missing");
		return false;
	}
	const QJsonArray assets = assetsValue.toArray();
	const QStringList downloadHosts = allowedDownloadHosts();

	for (int i = 0; i < assets.size(); i++) {
		if (!assets.at(i).isObject()) continue;
		const QJsonObject obj = assets.at(i).toObject();

		ReleaseAsset asset;
		asset.name = obj.value(QLatin1String("name")).toString().trimmed();
		if (asset.name.isEmpty()) continue;
		/* a name that could escape a directory has no business here */
		if (asset.name.contains(QLatin1Char('/')) || asset.name.contains(QLatin1Char('\\'))
			|| asset.name.contains(QLatin1String("..")))
		{
			continue;
		}

		const QJsonValue sizeValue = obj.value(QLatin1String("size"));
		if (!sizeValue.isDouble()) continue;
		asset.size = static_cast<qint64>(sizeValue.toDouble());
		if (asset.size <= 0) continue;

		const QUrl downloadUrl(obj.value(QLatin1String("browser_download_url")).toString());
		if (!isAllowedHost(downloadUrl, downloadHosts)) continue;
		asset.downloadUrl = downloadUrl;

		bool malformed = false;
		asset.sha256 = parseSha256Digest(obj.value(QLatin1String("digest")).toString(), malformed);
		asset.digestMalformed = malformed;

		release.assets.append(asset);
	}

	if (release.assets.isEmpty()) {
		error = ErrorInvalidResponse;
		detail = QLatin1String("release publishes no usable assets");
		return false;
	}
	return true;
}

//----------------------------------------------------------------------

ReleaseClient::ReleaseClient(QObject *parent)
	: IReleaseSource(parent), m_repository(QLatin1String(DEFAULT_REPOSITORY)),
	m_manager(this), m_reply(NULL), m_overallTimer(this), m_attempt(0), m_cancelled(false)
{
	m_overallTimer.setSingleShot(true);
	connect(&m_overallTimer, SIGNAL(timeout()), this, SLOT(onOverallTimeout()));
}

ReleaseClient::ReleaseClient(const QString &repository, QObject *parent)
	: IReleaseSource(parent), m_repository(repository),
	m_manager(this), m_reply(NULL), m_overallTimer(this), m_attempt(0), m_cancelled(false)
{
	m_overallTimer.setSingleShot(true);
	connect(&m_overallTimer, SIGNAL(timeout()), this, SLOT(onOverallTimeout()));
}

ReleaseClient::~ReleaseClient()
{
	teardown();
}

void ReleaseClient::fetchLatest()
{
	if (isBusy()) return;
	m_attempt = 0;
	m_cancelled = false;
	startRequest();
}

void ReleaseClient::startRequest()
{
	m_buffer.clear();

	const QUrl url = latestReleaseUrl(m_repository);
	if (!isAllowedHost(url, allowedApiHosts())) {
		emit failed(ErrorForbiddenRedirect, QLatin1String("refusing to contact an unexpected host"));
		return;
	}
	m_reply = m_manager.get(buildRequest(url));
	if (!m_reply) {
		emit failed(ErrorNetwork, QLatin1String("request could not be started"));
		return;
	}
	connect(m_reply, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
	connect(m_reply, SIGNAL(finished()), this, SLOT(onFinished()));
	connect(m_reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(onSslErrors(QList<QSslError>)));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	connect(m_reply, SIGNAL(redirected(QUrl)), this, SLOT(onRedirected(QUrl)));
#endif
	m_overallTimer.start(OVERALL_TIMEOUT_MS);
}

void ReleaseClient::cancel()
{
	if (!isBusy()) return;
	m_cancelled = true;
	QNetworkReply *reply = m_reply;
	teardown();
	if (reply) {
		reply->abort();
		reply->deleteLater();
	}
	emit failed(ErrorCancelled, QString());
}

void ReleaseClient::teardown()
{
	m_overallTimer.stop();
	if (m_reply) {
		m_reply->disconnect(this);
		m_reply = NULL;
	}
	m_buffer.clear();
}

void ReleaseClient::onReadyRead()
{
	if (!m_reply) return;
	m_buffer.append(m_reply->readAll());
	if (m_buffer.size() > MAX_RESPONSE_BYTES) {
		abortWith(ErrorResponseTooLarge, QLatin1String("response exceeded the size limit"));
	}
}

void ReleaseClient::onSslErrors(const QList<QSslError> &errors)
{
	/* Never ignoreSslErrors(): a certificate problem ends the exchange. */
	QString detail;
	if (!errors.isEmpty()) {
		detail = errors.first().errorString();
	}
	abortWith(ErrorTls, detail);
}

void ReleaseClient::onRedirected(const QUrl &url)
{
	if (!isAllowedHost(url, allowedApiHosts()) && !isAllowedHost(url, allowedDownloadHosts())) {
		abortWith(ErrorForbiddenRedirect,
			QLatin1String("redirect to an unexpected host was refused"));
	}
}

void ReleaseClient::onOverallTimeout()
{
	abortWith(ErrorTimeout, QLatin1String("the request did not complete in time"));
}

void ReleaseClient::abortWith(UpdateError error, const QString &detail)
{
	if (!m_reply) return;
	QNetworkReply *reply = m_reply;
	teardown();
	reply->abort();
	reply->deleteLater();

	if (shouldRetry(error)) {
		m_attempt++;
		startRequest();
		return;
	}
	emit failed(error, detail);
}

bool ReleaseClient::shouldRetry(UpdateError error) const
{
	if (m_cancelled) return false;
	if (m_attempt >= MAX_RETRIES) return false;
	/* Only transient transport problems are worth repeating. A rate limit, a
	   TLS failure or a bad payload will not fix itself on a second try. */
	return (error == ErrorNetwork || error == ErrorTimeout);
}

void ReleaseClient::onFinished()
{
	QNetworkReply *reply = m_reply;
	if (!reply) return;

	m_buffer.append(reply->readAll());
	const QByteArray payload = m_buffer;
	const QNetworkReply::NetworkError netError = reply->error();
	const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	const QByteArray rateRemaining = reply->rawHeader("X-RateLimit-Remaining");
	const QString netErrorString = reply->errorString();
	const QUrl finalUrl = reply->url();

	teardown();
	reply->deleteLater();

	if (m_cancelled) {
		return;
	}
	if (!isAllowedHost(finalUrl, allowedApiHosts()) && !isAllowedHost(finalUrl, allowedDownloadHosts())) {
		emit failed(ErrorForbiddenRedirect, QLatin1String("response came from an unexpected host"));
		return;
	}
	if (status == 403 || status == 429) {
		const bool exhausted = (rateRemaining == QByteArray("0"));
		emit failed(ErrorRateLimited, exhausted
			? QLatin1String("the API rate limit is exhausted")
			: QLatin1String("the API refused the request"));
		return;
	}
	if (netError != QNetworkReply::NoError) {
		const UpdateError mapped = (netError == QNetworkReply::TimeoutError
			|| netError == QNetworkReply::OperationCanceledError)
			? ErrorTimeout : ErrorNetwork;
		if (shouldRetry(mapped)) {
			m_attempt++;
			startRequest();
			return;
		}
		emit failed(mapped, netErrorString);
		return;
	}
	if (status != 200) {
		emit failed(ErrorInvalidResponse,
			QLatin1String("unexpected HTTP status ") + QString::number(status));
		return;
	}

	ReleaseInfo release;
	UpdateError error = ErrorNone;
	QString detail;
	if (!parseLatestRelease(payload, release, error, detail)) {
		emit failed(error, detail);
		return;
	}
	emit releaseReady(release);
}

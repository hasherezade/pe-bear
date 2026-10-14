#include "UpdateDialog.h"
#include "../../REbear.h"

using namespace pe_bear::updater;

UpdateDialog::UpdateDialog(UpdateChecker *checker, QWidget *parent)
	: QDialog(parent), m_checker(checker)
{
	setWindowTitle(tr("Updates"));
	setMinimumWidth(420);
	setLayout(&m_topLayout);

	QFont headlineFont = m_headline.font();
	headlineFont.setBold(true);
	m_headline.setFont(headlineFont);
	m_versions.setTextFormat(Qt::PlainText);
	m_status.setWordWrap(true);
	m_status.setTextFormat(Qt::PlainText);

	m_openPageButton.setText(tr("Open the release page"));
	m_skipButton.setText(tr("Skip this version"));
	m_closeButton.setText(tr("Close"));
	connect(&m_openPageButton, SIGNAL(clicked()), this, SLOT(onOpenPageClicked()));
	connect(&m_skipButton, SIGNAL(clicked()), this, SLOT(onSkipClicked()));
	connect(&m_closeButton, SIGNAL(clicked()), this, SLOT(hide()));

	QHBoxLayout *buttons = new QHBoxLayout();
	buttons->addWidget(&m_skipButton);
	buttons->addStretch();
	buttons->addWidget(&m_closeButton);
	buttons->addWidget(&m_openPageButton);

	m_topLayout.addWidget(&m_headline);
	m_topLayout.addWidget(&m_versions);
	m_topLayout.addWidget(&m_status);
	m_topLayout.addStretch();
	m_topLayout.addLayout(buttons);

	if (m_checker) {
		connect(m_checker, SIGNAL(stateChanged(int)), this, SLOT(onStateChanged(int)));
		connect(m_checker, SIGNAL(errorOccurred(int, QString)),
			this, SLOT(onErrorOccurred(int, QString)));
	}
	refresh();
}

void UpdateDialog::present()
{
	refresh();
	show();
	raise();
	activateWindow();
}

void UpdateDialog::setStatus(const QString &text, bool isError)
{
	m_status.setText(text);
	m_status.setStyleSheet(isError ? QLatin1String("color: " ERR_COLOR ";") : QString());
}

void UpdateDialog::onStateChanged(int)
{
	refresh();
}

void UpdateDialog::onErrorOccurred(int error, const QString &detail)
{
	QString text = updateErrorMessage(static_cast<UpdateError>(error));
	if (!detail.isEmpty()) {
		text += QLatin1String("\n") + detail;
	}
	setStatus(text, true);
}

void UpdateDialog::refresh()
{
	if (!m_checker) return;
	const UpdateState state = m_checker->state();
	const ReleaseInfo release = m_checker->release();
	const Version current = Version::current();

	m_versions.setText(tr("Installed version: ") + current.toString());
	if (release.isValid()) {
		m_versions.setText(tr("Installed version: ") + current.toString()
			+ QLatin1String("\n") + tr("Available version: ") + release.version.toString());
	}
	const bool available = (state == StateUpdateAvailable) && release.isValid();
	m_openPageButton.setVisible(available && release.htmlUrl.isValid());
	m_skipButton.setVisible(available);

	switch (state) {
		case StateIdle:
			m_headline.setText(tr("Updates"));
			setStatus(QString());
			break;
		case StateChecking:
			m_headline.setText(tr("Checking for updates..."));
			setStatus(QString());
			break;
		case StateUpToDate:
			m_headline.setText(tr("PE-bear is up to date."));
			setStatus(QString());
			break;
		case StateUpdateAvailable:
			m_headline.setText(tr("A new version of PE-bear is available."));
			setStatus(tr("PE-bear does not download or install anything itself: the release page lists the packages."));
			break;
		case StateFailed:
			m_headline.setText(tr("The update check did not succeed."));
			onErrorOccurred(static_cast<int>(m_checker->lastError()), m_checker->lastErrorDetail());
			break;
		default:
			break;
	}
}

void UpdateDialog::onOpenPageClicked()
{
	if (!m_checker) return;
	const QUrl url = m_checker->release().htmlUrl;
	/* The only URL ever opened is the one the API returned for the release,
	   and the client has already refused anything not on github.com. */
	if (url.isValid() && url.scheme() == QLatin1String("https")) {
		QDesktopServices::openUrl(url);
	}
}

void UpdateDialog::onSkipClicked()
{
	if (!m_checker) return;
	const QMessageBox::StandardButton answer = QMessageBox::question(this,
		tr("Skip this version"),
		tr("PE-bear will stop offering this version.") + QLatin1String("\n")
		+ tr("You will still be told about later ones."),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (answer != QMessageBox::Yes) return;
	m_checker->skipCurrentVersion();
	hide();
}

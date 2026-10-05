#pragma once

#include "../../QtCompat.h"
#include "../UpdateChecker.h"

/**
 * The single place where the user sees the result of an update check.
 *
 * It only reflects the checker's state. The one thing it can do beyond that
 * is open the release page in the browser; nothing is downloaded or installed
 * by PE-bear itself.
 */
class UpdateDialog : public QDialog
{
	Q_OBJECT
public:
	UpdateDialog(pe_bear::updater::UpdateChecker *checker, QWidget *parent = 0);

	/** Shows the dialog and brings it forward. */
	void present();

protected slots:
	void onStateChanged(int state);
	void onErrorOccurred(int error, const QString &detail);

private slots:
	void onOpenPageClicked();
	void onSkipClicked();

private:
	void refresh();
	void setStatus(const QString &text, bool isError = false);

	pe_bear::updater::UpdateChecker *m_checker;

	QVBoxLayout m_topLayout;
	QLabel m_headline;
	QLabel m_versions;
	QLabel m_status;
	QPushButton m_openPageButton;
	QPushButton m_skipButton;
	QPushButton m_closeButton;
};

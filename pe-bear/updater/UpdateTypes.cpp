#include "UpdateTypes.h"

using namespace pe_bear::updater;

QString pe_bear::updater::updateStateToString(UpdateState s)
{
	switch (s) {
		case StateIdle: return QLatin1String("Idle");
		case StateChecking: return QLatin1String("Checking");
		case StateUpToDate: return QLatin1String("UpToDate");
		case StateUpdateAvailable: return QLatin1String("UpdateAvailable");
		case StateFailed: return QLatin1String("Failed");
		default: return QLatin1String("Invalid");
	}
}

QString pe_bear::updater::updateErrorToString(UpdateError e)
{
	switch (e) {
		case ErrorNone: return QLatin1String("None");
		case ErrorNetwork: return QLatin1String("Network");
		case ErrorTimeout: return QLatin1String("Timeout");
		case ErrorRateLimited: return QLatin1String("RateLimited");
		case ErrorTls: return QLatin1String("Tls");
		case ErrorForbiddenRedirect: return QLatin1String("ForbiddenRedirect");
		case ErrorInvalidResponse: return QLatin1String("InvalidResponse");
		case ErrorResponseTooLarge: return QLatin1String("ResponseTooLarge");
		case ErrorNoStableRelease: return QLatin1String("NoStableRelease");
		case ErrorInvalidVersion: return QLatin1String("InvalidVersion");
		case ErrorCancelled: return QLatin1String("Cancelled");
		default: return QLatin1String("Invalid");
	}
}

QString pe_bear::updater::updateErrorMessage(UpdateError e)
{
	switch (e) {
		case ErrorNone:
			return QString();
		case ErrorNetwork:
			return QCoreApplication::translate("Updater", "Could not reach the update server.");
		case ErrorTimeout:
			return QCoreApplication::translate("Updater", "The update server did not respond in time.");
		case ErrorRateLimited:
			return QCoreApplication::translate("Updater", "The update server is rate-limiting requests. Try again later.");
		case ErrorTls:
			return QCoreApplication::translate("Updater", "The secure connection to the update server could not be established.");
		case ErrorForbiddenRedirect:
			return QCoreApplication::translate("Updater", "The update server redirected to an unexpected host.");
		case ErrorInvalidResponse:
			return QCoreApplication::translate("Updater", "The update server returned an unexpected response.");
		case ErrorResponseTooLarge:
			return QCoreApplication::translate("Updater", "The response from the update server was too large.");
		case ErrorNoStableRelease:
			return QCoreApplication::translate("Updater", "No stable release was found.");
		case ErrorInvalidVersion:
			return QCoreApplication::translate("Updater", "The release version could not be interpreted.");
		case ErrorCancelled:
			return QCoreApplication::translate("Updater", "Cancelled.");
		default:
			return QCoreApplication::translate("Updater", "Unknown error.");
	}
}

#pragma once

#include "UnitTest.h"

class UpdateCheckerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseValidManifest();
    void _parseRejectsBadJsonAndMissingFields();
    void _parseRejectsHttpPackageUrl();
    void _isNewerThan();
    void _packageForPlatform();
    void _shouldPrompt();
    void _installerDestinationPathUsesBasename();
    void _checkOnStartupEmptyUrlDoesNothing();
    void _checkOnStartupHttpUrlDoesNothing();
    void _applyNewerAndroidSetsDownload();
    void _applyNewerWithoutPlatformHidesDownload();
    void _maybeShowDialogRespectsFlyingAndDismiss();
};

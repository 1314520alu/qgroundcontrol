#pragma once

#include "UnitTest.h"

class UnipodMt11MediaClientTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testDefaultBaseUrlFromRtsp();
    void testBuildMediaListUrl();
    void testBuildMediaCountAndDirectoriesUrls();
    void testParseMediaListSuccess();
    void testParseMediaListError();
    void testParseMediaListInvalidJson();
    void testParseDirectoriesResponse();
    void testJoinSavePath();
};

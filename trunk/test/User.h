#pragma once

#include "..\inc\Process.h"
#include "pch.h"

class User
{

};

int SidTest(int argc, _TCHAR * argv[]);
void TestIsCurrentUserLocalAdministrator();
void TestEnumerateAccountRights();
void TestGetCurrentSid();
void TestConvertStringSidToSid();

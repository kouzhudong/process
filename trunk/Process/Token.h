/*
������Ҫ��Token��صĶ�����ע�⣺Token�̳����û���

Token���ڽ��̣�����������ļ���

���ĳ��˽��̵�Token���û��⣬�������ػ����飬������ݡ�

��Security�������ܺã����Ƿ�Χ̫�㣬�����Tokenרҵ��
*/

#pragma once

class Security
{

};


//////////////////////////////////////////////////////////////////////////////////////////////////
//StartInteractiveClientProcess�����õ��ļ������塣


#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | \
DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | \
DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | \
DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | \
WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | \
WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS | \
WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | \
STANDARD_RIGHTS_REQUIRED)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
BOOL WINAPI SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
);


EXTERN_C
__declspec(dllexport)
BOOL WINAPI AdjustCurrentProcessPrivilege(PCTSTR szPrivilege, BOOL fEnable);


//////////////////////////////////////////////////////////////////////////////////////////////////

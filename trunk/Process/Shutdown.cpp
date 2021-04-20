#include "pch.h"
#include "Shutdown.h"


//////////////////////////////////////////////////////////////////////////////////////////////////


bool GetCurrentToken(OUT PHANDLE hToken)
/*
���ܣ���ȡ��ǰ������/�̣߳���TOKEN��
ע�⣺�����Ȩ�ޡ�

����ӣ�TOKEN_ASSIGN_PRIMARY
����http://blogs.msdn.com/b/s4cd/archive/2007/04/16/guest-blog-user-account-control-for-developers.aspx
*/
{
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY, TRUE, hToken)) {
        if (GetLastError() == ERROR_NO_TOKEN) {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY, hToken)) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }

    return TRUE;
}


bool GetComSpec(OUT TCHAR * cmd)
/*
���ܣ���ȡ ������ʾ��������·����

�������Ӧ�ñȽϱ�׼��
https://technet.microsoft.com/en-us/library/cc976142.aspx
HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment
*/
{
    if (cmd == NULL) {
        return false;
    }

    TCHAR  infoBuf[MAX_PATH] = {0};
    DWORD  bufCharCount = ExpandEnvironmentStrings(L"%ComSpec%", infoBuf, MAX_PATH);//��ʵҲ���Զ�̬��ȡ��Ҫ���ڴ�Ĵ�С.
    if (bufCharCount > MAX_PATH || bufCharCount == 0) {
        int x = GetLastError();
        return false;
    }

    RtlCopyMemory(cmd, infoBuf, bufCharCount * sizeof(TCHAR));
    return true;
}


EXTERN_C
__declspec(dllexport)
int WINAPI RemoveShutdownPrivilegeStartProcess()
/*
���ܣ�ȥ���ػ���Ȩ�ޣ�Ȼ���������̡�
      ������û�йػ���Ȩ���������̡�
      ͨ�׵�˵�ǣ���ֹ�������Ǹ����̹ػ���
*/
{
    HANDLE hToken = 0;
    GetCurrentToken(&hToken);

    DWORD Flags = 0;
    DWORD DisableSidCount = 0;
    PSID_AND_ATTRIBUTES SidsToDisable = NULL;
    DWORD DeletePrivilegeCount = 0;
    LUID_AND_ATTRIBUTES PrivilegesToDelete = {0};
    DWORD RestrictedSidCount = 0;
    PSID_AND_ATTRIBUTES SidsToRestrict = NULL;
    HANDLE NewTokenHandle = NULL;

    LUID shutdownPrivilege = {0};

    if (!LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        SE_SHUTDOWN_NAME,// privilege to lookup 
        &shutdownPrivilege))        // receives LUID of privilege
    {
        printf("LookupPrivilegeValue error: %u\n", GetLastError());
        return FALSE;
    }

    DeletePrivilegeCount = 1;
    PrivilegesToDelete.Luid = shutdownPrivilege;
    PrivilegesToDelete.Attributes = 0;

    BOOL B = CreateRestrictedToken(
        hToken,
        Flags,
        DisableSidCount,
        SidsToDisable,
        DeletePrivilegeCount,
        &PrivilegesToDelete,
        RestrictedSidCount,
        SidsToRestrict,
        &NewTokenHandle
    );
    if (B == 0) {
        int x = GetLastError();
        return x;
    }

    TCHAR ComSpec[MAX_PATH] = {0};
    GetComSpec(ComSpec);

    LPCTSTR lpApplicationName = NULL;
    LPSECURITY_ATTRIBUTES lpProcessAttributes = NULL;
    LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL;
    BOOL bInheritHandles = false;
    DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS;
    LPVOID lpEnvironment = NULL;
    LPCTSTR lpCurrentDirectory = NULL;
    STARTUPINFO StartupInfo = {0};
    PROCESS_INFORMATION ProcessInformation = {0};
    StartupInfo.cb = sizeof(STARTUPINFO);
    B = CreateProcessAsUser(
        NewTokenHandle,
        lpApplicationName,
        ComSpec,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        dwCreationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        &StartupInfo,
        &ProcessInformation
    );
    if (B == 0) {
        int x = GetLastError();//5 �ܾ����ʡ� 
        return x;
    }

    if (B && ProcessInformation.hProcess != INVALID_HANDLE_VALUE) {
        WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
        CloseHandle(ProcessInformation.hProcess);
    }

    if (ProcessInformation.hThread != INVALID_HANDLE_VALUE) {
        CloseHandle(ProcessInformation.hThread);
    }

    if (hToken != INVALID_HANDLE_VALUE) {
        CloseHandle(hToken);
    }

    if (NewTokenHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(NewTokenHandle);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
BOOL WINAPI MySystemShutdown()
//�ػ��ķ���һ��
{
    HANDLE hToken; // Get a token for this process. 
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) return(FALSE);

    TOKEN_PRIVILEGES tkp; // Get the LUID for the shutdown privilege. 
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process. 
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
    if (GetLastError() != ERROR_SUCCESS) return FALSE;

#pragma prefast(push)
#pragma prefast(disable: 28159, "����ʹ�á�InitiateSystemShutdownEx��")
    // Shut down the system and force all applications to close. 
    if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED))
        return FALSE;
#pragma prefast(pop)    

    return TRUE;//shutdown was successful
}


EXTERN_C
__declspec(dllexport)
BOOL WINAPI MySystemShutdownWithDialogbox(LPCTSTR lpMsg)
//����ʾ�Ĺػ���
{
    HANDLE hToken;              // handle to process token 
    // Get the current process token handle so we can get shutdown privilege. 
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    TOKEN_PRIVILEGES tkp;       // pointer to token structure 
    // Get the LUID for shutdown privilege. 
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get shutdown privilege for this process. 
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

    // Cannot test the return value of AdjustTokenPrivileges. 
    if (GetLastError() != ERROR_SUCCESS) return FALSE;

#pragma prefast(push)
#pragma prefast(disable: 28159, "����ʹ�á�InitiateSystemShutdownEx��")
    BOOL fResult;               // system shutdown flag 
    // Display the shutdown dialog box and start the countdown. 
    fResult = InitiateSystemShutdown(NULL,    // shut down local computer 
                                     (LPWSTR)lpMsg,   // message for user
                                     30,      // time-out period, in seconds 
                                     FALSE,   // ask user to close apps 
                                     TRUE);   // reboot after shutdown 
    if (!fResult)
        return FALSE;
#pragma prefast(pop)      

    // Disable shutdown privilege. 
    tkp.Privileges[0].Attributes = 0;
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

    return TRUE;
}


EXTERN_C
__declspec(dllexport)
BOOL WINAPI PreventSystemShutdown()
//��ֹ�ػ���
{
    HANDLE hToken;              // handle to process token 
    // Get the current process token handle  so we can get shutdown privilege. 
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    TOKEN_PRIVILEGES tkp;       // pointer to token structure 
    // Get the LUID for shutdown privilege. 
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get shutdown privilege for this process. 
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
    if (GetLastError() != ERROR_SUCCESS) return FALSE;

    if (!AbortSystemShutdown(NULL)) return FALSE; // Prevent the system from shutting down. 

    tkp.Privileges[0].Attributes = 0; // Disable shutdown privilege. 
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

    return TRUE;
}


int __cdecl ShutdownTest()
{
    //���������
    int r = MessageBox(NULL, (LPCWSTR)L"Ҫ�����������", (LPCWSTR)L"�������������", MB_YESNO);
    if (r == IDYES) LockWorkStation();

    //ע����ǰ�û��ķ���һ��
    r = MessageBox(NULL, (LPCWSTR)L"Ҫע����ǰ�û���", (LPCWSTR)L"ע����ǰ�û�����һ", MB_YESNO);
    if (r == IDYES) ExitWindows(0, 0);

    //ע����ǰ�û��ķ�������
    r = MessageBox(NULL, (LPCWSTR)L"Ҫע����ǰ�û���", (LPCWSTR)L"ע����ǰ�û����Զ�", MB_YESNO);
    if (r == IDYES) ExitWindowsEx(EWX_LOGOFF, 0);

    /*��Ӧ�ã����棩��������ֹע���İ취�����Ƿ���ʧ�ܡ�
    case WM_QUERYENDSESSION:
    {
    int r = MessageBox(NULL,(LPCWSTR)L"End the session?",(LPCWSTR)L"WM_QUERYENDSESSION",MB_YESNO);
    return r == IDYES; // Return TRUE to continue, FALSE to stop.
    break;
    }
    */

    //�ػ�����һ��
    r = MessageBox(NULL, (LPCWSTR)L"Ҫ�ػ���", (LPCWSTR)L"�ػ�����һ", MB_YESNO);
    if (r == IDYES) MySystemShutdown();//�Ѿ���ʼ�ػ���������sleep��������Ҳû���á�

    //�����ԣ�����Ĺػ�������ȡ������֪����Ϣ�ܷ����ء�hook ExitWindowsEx�ǿ��Եġ�
    //��ʹ�����������ȡ���ػ�������Ҳ�޼����¡�
    /*ȡ���ػ�
    r = MessageBox(NULL,(LPCWSTR)L"Ҫȡ���ػ���",(LPCWSTR)L"ȡ���ػ�",MB_YESNO);
    if (r == IDYES) PreventSystemShutdown();
    */

    //�ػ����Զ���
    r = MessageBox(NULL, (LPCWSTR)L"Ҫ�ػ���", (LPCWSTR)L"�ػ����Զ�", MB_YESNO);
    if (r == IDYES)MySystemShutdownWithDialogbox(L"made by correy");//��xp�¾��ǵ����Ǹ��������Ϣ��Win 7��Ҳ������Ϣ��

    //ȡ���ػ�
    r = MessageBox(NULL, (LPCWSTR)L"Ҫȡ���ػ���", (LPCWSTR)L"ȡ���ػ�", MB_YESNO);
    if (r == IDYES) PreventSystemShutdown();//�൱��shutdon -a

    //�����ľͲ�˵�ˣ���ĺ���Ҳ��ʵ�֡�

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

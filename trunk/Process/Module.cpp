#include "pch.h"
#include "Module.h"
#include <shellapi.h>


#pragma warning(disable:6273)
#pragma warning(disable:4477)
#pragma warning(disable:4313)
#pragma warning(disable:6328)
#pragma warning(disable:4311)
#pragma warning(disable:4302)


//////////////////////////////////////////////////////////////////////////////////////////////////


// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS 
// and compile with -DPSAPI_VERSION=1


int PrintModules(DWORD processID)
/*
ȱ�㣺ö�ٲ���WOW64��DLL��
*/
{
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;

    // Print the process identifier.
    printf("\nProcess ID: %u\n", processID);

    // Get a handle to the process.
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (NULL == hProcess)
        return 1;

    // Get a list of all the modules in this process.
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            TCHAR szModName[MAX_PATH];

            // Get the full path to the module's file.
            if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                // Print the module name and handle value.
                _tprintf(TEXT("\t%s (0x%08X)\n"), szModName, hMods[i]);
            }
        }
    }

    // Release the handle to the process.
    CloseHandle(hProcess);

    return 0;
}


EXTERN_C
__declspec(dllexport)
int WINAPI EnumeratingAllModulesForProcess(void)
/*
Enumerating All Modules For a Process
2018/05/31

To determine which processes have loaded a particular DLL, you must enumerate the modules for each process. 
The following sample code uses the EnumProcessModules function to enumerate the modules of current processes in the system.

https://docs.microsoft.com/zh-cn/windows/win32/psapi/enumerating-all-modules-for-a-process
*/
{
    DWORD aProcesses[1024];
    DWORD cbNeeded;
    DWORD cProcesses;
    unsigned int i;

    // Get the list of process identifiers.
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
        return 1;

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the names of the modules for each process.
    for (i = 0; i < cProcesses; i++) {
        PrintModules(aProcesses[i]);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
BOOL WINAPI ListProcessModules(DWORD dwPID)
/*
Traversing the Module List
05/31/2018

The following example obtains a list of modules for the specified process. 
The ListProcessModules function takes a snapshot of the modules associated with a given process using the CreateToolhelp32Snapshot function, 
and then walks through the list using the Module32First and Module32Next functions. 
The dwPID parameter of ListProcessModules identifies the process for which modules are to be enumerated, 
and is usually obtained by calling CreateToolhelp32Snapshot to enumerate the processes running on the system. 
See Taking a Snapshot and Viewing Processes for a simple console application that uses this function.

A simple error-reporting function, printError, displays the reason for any failures, 
which usually result from security restrictions.

�÷�ʾ����ListProcessModules(GetCurrentProcessId());

��ֹWOW64�����С�
WOW64����64����֣�WARNING: CreateToolhelp32Snapshot (of modules) failed with error 299 (����ɲ��ֵ� ReadProcessMemory �� WriteProcessMemory ����)
����WOW64����64ʧ�ܡ�

����WOW64�����ȡ����WOW64���磺C:\Windows\SysWOW64����DLL��

https://docs.microsoft.com/en-us/windows/win32/toolhelp/traversing-the-module-list
*/
{
    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
    MODULEENTRY32 me32;

    //  Take a snapshot of all modules in the specified process. 
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);
    if (hModuleSnap == INVALID_HANDLE_VALUE)
    {
        printError(TEXT("CreateToolhelp32Snapshot (of modules)"));
        return(FALSE);
    }

    //  Set the size of the structure before using it. 
    me32.dwSize = sizeof(MODULEENTRY32);

    //  Retrieve information about the first module, and exit if unsuccessful 
    if (!Module32First(hModuleSnap, &me32))
    {
        printError(TEXT("Module32First"));  // Show cause of failure 
        CloseHandle(hModuleSnap);     // Must clean up the snapshot object! 
        return(FALSE);
    }

    //  Now walk the module list of the process, and display information about each module 
    do
    {
        _tprintf(TEXT("\n\n     MODULE NAME:     %s"), me32.szModule);
        _tprintf(TEXT("\n     executable     = %s"), me32.szExePath);
        _tprintf(TEXT("\n     process ID     = 0x%08X"), me32.th32ProcessID);
        _tprintf(TEXT("\n     ref count (g)  =     0x%04X"), me32.GlblcntUsage);
        _tprintf(TEXT("\n     ref count (p)  =     0x%04X"), me32.ProccntUsage);
        _tprintf(TEXT("\n     base address   = 0x%08X"), (DWORD)me32.modBaseAddr);
        _tprintf(TEXT("\n     base size      = %d"), me32.modBaseSize);

    } while (Module32Next(hModuleSnap, &me32));

    _tprintf(TEXT("\n"));

    //  Do not forget to clean up the snapshot object. 
    CloseHandle(hModuleSnap);
    return(TRUE);
}


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
void CALLBACK RunDllApi(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
/*
�˺���ר�����ڱ�rundll32.exe���ã����Դ˺�����ԭ�͹̶���

��������ɫ������ɸ��Ĳ����ĸ����������ƣ�ϵͳ/�ڴ�ȵĳ��⣩��

�÷�ʾ����
rundll32.exe Process.dll,RunDllApi notepad.exe d:\test.txt
��Ȼ������Ը�����Ĳ������磺
rundll32.exe Process.dll,RunDllApi notepad.exe d:\test.txt x y z
ע�⣺���ŵ����á�

ע�⣺rundll32Ĭ����û�п���̨�ġ�
*/
{
    DWORD NumberOfCharsWritten = 0;

    //__debugbreak();    

    setlocale(LC_CTYPE, ".936");  

    //HANDLE Output = GetStdHandle(STD_OUTPUT_HANDLE);
    //char buffer[MAX_PATH] = {0};
    //wsprintfA(buffer, "����������:%s.\n", lpszCmdLine);
    //WriteConsole(Output, buffer, sizeof(buffer), &NumberOfCharsWritten, NULL);
    MessageBoxA(0, lpszCmdLine, "����������", 0);

    WCHAR Buffer[MAX_PATH] = {0};
    MessageBox(0, GetCommandLineW(), L"������", 0);

    WCHAR pwszDst[MAX_PATH];
    SHAnsiToUnicode(lpszCmdLine, pwszDst, MAX_PATH);
    PathRemoveBlanksW(pwszDst);

    int Args;
    LPWSTR * Arglist = CommandLineToArgvW(pwszDst, &Args);
    if (NULL == Arglist) {
        wsprintf(Buffer, L"LastError��%d.\n", GetLastError());
        MessageBox(0, Buffer, L"CommandLineToArgvW���д���", 0);
        return;
    }

    const WCHAR * temp = L"�����������Ľ�����\n";
    MessageBoxA(0, lpszCmdLine, "��ʼ����������������", 0);

    wsprintf(Buffer, L"%d.\n", Args);
    MessageBox(0, Buffer, L"�����������ĸ�����", 0);

    for (int i = 0; i < Args; i++) {
        wsprintf(Buffer, L"������%d.\n", i + 1);
        MessageBox(0, Arglist[i], Buffer, 0);
    }

    ShellExecute(hwnd, 0i64, Arglist[0], Arglist[1], 0i64, nCmdShow);

    LocalFree(Arglist);
}


//////////////////////////////////////////////////////////////////////////////////////////////////


#define _WINSOCK_DEPRECATED_NO_WARNINGS 


set<wstring> g_Module;
CRITICAL_SECTION g_ModuleLock;


void GetAllModuleInfo(HANDLE hProcess)
/*
���ܣ���ȡһ�����̵����е�ģ����Ϣ��

To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS and compile with -DPSAPI_VERSION=1

�˰취����ʾWOW64��dll,����ʾ��·�����ԣ���64��·����
����GetModuleFileNameEx���µġ�
�Ľ�˼·����GetMappedFileName�����ǵõ�����NT·����

�˺�����������WOW64,������̫��Ľ��̻�ȡΪ�գ���ʹ��ȨҲ���С�

�ο���
https://docs.microsoft.com/zh-cn/windows/win32/psapi/enumerating-all-modules-for-a-process
https://docs.microsoft.com/zh-cn/windows/win32/memory/obtaining-a-file-name-from-a-file-handle
*/
{
    DWORD cbNeeded = 0;
    EnumProcessModulesEx(hProcess, NULL, 0, &cbNeeded, LIST_MODULES_ALL);
    if (0 == cbNeeded) {
        //LOGW(IMPORTANT_INFO_LEVEL, "���棺LastError:%d, pid:%d", GetLastError(), GetProcessId(hProcess));
        return;
    }

    HMODULE * hMods = (HMODULE *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbNeeded);
    if (NULL == hMods) {
        //LOGA(ERROR_LEVEL, "�����ڴ�ʧ��");
        return;
    }

    if (EnumProcessModulesEx(hProcess, hMods, cbNeeded, &cbNeeded, LIST_MODULES_ALL)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            TCHAR szModName[MAX_PATH] = {0};

#pragma warning(push)
#pragma warning(disable:6011)
#pragma warning(disable:6385)
            if (GetMappedFileName(hProcess, hMods[i], szModName, MAX_PATH)) {
#pragma warning(pop)
                Nt2Dos(szModName);//������NT������\Device\HarddiskVolume6\��ͷ��
                lstrcat(szModName, L"\n");
                g_Module.insert(szModName);
            } else {
                //LOGA(ERROR_LEVEL, "pid:%d, LastError:%#x", GetProcessId(hProcess), GetLastError());
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, hMods);
}


void GetProcessListInfo()
/*
ע�⣺
����û�ж�Ӧ�ļ���һЩ���̣�������OpenProcessʧ�ܵġ�
�����ܱ����Ľ��̣�������OpenProcessʧ�ܵġ�
*/
{
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return;
    }

    do {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION,
                                      FALSE,
                                      pe32.th32ProcessID);
        if (hProcess) {
            GetAllModuleInfo(hProcess);
            CloseHandle(hProcess);
        } else {
            switch (GetLastError()) {
            case ERROR_ACCESS_DENIED://5���ܾ����ʣ���Ȩ�޲��㣩��
            case ERROR_INVALID_PARAMETER://87���������󣬼������Ѿ��˳�����
                break;
            default:
                //LOGW(IMPORTANT_INFO_LEVEL, "pid:%#d", pe32.th32ProcessID);
                //DbgPrintW("���棺�򿪽���ʧ��, pid:%#d, LastError:%d", pe32.th32ProcessID, GetLastError());
                break;
            }
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
}


void PrintAllModule()
{
    wstring all;
    int n = 0;

    //һ��һ���Ĵ�ӡ�е����������ռ��£�һ�´�ӡ��
    if (!g_Module.empty()) {
        for (wstring temp : g_Module) {
            //printf("%ls.\n", temp.c_str());
            all += temp;
            n++;
        }
    }

    //printf("%d.\n", all.size());
    //printf("%d.\n", all.length());
    //printf("%d.\n", all.max_size());//-2.
    //printf("%d.\n", all._Mysize);
    printf("ģ���ܸ�����%d.\n", n);

    printf("ģ����Ϣ���£�\n");
    printf("\n");

    printf("%ls\n", all.c_str());
}


void PrintAllModuleTest()
/*
���ܣ�ö��ϵͳ�����н��̣��ų�û�ж�Ӧ�ļ���һЩ���̺��ܱ����Ľ��̣�������ģ�飨ȥ���ظ�����

�о���������ǲ��Ǻ����á�
1.һЩϵͳ�Ĺ��ߣ�Ҳ�����ƵĹ��ܣ��磺tasklist /m.
2.procexp.exe����������
3.Listdlls.exe�����ظ���
*/
{
    GetProcessListInfo();

    PrintAllModule();
}


//////////////////////////////////////////////////////////////////////////////////////////////////


int PrintModulesEx(DWORD processID)
/*
To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS and compile with -DPSAPI_VERSION=1

�˰취����ʾWOW64��dll,����ʾ��·�����ԣ���64��·����
����GetModuleFileNameEx���µġ�
�Ľ�˼·����GetMappedFileName�����ǵõ�����NT·����

�˺�����������WOW64,������̫��Ľ��̻�ȡΪ�գ���ʹ��ȨҲ���С�

�ο���
https://docs.microsoft.com/zh-cn/windows/win32/psapi/enumerating-all-modules-for-a-process
https://docs.microsoft.com/zh-cn/windows/win32/memory/obtaining-a-file-name-from-a-file-handle
*/
{
    HMODULE hMods[1024];
    HANDLE hProcess;
    DWORD cbNeeded;
    unsigned int i;

    // Print the process identifier.
    printf("\nProcess ID: %u\n", processID);

    // Get a handle to the process.
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (NULL == hProcess) {
        printf("\nLastError: %u\n", GetLastError());
        return 1;
    }

    // Get a list of all the modules in this process.
    if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL)) {
        for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            TCHAR szModName[MAX_PATH];

            // Get the full path to the module's file.
            if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
                // Print the module name and handle value.
                //_tprintf(TEXT("\t%s (0x%p)\n"), szModName, hMods[i]);
            }

            if (GetMappedFileName(hProcess, hMods[i], szModName, MAX_PATH)) {
                // Translate path with device name to drive letters.
                TCHAR szTemp[MAX_PATH];
                szTemp[0] = '\0';

                if (GetLogicalDriveStrings(MAX_PATH - 1, szTemp)) {
                    TCHAR szName[MAX_PATH];
                    TCHAR szDrive[3] = TEXT(" :");
                    BOOL bFound = FALSE;
                    TCHAR * p = szTemp;

                    do {
                        // Copy the drive letter to the template string
                        *szDrive = *p;

                        // Look up each device name
                        if (QueryDosDevice(szDrive, szName, MAX_PATH)) {
                            size_t uNameLen = _tcslen(szName);

                            if (uNameLen < MAX_PATH) {
                                bFound = _tcsnicmp(szModName, szName, uNameLen) == 0 && *(szModName + uNameLen) == _T('\\');

                                if (bFound) {
                                    // Reconstruct pszFilename using szTempFile
                                    // Replace device path with DOS path
                                    TCHAR szTempFile[MAX_PATH];
                                    StringCchPrintf(szTempFile,
                                                    MAX_PATH,
                                                    TEXT("%s%s"),
                                                    szDrive,
                                                    szModName + uNameLen);
                                    StringCchCopyN(szModName, MAX_PATH, szTempFile, _tcslen(szTempFile));
                                }
                            }
                        }

                        // Go to the next NULL character.
                        while (*p++);
                    } while (!bFound && *p); // end of string
                }

                _tprintf(TEXT("\t%s (0x%p)\n"), szModName, hMods[i]);//������NT������\Device\HarddiskVolume6\��ͷ��
            }
        }
    }

    CloseHandle(hProcess);// Release the handle to the process.
    return 0;
}


int TestPrintModulesEx(void)
{
    DWORD aProcesses[1024];
    DWORD cbNeeded;
    DWORD cProcesses;
    unsigned int i;

    EnablePrivilege(SE_DEBUG_NAME, TRUE);

    // Get the list of process identifiers.
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        _ASSERTE(FALSE);
        return 1;
    }

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the names of the modules for each process.
    for (i = 0; i < cProcesses; i++) {
        PrintModulesEx(aProcesses[i]);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

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

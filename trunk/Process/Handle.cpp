#include "pch.h"
#include "Handle.h"
#include "Token.h"


#pragma warning(disable:6011)
#pragma warning(disable:6387)


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
DWORD WINAPI EnumerateProcessHandles(ULONG pid)
/*!
* Enumerate all handles of the specified process using undocumented APIs.
*/
{
    // The functions have no associated import library.
    // You must use the LoadLibrary and GetProcAddress functions to dynamically link to ntdll.dll.
    HINSTANCE hNtDll = LoadLibrary(_T("ntdll.dll"));
    assert(hNtDll != NULL);
    PFN_NTQUERYSYSTEMINFORMATION NtQuerySystemInformation = (PFN_NTQUERYSYSTEMINFORMATION)
        GetProcAddress(hNtDll, "NtQuerySystemInformation");
    assert(NtQuerySystemInformation != NULL);

    PFN_ZwQueryObject NtQueryObject = (PFN_ZwQueryObject)GetProcAddress(hNtDll, "NtQueryObject");
    assert(NtQueryObject != NULL);

    // NtQuerySystemInformation does not return the correct required buffer size if the buffer passed is too small.
    // Instead you must call the function while increasing the buffer size until the function no longer returns STATUS_INFO_LENGTH_MISMATCH.
    DWORD nSize = 4096, nReturn;
    PSYSTEM_HANDLE_INFORMATION pSysHandleInfo = (PSYSTEM_HANDLE_INFORMATION)HeapAlloc(GetProcessHeap(), 0, nSize);
    while (NtQuerySystemInformation(SystemHandleInformation, pSysHandleInfo, nSize, &nReturn) == STATUS_INFO_LENGTH_MISMATCH)// Get system handle information.
    {
        HeapFree(GetProcessHeap(), 0, pSysHandleInfo);
        nSize += 4096;
        pSysHandleInfo = (SYSTEM_HANDLE_INFORMATION *)HeapAlloc(GetProcessHeap(), 0, nSize);
        _ASSERTE(pSysHandleInfo);
    }
    /*
    ��������Ѿ�������ϵͳ�ľ����Ϣ���Ѿ���ȡ�ˡ�
    ��Ȼ���б����Ϣ������̵ȡ�
    ������Ǹ�����Щ��Ϣ���з�����ѣ�����̡�

    ��ʵ�������������ʾ����ϵͳ�ľ������ϸ��Ϣ��
    */

    DWORD dwhandles = 0;//һ�����̵����еľ��������

    // Get the handle of the target process.
    // The handle will be used to duplicate the handles in the process.
    HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        _tprintf(_T("OpenProcess failed w/err 0x%08lx\n"), GetLastError());
        return -1;
    }
    /*
    �����������ҪȨ�ޣ�������ϵͳ������ȨҲ�򲻿���
    �����ԣ�
    ��XP�Ͽ��Դ�ϵͳ���̣�system����
    ��Windows 7�ϣ���Ȩ���Թ���Ա��administrator��Ȩ�ޣ������Է����Ȩ�ޣ�NT AUTHORITY\SYSTEM�������У����ƵĻỰ���뵼�£��ɷ���Ӧ��Ҳ�ڻỰ0�����Ի���������Ȩ�޴󣩡�
    */

    for (ULONG i = 0; i < pSysHandleInfo->NumberOfHandles; i++) {
        PSYSTEM_HANDLE pHandle = &(pSysHandleInfo->Handles[i]);

        //���ݽ��̽���������
        if (pHandle->ProcessId == pid) {
            dwhandles++;	// Increase the number of handles

            /*
            �����Է��֣�EtwRegistration(39)���͵ľ����ʧ�ܡ�
            0x00000032 ��֧�ָ�����
            */
            HANDLE hCopy;// Duplicate the handle in the current process
            if (!DuplicateHandle(hProcess,
                                 (HANDLE)pHandle->Handle,
                                 GetCurrentProcess(),
                                 &hCopy,
                                 MAXIMUM_ALLOWED,
                                 FALSE,
                                 0)) {
                wprintf(L"DuplicateHandle fail with 0x%x,HANDLE:0x%x,ObjectTypeNumber:%d\n",
                        GetLastError(), pHandle->Handle, pHandle->ObjectTypeNumber);
                continue;
            }

            /*
            ������������ø��Ƶľ������һЩ���������ѯֵ��
            */
            ULONG  ObjectInformationLength = sizeof(OBJECT_NAME_INFORMATION) + 512;
            POBJECT_NAME_INFORMATION poni = (POBJECT_NAME_INFORMATION)
                HeapAlloc(GetProcessHeap(), 0, ObjectInformationLength);
            assert(poni != NULL);

            ULONG  ReturnLength;

            /*
            ��������������TOKEN���̣߳����̵����͵ģ���Ҫ������Ĵ���
            Ҳ����˵��������ǲ�ѯ�����ġ�
            */
            if (NtQueryObject(hCopy,
                              (OBJECT_INFORMATION_CLASS)1,
                              poni,
                              ObjectInformationLength, 
                              &ReturnLength) != STATUS_SUCCESS) {
                wprintf(L"NtQueryObject fail!\n");
                HeapFree(GetProcessHeap(), 0, poni);
                continue;
            }

            wprintf(L"HANDLE:0x%x, NAME:%wZ\n", pHandle->Handle, &poni->Name);

            HeapFree(GetProcessHeap(), 0, poni);
            CloseHandle(hCopy);
        }

        //������ʾ����ϵͳ�ľ����Ϣ��
        //wprintf(L"PID:0x%x\n", pHandle->ProcessId);
        //wprintf(L"\tHANDLE:0x%x\n", pHandle->Handle);
    }

    CloseHandle(hProcess);
    HeapFree(GetProcessHeap(), 0, pSysHandleInfo);// Clean up.	
    return dwhandles;// Return the number of handles in the process
}


int EnumerateCurrentProcessHandles(int argc, _TCHAR * argv[])
/*
ö��һ�����̵ľ����Ϣ��
����һ�������Ĺ��ܡ�

һֱ������û������
����Ҳ�кö�Ĵ��롣
��������ʵ���ˡ�

�����޸���΢���CppFileHandle���̡�
��һ���Ŀ��Ŷȡ�

made by correy
made at 2014.06.25
*/
{
    BOOL B = AdjustCurrentProcessPrivilege(SE_DEBUG_NAME, TRUE);
    DWORD dwFiles = EnumerateProcessHandles(GetCurrentProcessId());
    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


/****************************** Module Header ******************************\
* Module Name:  CppFileHandle.cpp
* Project:      CppFileHandle
* Copyright (c) Microsoft Corporation.
*
* CppFileHandle demonstrates two typical scenarios of using file handles:
*
* 1) Enumerate file handles of a process
* 2) Get file name from a file handle
*
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/en-us/openness/resources/licenses.aspx#MPL.
* All other rights reserved.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/


DWORD EnumerateFileHandles(ULONG pid)
/*!
* Enumerate file handles of the specified process using undocumented APIs.
*
* \param pid
* Process Id
*/
{
    /////////////////////////////////////////////////////////////////////////
    // Prepare for NtQuerySystemInformation and NtQueryInformationFile.

    // The functions have no associated import library.
    // You must use the LoadLibrary and GetProcAddress functions to dynamically link to ntdll.dll.
    HINSTANCE hNtDll = LoadLibrary(_T("ntdll.dll"));
    assert(hNtDll != NULL);

    PFN_NTQUERYSYSTEMINFORMATION NtQuerySystemInformation = (PFN_NTQUERYSYSTEMINFORMATION)GetProcAddress(hNtDll, "NtQuerySystemInformation");
    assert(NtQuerySystemInformation != NULL);

    PFN_NTQUERYINFORMATIONFILE NtQueryInformationFile = (PFN_NTQUERYINFORMATIONFILE)GetProcAddress(hNtDll, "NtQueryInformationFile");

    /////////////////////////////////////////////////////////////////////////
    // Get system handle information.
    DWORD nSize = 4096, nReturn;
    PSYSTEM_HANDLE_INFORMATION pSysHandleInfo = (PSYSTEM_HANDLE_INFORMATION)HeapAlloc(GetProcessHeap(), 0, nSize);

    // NtQuerySystemInformation does not return the correct required buffer size if the buffer passed is too small. 
    // Instead you must call the function while increasing the buffer size until the function no longer returns STATUS_INFO_LENGTH_MISMATCH.
    while (NtQuerySystemInformation(SystemHandleInformation, pSysHandleInfo, nSize, &nReturn) == STATUS_INFO_LENGTH_MISMATCH) {
        HeapFree(GetProcessHeap(), 0, pSysHandleInfo);
        nSize += 4096;
        pSysHandleInfo = (SYSTEM_HANDLE_INFORMATION *)HeapAlloc(GetProcessHeap(), 0, nSize);
    }

    /////////////////////////////////////////////////////////////////////////
    // Enumerate file handles of the process.
    DWORD dwFiles = 0;

    // Get the handle of the target process.
    // The handle will be used to duplicate the file handles in the process.
    HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        _tprintf(_T("OpenProcess failed w/err 0x%08lx\n"), GetLastError());
        return -1;
    }

    for (ULONG i = 0; i < pSysHandleInfo->NumberOfHandles; i++) {
        PSYSTEM_HANDLE pHandle = &(pSysHandleInfo->Handles[i]);

        // Check for file handles of the specified process
        if (pHandle->ProcessId == pid && pHandle->ObjectTypeNumber == HANDLE_TYPE_FILE) {
            dwFiles++;	// Increase the number of file handles

            // Duplicate the handle in the current process
            HANDLE hCopy;
            if (!DuplicateHandle(hProcess,
                                 (HANDLE)pHandle->Handle,
                                 GetCurrentProcess(),
                                 &hCopy,
                                 MAXIMUM_ALLOWED,
                                 FALSE,
                                 0))
                continue;

            // Retrieve file name information about the file object.
            IO_STATUS_BLOCK ioStatus;
            PFILE_NAME_INFORMATION pNameInfo = (PFILE_NAME_INFORMATION)malloc(MAX_PATH * 2 * 2);
            DWORD dwInfoSize = MAX_PATH * 2 * 2;

            if (NtQueryInformationFile(hCopy, &ioStatus, pNameInfo, dwInfoSize, FileNameInformation) == STATUS_SUCCESS) {
                // Get the file name and print it
                WCHAR wszFileName[MAX_PATH + 1];
                StringCchCopyNW(wszFileName,
                                MAX_PATH + 1, 
                                pNameInfo->FileName,
                                /*must be WCHAR*/ pNameInfo->FileNameLength /*in bytes*/ / 2);
                wprintf(L"0x%x:\t%s\n", pHandle->Handle, wszFileName);
            }
            free(pNameInfo);

            CloseHandle(hCopy);
        }
    }

    CloseHandle(hProcess);

    /////////////////////////////////////////////////////////////////////////	
    HeapFree(GetProcessHeap(), 0, pSysHandleInfo);// Clean up.

    return dwFiles;// Return the number of file handles in the process
}


#pragma region GetFileNameFromHandle
BOOL GetFileNameFromHandle(HANDLE hFile)
/*!
* Get file name from a handle to a file object using a file mapping object.
* It uses the CreateFileMapping and MapViewOfFile functions to create the mapping.
* Next, it uses the GetMappedFileName function to obtain the file name.
* For remote files, it prints the device path received from this function.
* For local files, it converts the path to use a drive letter and prints this path.
*
* \param hFile
* File handle
*/
{
    TCHAR szFileName[MAX_PATH + 1];
    HANDLE hFileMap;

    // Get the file size
    DWORD dwFileSizeHi = 0;
    DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);
    if (dwFileSizeLo == 0 && dwFileSizeHi == 0) {
        _tprintf(_T("Cannot map a file with a length of zero\n"));
        return FALSE;
    }

    /////////////////////////////////////////////////////////////////////////
    // Create a file mapping object.

    // Create a file mapping to get the file name
    hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 1, NULL);
    if (!hFileMap) {
        _tprintf(_T("CreateFileMapping failed w/err 0x%08lx\n"), GetLastError());
        return FALSE;
    }

    void * pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);
    if (!pMem) {
        _tprintf(_T("MapViewOfFile failed w/err 0x%08lx\n"), GetLastError());
        CloseHandle(hFileMap);
        return FALSE;
    }

    /////////////////////////////////////////////////////////////////////////
    // Call the GetMappedFileName function to obtain the file name.
    if (GetMappedFileName(GetCurrentProcess(), pMem, szFileName, MAX_PATH)) {
        // szFileName contains device file name like:
        // \Device\HarddiskVolume2\Users\JLG\AppData\Local\Temp\HLe6098.tmp
        _tprintf(_T("Device name is %s\n"), szFileName);

        // Translate path with device name to drive letters.
        TCHAR szTemp[BUFFER_SIZE];
        szTemp[0] = '\0';

        // Get a series of null-terminated strings, one for each valid drive in the system, plus with an additional null character. 
        // Each string is a drive name. e.g. C:\\0D:\\0\0
        if (GetLogicalDriveStrings(BUFFER_SIZE - 1, szTemp)) {
            TCHAR szName[MAX_PATH];
            TCHAR szDrive[3] = _T(" :");
            BOOL bFound = FALSE;
            TCHAR * p = szTemp;

            do {
                // Copy the drive letter to the template string
                *szDrive = *p;

                // Look up each device name. For example, given szDrive is C:, the output szName may be \Device\HarddiskVolume2.
                if (QueryDosDevice(szDrive, szName, MAX_PATH)) {
                    UINT uNameLen = (UINT)_tcslen(szName);
                    if (uNameLen < MAX_PATH) {
                        // Match the device name e.g. \Device\HarddiskVolume2
                        bFound = _tcsnicmp(szFileName, szName, uNameLen) == 0;
                        if (bFound) {
                            // Reconstruct szFileName using szTempFile
                            // Replace device path with DOS path
                            TCHAR szTempFile[MAX_PATH];
                            StringCchPrintf(szTempFile, MAX_PATH, _T("%s%s"), szDrive, szFileName + uNameLen);
                            StringCchCopyN(szFileName, MAX_PATH + 1, szTempFile, _tcslen(szTempFile));
                        }
                    }
                }

                while (*p++);// Go to the next NULL character, i.e. the next drive name.
            } while (!bFound && *p); // End of string
        }
    }

    /////////////////////////////////////////////////////////////////////////
    // Clean up the file mapping object.
    UnmapViewOfFile(pMem);
    CloseHandle(hFileMap);

    _tprintf(_T("File name is %s\n\n"), szFileName);
    return TRUE;
}
#pragma endregion


int FileHandleTest(int argc, _TCHAR * argv[])
/*
ԭ���������֣�File handle operations demo (CppFileHandle)
*/
{
    // Enumerate file handles of a process using undocumented APIs
    ULONG pid = GetCurrentProcessId();
    DWORD dwFiles = EnumerateFileHandles(pid);

    _tprintf(TEXT("\r\n"));

    // Get file name from file handle using a file mapping object
    HANDLE hFile;
    hFile = CreateFile(TEXT("test.txt"),
                       GENERIC_WRITE | GENERIC_READ,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        _tprintf(TEXT("CreateFile failed with %d\n"), GetLastError());
        return 0;
    }

    BYTE bWriteBuffer[] = "0123456789";
    DWORD dwBytesWritten;

    // Write 11 bytes from the buffer to the file 
    if (!WriteFile(hFile,                // File handle 
                   bWriteBuffer,                    // Buffer to be write from 
                   sizeof(bWriteBuffer),            // Number of bytes to write 
                   &dwBytesWritten,                 // Number of bytes that were written 
                   NULL))                           // No overlapped structure 
    {
        // WriteFile returns FALSE because of some error  
        _tprintf(TEXT("Could not write to file w/err 0x%08lx\n"), GetLastError());
        CloseHandle(hFile);
        return 0;
    }

    GetFileNameFromHandle(hFile);
    CloseHandle(hFile);

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
NTSTATUS WINAPI ZwEnumerateDirectoryObject(_In_ PCWSTR ObjectDirectory, _In_ PCWSTR TypeNameFilter)
/*
���ܣ�ö��һ��DirectoryObject�µĶ���

ObjectDirectory��ȡֵ�������ǣ�
1.L"\\KnownDlls"
2.L"\\KnownDlls32"
3.�ȵȣ�

TypeNameFilter�ǹ��ˣ���ȡ��ֵ�У�
1.SymbolicLink
2.Section
3.�ȵȡ�
*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    OBJECT_ATTRIBUTES oa;
    HANDLE FileHandle = 0;
    IO_STATUS_BLOCK  IoStatusBlock = {0};
    PVOID FileInformation = 0;
    ULONG Length = sizeof(FILE_DIRECTORY_INFORMATION);//��������õ�̫С�ᵼ��ZwQueryDirectoryFile������    
    BOOLEAN           RestartScan;
    ULONG             Context = 0;
    ULONG             ReturnedLength;
    //UNICODE_STRING driver = RTL_CONSTANT_STRING(L"section");
    UNICODE_STRING Directory = {0};

    Directory.Buffer = (PWSTR)ObjectDirectory;
    Directory.MaximumLength = (USHORT)(wcslen(ObjectDirectory) * sizeof(WCHAR));
    Directory.Length = Directory.MaximumLength;

    HINSTANCE hNtDll = LoadLibrary(_T("ntdll.dll"));
    assert(hNtDll != NULL);
    NtOpenDirectoryObject_PFN NtOpenDirectoryObject = (NtOpenDirectoryObject_PFN)
        GetProcAddress(hNtDll, "NtOpenDirectoryObject");
    assert(NtOpenDirectoryObject != NULL);

    NtQueryDirectoryObject_PFN NtQueryDirectoryObject =
        (NtQueryDirectoryObject_PFN)GetProcAddress(hNtDll, "NtQueryDirectoryObject");
    assert(NtQueryDirectoryObject != NULL);

    InitializeObjectAttributes(&oa, &Directory, OBJ_CASE_INSENSITIVE, 0, 0);
    status = NtOpenDirectoryObject(&FileHandle, DIRECTORY_QUERY, &oa);
    if (!NT_SUCCESS(status)) {
        //Print(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "0x%#x", status);
        return status;
    }

    Length = Length + 520;//Ϊ�μ�������֣��뿴ZwEnumerateFile1��˵����
    FileInformation = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Length);
    if (FileInformation == NULL) {
        status = STATUS_UNSUCCESSFUL;
        //Print(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "0x%#x", status);
        CloseHandle(FileHandle);
        return status;
    }
    RtlZeroMemory(FileInformation, Length);

    do {
        UNICODE_STRING FileName = {0};
        POBJECT_DIRECTORY_INFORMATION podi = 0;
        UNICODE_STRING FullName = {0};

        RestartScan = FALSE;//ΪTRUE�ᵼ����ѭ��;
        status = NtQueryDirectoryObject(FileHandle,
                                        FileInformation,
                                        Length,
                                        TRUE,
                                        RestartScan,
                                        &Context,
                                        &ReturnedLength);
        if (status != STATUS_NO_MORE_FILES && status != STATUS_SUCCESS) {
            break;//�������û���߹���
        }

        podi = (POBJECT_DIRECTORY_INFORMATION)FileInformation;

        if (TypeNameFilter) {
            if (_wcsicmp(podi->TypeName.Buffer, TypeNameFilter) != 0) {
                continue;
            }
        }

        printf("%ls\n", podi->Name.Buffer);
    } while (status != STATUS_NO_MORE_FILES);

    if (STATUS_NO_MORE_FILES == status) {
        status = STATUS_SUCCESS;
    }

    if (FileInformation) {
        HeapFree(GetProcessHeap(), 0, FileInformation);
        FileInformation = NULL;
    }

    CloseHandle(FileHandle);

    return status;
}


EXTERN_C
__declspec(dllexport)
void WINAPI QuerySymbolicLinkName(_In_ PCWSTR SymbolicLink)
/*
���ܣ���ȡ��\\KnownDlls\\KnownDllPath�����ķ������ӵ�ֵ��

ע��64λ�»��и�\\KnownDlls\\KnownDllPath32������ʵ�Ǹ��������ӡ�
*/
{
    ULONG ActualLength;
    HANDLE LinkHandle;
    WCHAR NameBuffer[128] = {0};//������ܶ����С��.
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING LinkString, NameString;

    HINSTANCE hNtDll = LoadLibrary(_T("ntdll.dll"));
    assert(hNtDll != NULL);
    NtOpenSymbolicLinkObject_PFN NtOpenSymbolicLinkObject = (NtOpenSymbolicLinkObject_PFN)
        GetProcAddress(hNtDll, "NtOpenSymbolicLinkObject");
    assert(NtOpenSymbolicLinkObject != NULL);

    NtQuerySymbolicLinkObject_PFN NtQuerySymbolicLinkObject =
        (NtQuerySymbolicLinkObject_PFN)GetProcAddress(hNtDll, "NtQuerySymbolicLinkObject");
    assert(NtQuerySymbolicLinkObject != NULL);

    NameString.Buffer = (PWSTR)SymbolicLink;
    NameString.MaximumLength = (USHORT)(wcslen(SymbolicLink) * sizeof(WCHAR));
    NameString.Length = NameString.MaximumLength;
    //RtlInitUnicodeString(&NameString, L"\\KnownDlls\\KnownDllPath");//��������//,��Ȼ��ZwOpenSymbolicLinkObject����ʧ��.���ǵõ��ľ��Ϊ0.

    InitializeObjectAttributes(&ObjectAttributes, &NameString, OBJ_CASE_INSENSITIVE, NULL, NULL);
    NtOpenSymbolicLinkObject(&LinkHandle, GENERIC_READ, &ObjectAttributes);//SYMBOLIC_LINK_QUERY

    LinkString.Buffer = NameBuffer;
    LinkString.MaximumLength = sizeof(NameBuffer);
    NtQuerySymbolicLinkObject(LinkHandle, &LinkString, &ActualLength);//LinkString������Ҫ��ֵ.
    //KdPrint(("KnownDllPath: %wZ \n", &LinkString));

    CloseHandle(LinkHandle);
}


//////////////////////////////////////////////////////////////////////////////////////////////////


/*
GetHandleInformation
SetHandleInformation
ZwDuplicateObject
ZwDuplicateToken
*/


DWORD CALLBACK ThreadProc(PVOID pvParam)
{
    HANDLE hMutex = (HANDLE)pvParam;

    // Perform work here, closing the handle when finished with the
    // mutex. If the reference count is zero, the object is destroyed.
    CloseHandle(hMutex);
    return 0;
}


int DuplicateHandleTest()
/*
Examples
The following example creates a mutex, duplicates a handle to the mutex, and passes it to another thread.
Duplicating the handle ensures that the reference count is increased so that the mutex object will not be destroyed until both threads have closed the handle.

https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle
*/
{
    HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
    HANDLE hMutexDup, hThread;
    DWORD dwThreadId;

    DuplicateHandle(GetCurrentProcess(),
                    hMutex,
                    GetCurrentProcess(),
                    &hMutexDup,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS);

    hThread = CreateThread(NULL, 0, ThreadProc, (LPVOID)hMutexDup, 0, &dwThreadId);

    // Perform work here, closing the handle when finished with the
    // mutex. If the reference count is zero, the object is destroyed.
    CloseHandle(hMutex);

    // Wait for the worker thread to terminate and clean up.
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    return 0;
}


#pragma region Application Family or OneCore Family
#if 0
void CompareObjectHandlesTest()
/*
Compares two object handles to determine if they refer to the same underlying kernel object.

Examples
The following code sample creates three handles, two of which refer to the same object,
then compares them to show that identical underlying kernel objects will return TRUE,
while non-identical objects will return FALSE.

https://docs.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-compareobjecthandles
*/
{
    HANDLE Event1;
    HANDLE Event2;
    HANDLE Event3;

    // Create a handle to a named event.
    Event1 = CreateEventW(NULL, TRUE, FALSE, L"{75A520B7-2C11-4809-B43A-0D31FB1FDD19}");
    if (Event1 == NULL) { ExitProcess(0); }

    // Create a handle to the same event.
    Event2 = CreateEventW(NULL, TRUE, FALSE, L"{75A520B7-2C11-4809-B43A-0D31FB1FDD19}");
    if (Event2 == NULL) { ExitProcess(0); }

    // Create a handle to an anonymous, unnamed event.
    Event3 = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (Event3 == NULL) { ExitProcess(0); }

    // Compare two handles to the same kernel object.
    if (CompareObjectHandles(Event1, Event2) != FALSE) {	// This message should be printed by the program.
        wprintf(L"Event1 and Event2 refer to the same underlying event object.\n");
    }

    // Compare two handles to different kernel objects.
    if (CompareObjectHandles(Event1, Event3) == FALSE) {	// This message should be printed by the program.
        wprintf(L"Event1 and Event3 refer to different underlying event objects.  (Error %lu)\n", GetLastError());
    }

    // Compare two handles to different kernel objects, each of a different type of kernel object.
    // The comparison is legal to make, though the function will always return FALSE and indicate 
    // a last error status of ERROR_NOT_SAME_OBJECT.
    if (CompareObjectHandles(Event1, GetCurrentProcess()) == FALSE) {	// This message should be printed by the program.
        wprintf(L"Event1 and the current process refer to different underlying kernel objects.  (Error %lu)\n", GetLastError());
    }
}
#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM) */
#pragma endregion


//////////////////////////////////////////////////////////////////////////////////////////////////

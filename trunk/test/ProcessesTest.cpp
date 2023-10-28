#include "ProcessesTest.h"


BOOL WINAPI EnumeratingProcessesCallBackTest(_In_ DWORD Pid, _In_opt_ PVOID Context)
{
    printf("pid:%d\n", Pid);

    return TRUE;
}


BOOL WINAPI  EnumerateProcessCallBackTest(_In_ PVOID lppe, /*ʵ��������LPPROCESSENTRY32W*/
                                          _In_opt_ PVOID Context)
{
    LPPROCESSENTRY32W pe32 = (LPPROCESSENTRY32W)lppe;

    printf("Pid:%08d\tExeFile:%-50ls\n", pe32->th32ProcessID, pe32->szExeFile);

    return TRUE;
}


void StartProtectProcess()
/*
�����ܱ������ӽ���
�µİ�ȫģ�ͻ������ܷ�������������ķ��������ܱ������ӽ��̡� ��Щ�ӽ��̽����븸������ͬ�ı������������У�
������������ļ�����ʹ��ͨ�� ELAM ��Դ����ע�����֤ͬ�����ǩ����

Ϊ��������������������������ܱ������ӽ��̣��ѹ����µ���չ������Կ ��PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL����
���ұ����� UpdateProcThreadAttribute API һ��ʹ�á�
ָ�� PROTECTION_LEVEL_SAME ����ֵ��ָ����봫�ݵ� UpdateProcThreadAttribute API �С�

ע�⣺

Ϊ��ʹ�ô������ԣ����񻹱����� CreateProcess ���õĽ��̴�����־������ָ��CREATE_PROTECTED_PROCESS��
��Ҫʹ�� /ac ���ضԷ���������ļ�����ǩ�����԰�������֤�飬�Խ������ӵ���֪ CA��
δ��ȷ���ӵ���֪�� CA ����ǩ��֤�齫�������á�
����ʾ����

https://learn.microsoft.com/zh-cn/windows/win32/services/protecting-anti-malware-services-
*/
{
    DWORD ProtectionLevel = PROTECTION_LEVEL_SAME;//�����ɹ��ˣ������Ǳ����Ľ��̣���������ǩ������
    //DWORD ProtectionLevel = PROTECTION_LEVEL_WINTCB;//����ʧ�ܣ�����ֵ��0x00000241��
    SIZE_T AttributeListSize = 0;
    DWORD Result;
    PROCESS_INFORMATION ProcessInformation = {0};
    STARTUPINFOEXW StartupInfoEx = {0};

    StartupInfoEx.StartupInfo.cb = sizeof(StartupInfoEx);

    InitializeProcThreadAttributeList(nullptr, 1, 0, &AttributeListSize);
    StartupInfoEx.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, AttributeListSize);
    _ASSERTE(StartupInfoEx.lpAttributeList);
    if (InitializeProcThreadAttributeList(StartupInfoEx.lpAttributeList, 1, 0, &AttributeListSize) == FALSE) {
        Result = GetLastError();
        goto exitFunc;
    }

    if (UpdateProcThreadAttribute(StartupInfoEx.lpAttributeList,
                                  0,
                                  PROC_THREAD_ATTRIBUTE_PROTECTION_LEVEL,
                                  &ProtectionLevel,
                                  sizeof(ProtectionLevel),
                                  nullptr,
                                  nullptr) == FALSE) {
        Result = GetLastError();
        goto exitFunc;
    }

    if (CreateProcessW(L"c:\\windows\\system32\\cmd.exe",
                       nullptr,
                       nullptr,
                       nullptr,
                       FALSE,
                       EXTENDED_STARTUPINFO_PRESENT | CREATE_PROTECTED_PROCESS,
                       nullptr,
                       nullptr,
                       (LPSTARTUPINFOW)&StartupInfoEx,
                       &ProcessInformation) == FALSE) {
        Result = GetLastError();
        goto exitFunc;
    }

exitFunc:
    if (ProcessInformation.hProcess){
        CloseHandle(ProcessInformation.hProcess);
    }
    
    if (ProcessInformation.hThread){
        CloseHandle(ProcessInformation.hThread);
    }    
}


void TestProcess()
{
    //EnumeratingAllProcesses(EnumeratingProcessesCallBackTest, nullptr);

    EnumerateProcess(EnumerateProcessCallBackTest, nullptr);


}

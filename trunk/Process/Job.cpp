#include "pch.h"
#include "Job.h"


#pragma warning (disable: 4996)
#pragma warning (disable: 6387)


//////////////////////////////////////////////////////////////////////////////////////////////////


HANDLE CreateJob()
{
    UUID Uuid = {0};

    RPC_STATUS STATUS = UuidCreate(&Uuid);
    if (RPC_S_OK != STATUS) {
        return INVALID_HANDLE_VALUE;
    }

    RPC_WSTR StringUuid = NULL;
    STATUS = UuidToString(&Uuid, &StringUuid);
    if (RPC_S_OK != STATUS) {
        return INVALID_HANDLE_VALUE;
    }

    /*
    �����������L"Global\\Job\\test"�����صĴ��������������ϵͳ�Ҳ���ָ����·����
    */
    WCHAR JobName[MAX_PATH] = {0};

    wsprintf(JobName, L"Global\\TestJob_{%s}", StringUuid);

    /*
    SECURITY_ATTRIBUTES�����òο���Creating a Child Process with Redirected Input and Output
    https://msdn.microsoft.com/zh-cn/library/windows/desktop/ms682499%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
    */
    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    /*
    1.ֻ�������������������Ҳ���Ǵ������Ľ��̡�
    2.If lpJobAttributes is NULL, the job object gets a default security descriptor and the handle cannot be inherited.
      �������ӽ����ǲ���̳�JOB����ġ�
    3.��ʹ������SECURITY_ATTRIBUTES����SetHandleInformation + HANDLE_FLAG_INHERIT������Ч�ġ�
    ����ֻ��ʹ�ã�IsProcessInJob�ˡ�
    */
    HANDLE hjob = CreateJobObject(&sa, JobName);//
    if (NULL == hjob) {
        int x = GetLastError();
    }

    /*
    �������÷��μ���
    https://msdn.microsoft.com/en-us/library/windows/desktop/ms724935(v=vs.85).aspx
    https://support.microsoft.com/de-ch/kb/315939/zh-cn
    */
    BOOL B = SetHandleInformation(hjob, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    if (0 == B) {
        int x = GetLastError();
    }

    /*
    ���гɹ��ˣ������ӽ�������̵ȶ�û�м̳���������Ҳ����˵û�п����������
    */

    STATUS = RpcStringFree(&StringUuid);
    if (RPC_S_OK != STATUS) {
        return INVALID_HANDLE_VALUE;
    }

    return hjob;
}


int TestJob(int argc, char * argv[])
/*
Ŀ�ģ�����JOB����ع��ܺ��÷���

made by correy
made at 2015.07.10
*/
{
    //DebugBreak();

    HANDLE hjob = CreateJob();
    if (INVALID_HANDLE_VALUE == hjob) {
        return 0;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Start the child process. 
    if (!CreateProcess(L"c:\\windows\\system32\\calc.exe" /*���Ӧ���б�׼�Ļ�ȡ�İ취�������ǲ��ԡ�*/, 
                       NULL,        // Command line
                       NULL,           // Process handle not inheritable
                       NULL,           // Thread handle not inheritable
                       FALSE,          // Set handle inheritance to FALSE
                       0,              // No creation flags
                       NULL,           // Use parent's environment block
                       NULL,           // Use parent's starting directory 
                       &si,            // Pointer to STARTUPINFO structure
                       &pi)           // Pointer to PROCESS_INFORMATION structure
        ) {
        printf("CreateProcess failed (%d)\n", GetLastError());
        return 0;
    }

    BOOL B = AssignProcessToJobObject(hjob, pi.hProcess);//�������û�п���JOB����
    if (0 == B) {
        int x = GetLastError();
    }

    BOOL Result = false;
    B = IsProcessInJob(pi.hProcess, hjob, &Result);
    if (0 == B) {
        int x = GetLastError();
        printf("failed (%d)\n", GetLastError());
    }

    // Start the child process. 
    if (!CreateProcess(L"c:\\windows\\system32\\cmd.exe" /*���Ӧ���б�׼�Ļ�ȡ�İ취�������ǲ��ԡ�*/,
                       NULL,        // Command line
                       NULL,           // Process handle not inheritable
                       NULL,           // Thread handle not inheritable
                       FALSE,          // Set handle inheritance to FALSE
                       0,              // No creation flags
                       NULL,           // Use parent's environment block
                       NULL,           // Use parent's starting directory 
                       &si,            // Pointer to STARTUPINFO structure
                       &pi)           // Pointer to PROCESS_INFORMATION structure
        ) {
        printf("CreateProcess failed (%d)\n", GetLastError());
        return 0;
    }

    B = AssignProcessToJobObject(hjob, pi.hProcess);//�������û�п���JOB����
    if (0 == B) {
        int x = GetLastError();
    }

    Result = false;
    B = IsProcessInJob(pi.hProcess, hjob, &Result);
    if (0 == B) {
        int x = GetLastError();
        printf("failed (%d)\n", GetLastError());
    }

    /*
    �����������һЩ�������紴���ӽ��̣�����̣����ӽ��̣��������������ӽ��̵Ľ��̣���
    ���۲죬JOB�ľ��û�м̳�/���ݡ����������õ����⣨ SECURITY_ATTRIBUTES ����
    ������ֻ���Լ��ֶ������ˣ�AssignProcessToJobObject����
    */

    /*
    ������������ǣ���ɱ�����е�����ĳ��Job�Ľ��̡�
    ����SetHandleInformation����Ϊ�ɼ̳У��������û�п�����������
    ���ǻ��ǰ����еģ�����JOB�����̸������ˣ����۶���û�С�
    */
    B = TerminateJobObject(hjob, 0);//STILL_ACTIVE
    if (0 == B) {
        int x = GetLastError();
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hjob);

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

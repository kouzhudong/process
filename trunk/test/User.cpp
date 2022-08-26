#include "User.h"


int SidTest(int argc, _TCHAR * argv[])
/*
sidһ�����صĶ���,�����ǻ�ȡ����ö���û������Ĺ�ϵ.
������������΢�������ĺ���,
һ���ǴӾ�����sid,�������������,�ѵ�����ʹ�õ�����.
һ���Ǵ�(�û�)���ֻ�ȡsid.����������Ǻõ�.
������Ҫ������������:GetTokenInformation,LookupAccountNameW
��Ϊ��GetTokenInformation�ĺ�����ȡ�Ķ��������е�����,���Դ��ľ�����Ϊ:LookupAccountName.Cpp.
*/
{
    wchar_t sz_UserNamew[260] = {0};
    int len = ARRAYSIZE(sz_UserNamew);
    GetUserName(sz_UserNamew, (LPDWORD)&len);

    LPWSTR * wsz_sid = (LPWSTR *)HeapAlloc(GetProcessHeap(), 0, 0x200);
    _ASSERTE(wsz_sid);

    PSID * ppSid = (PSID *)HeapAlloc(GetProcessHeap(), 0, 0x200);
    _ASSERTE(ppSid);

    GetSid(sz_UserNamew, ppSid);//Administrator,DefaultapppoolӦ����ö�ٵİ취.NetUserEnum,����ȫ.�����û��.
    bool  b = ConvertSidToStringSid(*ppSid, (LPWSTR *)wsz_sid);
    int x = GetLastError();
    //MessageBox(0, (LPCWSTR)(*(int *)wsz_sid), 0, 0);

    RtlZeroMemory(wsz_sid, 0x200);
    RtlZeroMemory(ppSid, 0x200);

    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY_SOURCE | TOKEN_QUERY, &hToken))
        return(FALSE);

    GetLogonSID(hToken, ppSid);//������˼�ǵ�¼��sid,�õ��ǵ�ǰ���̻����̵߳ľ��.
    b = ConvertSidToStringSid(*ppSid, (LPWSTR *)wsz_sid);
    x = GetLastError();

    //�õ������ֵ��ע������Ҳ���.HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList
    //MessageBox(0, (LPCWSTR)(*(int *)wsz_sid), 0, 0);

    HeapFree(GetProcessHeap(), 0, wsz_sid);
    HeapFree(GetProcessHeap(), 0, ppSid);

    SearchTokenGroupsForSID();

    //////////////////////////////////////////////////////////////////////////////////////////////////

    hToken = NULL;
    if (GetCurrentToken(&hToken) == false) {
        return 0;
    }

    PSID pSid = NULL;
    if (!GetLogonSID(hToken, &pSid)) {
        return 0;
    }

    LPTSTR StringSid;
    if (ConvertSidToStringSid(pSid, &StringSid) == 0) {
        //������������ͷź�����
    }

    if (LocalFree(StringSid) != NULL) {
        int x = GetLastError();
    }

    if (pSid) {
        FreeLogonSID(&pSid);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    return 0;
}

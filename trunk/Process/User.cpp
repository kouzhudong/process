#include "pch.h"
#include "User.h"

#include <LsaLookup.h>
#include <NTSecAPI.h>

#include <ntsecapi.h>

#pragma warning(disable:6011)
#pragma warning(disable:6387)
#pragma warning(disable:4996)
#pragma warning(disable:28159)
#pragma warning(disable:26451)


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
BOOL WINAPI IsUserAdmin(VOID)
/*++
Routine Description: This routine returns TRUE if the caller's
process is a member of the Administrators local group. Caller is NOT
expected to be impersonating anyone and is expected to be able to
open its own process and process token.
Arguments: None.
Return Value:
   TRUE - Caller has Administrators local group.
   FALSE - Caller does not have Administrators local group. --

Examples
The following example shows checking a token for membership in the Administrators local group.

ע�⻹�и������У�IsUserAnAdmin

����һЩ�ж��ǲ��ǹ���Ա��С���룬û��ϸ���������ܶࡣ
ԭ��һ����������ʵ�֣��ؼ�����֪ʶ��֪ʶ�棬������
������������ȡ��msdn�����������ڵĻ�����΢��ġ�
����ϸ��ע���£�
���жϵ�ǰ�û��ǲ��ǹ���Ա����ĳ�Ա������һ����administrator�û�.
��win 7�·�administrator������û�����administrator��ĳ�Ա��Ȩ�����У���Ҫ�������������������ۡ�

https://docs.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-checktokenmembership
https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-isuseranadmin
*/
{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    b = AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdministratorsGroup);
    if (b) {
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
            b = FALSE;
        }
        FreeSid(AdministratorsGroup);
    }

    return(b);
}


EXTERN_C
__declspec(dllexport)
BOOL WINAPI IsUserAnSystem()
//BOOL WINAPI CurrentUserIsLocalSystem()
/*
���ܣ��жϽ����Ƿ�������NT AUTHORITY\SYSTEM�û��¡�

https://www.cnblogs.com/idebug/p/11124664.html
*/
{
    BOOL bIsLocalSystem = FALSE;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID psidLocalSystem;

    BOOL fSuccess = ::AllocateAndInitializeSid(&ntAuthority, 1, SECURITY_LOCAL_SYSTEM_RID,
                                               0, 0, 0, 0, 0, 0, 0, &psidLocalSystem);
    if (fSuccess) {
        fSuccess = ::CheckTokenMembership(0, psidLocalSystem, &bIsLocalSystem);
        ::FreeSid(psidLocalSystem);
    }

    return bIsLocalSystem;
}


EXTERN_C
__declspec(dllexport)
BOOL WINAPI IsCurrentUserLocalAdministrator(void)
/*
IsCurrentUserLocalAdministrator ()

This function checks the token of the calling thread to see if the caller
belongs to the Administrators group.

Return Value:
   TRUE if the caller is an administrator on the local machine.
   Otherwise, FALSE.

��Ҫ
��Ҫȷ���Ƿ��ڱ��ع���Ա�ʻ�������һ���̣߳������������߳�������ķ������ơ����Ľ������ִ�д˲�����

Windows 2000 �����߰汾��������ʹ��CheckTokenMembership() API ����������ƪ�����н��ܵĲ��衣
�й�������Ϣ������� Microsoft ƽ̨ SDK �ĵ���
��ϸ��Ϣ
Ĭ������£����߳�������ı���ǽ��̵��������"�û�������"��ȡ���κ�ֱ�����ӵ����̵߳ı�ʶ��
��ˣ�Ҫȷ���̵߳��û������ģ���Ӧ���ȳ��Ի�ȡ����OpenThreadToken�������߳����ơ�
����˷���ʧ�ܲ���ʱ���������� ERROR_NO_TOKEN��Ȼ��������ʹ��OpenProcessToken�����Ľ������ơ�

��ȡ��ǰ�û�������֮�󣬿���ʹ�÷���Ȩ�޼�鹦��������û��Ƿ�Ϊ����Ա����Ҫִ�д˲������밴�����в��������
ͨ��ʹ��AllocateAndInitializeSid�����������ع���Ա��İ�ȫ��ʶ��(SID)��
�����µİ�ȫ������(SD) �����ɷ��ʿ����б�(DACL)�����а������ʿ�����(ACE) �Ĺ���Ա��� SID��
�����뵱ǰ�û����¹���� SD��������û��Ƿ�Ϊ����Ա���Ʒ���Ȩ�޼�顣
����Ĵ���ʾ��ʹ���������Ƿ�ǰ�߳����б��ؼ�����ϵĹ���Ա���û���Ϊ������ǰ���ᵽ�ĺ�����
ʾ������

https://support.microsoft.com/zh-cn/kb/118626
*/
{
    BOOL   fReturn = FALSE;
    DWORD  dwStatus;
    DWORD  dwAccessMask;
    DWORD  dwAccessDesired;
    DWORD  dwACLSize;
    DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
    PACL   pACL = NULL;
    PSID   psidAdmin = NULL;

    HANDLE hToken = NULL;
    HANDLE hImpersonationToken = NULL;

    PRIVILEGE_SET   ps;
    GENERIC_MAPPING GenericMapping;

    PSECURITY_DESCRIPTOR     psdAdmin = NULL;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    /*
       Determine if the current thread is running as a user that is a member of
       the local admins group.  To do this, create a security descriptor that
       has a DACL which has an ACE that allows only local aministrators access.
       Then, call AccessCheck with the current thread's token and the security
       descriptor.  It will say whether the user could access an object if it
       had that security descriptor.  Note: you do not need to actually create
       the object.  Just checking access against the security descriptor alone
       will be sufficient.
    */
    const DWORD ACCESS_READ = 1;
    const DWORD ACCESS_WRITE = 2;

    __try {
        /*
           AccessCheck() requires an impersonation token.  We first get a primary
             token and then create a duplicate impersonation token.  The
             impersonation token is not actually assigned to the thread, but is
             used in the call to AccessCheck.  Thus, this function itself never
             impersonates, but does use the identity of the thread.  If the thread
             was impersonating already, this function uses that impersonation context.
          */
        if (!OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE | TOKEN_QUERY, TRUE, &hToken)) {
            if (GetLastError() != ERROR_NO_TOKEN)
                __leave;

            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &hToken))
                __leave;
        }

        if (!DuplicateToken(hToken, SecurityImpersonation, &hImpersonationToken))
            __leave;

        /*
          Create the binary representation of the well-known SID that
          represents the local administrators group.  Then create the security
            descriptor and DACL with an ACE that allows only local admins access.
            After that, perform the access check.  This will determine whether
            the current user is a local admin.
          */
        if (!AllocateAndInitializeSid(&SystemSidAuthority, 2,
                                      SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_ADMINS,
                                      0, 0, 0, 0, 0, 0, &psidAdmin))
            __leave;

        psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (psdAdmin == NULL)
            __leave;

        if (!InitializeSecurityDescriptor(psdAdmin, SECURITY_DESCRIPTOR_REVISION))
            __leave;

        // Compute size needed for the ACL.
        dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psidAdmin) - sizeof(DWORD);
        pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
        if (pACL == NULL)
            __leave;

        if (!InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
            __leave;

        dwAccessMask = ACCESS_READ | ACCESS_WRITE;
        if (!AddAccessAllowedAce(pACL, ACL_REVISION2, dwAccessMask, psidAdmin))
            __leave;

        if (!SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE))
            __leave;

        /*
           AccessCheck validates a security descriptor somewhat; set the group
             and owner so that enough of the security descriptor is filled out to
             make AccessCheck happy.
          */
        SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE);
        SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE);

        if (!IsValidSecurityDescriptor(psdAdmin))
            __leave;

        dwAccessDesired = ACCESS_READ;

        /*
           Initialize GenericMapping structure even though you do not use generic rights.
        */
        GenericMapping.GenericRead = ACCESS_READ;
        GenericMapping.GenericWrite = ACCESS_WRITE;
        GenericMapping.GenericExecute = 0;
        GenericMapping.GenericAll = ACCESS_READ | ACCESS_WRITE;

        if (!AccessCheck(psdAdmin, hImpersonationToken, dwAccessDesired,
                         &GenericMapping, &ps, &dwStructureSize, &dwStatus,
                         &fReturn)) {
            fReturn = FALSE;
            __leave;
        }
    } __finally {
        // Clean up.
        if (pACL) LocalFree(pACL);
        if (psdAdmin) LocalFree(psdAdmin);
        if (psidAdmin) FreeSid(psidAdmin);
        if (hImpersonationToken) CloseHandle(hImpersonationToken);
        if (hToken) CloseHandle(hToken);
    }

    return fReturn;
}


void TestIsCurrentUserLocalAdministrator()
{
    if (IsCurrentUserLocalAdministrator())
        printf("You are an administrator\n");
    else
        printf("You are not an administrator\n");
}


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
int WINAPI EnumCred()
/*
���ܣ���ȡ���������е�ƾ�ݣ������û��������룩�����Ҳ���Ҫ���������û������롣
*/
{
    DWORD Count = 0;
    PCREDENTIALW * Credential;
    BOOL ret = CredEnumerateW(NULL, 0, &Count, &Credential);
    _ASSERTE(ret);

    printf("Count:%d.\n", Count);
    printf("\n");

    for (DWORD x = 0; x < Count; x++) {
        printf("��%d����Ϣ��\n", x + 1);

        printf("TargetName:%ls.\n", Credential[x]->TargetName);

        if (lstrlen(Credential[x]->TargetAlias)) {
            printf("TargetAlias:%ls.\n", Credential[x]->TargetAlias);
        }

        if (lstrlen(Credential[x]->Comment)) {
            printf("Comment:%ls.\n", Credential[x]->Comment);
        }

        if (Credential[x]->Attributes) {
            printf("Keyword:%ls.\n", Credential[x]->Attributes->Keyword);
            printf("Value:%ls.\n", (LPWSTR)Credential[x]->Attributes->Value);
        }

        printf("UserName:%ls.\n", Credential[x]->UserName);
        printf("CredentialBlobSize(�ֽڣ����ַ�����Զ�):%d, CredentialBlob(����):%ls.\n",
               Credential[x]->CredentialBlobSize,
               (LPWSTR)Credential[x]->CredentialBlob);
        printf("\n");
    }

    CredFree(Credential);

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


BOOL GetAccountSid(LPTSTR SystemName, LPTSTR AccountName, PSID * Sid)
/*++
This function attempts to obtain a SID representing the supplied account on the supplied system.

If the function succeeds, the return value is TRUE.
A buffer is allocated which contains the SID representing the supplied account.
This buffer should be freed when it is no longer needed by calling HeapFree(GetProcessHeap(), 0, buffer)

If the function fails, the return value is FALSE. Call GetLastError() to obtain extended error information.
--*/
{
    LPTSTR ReferencedDomain = NULL;
    LPTSTR TempReferencedDomain = NULL;
    LPTSTR TempSid = NULL;
    DWORD cbSid = 128;    // initial allocation attempt
    DWORD cchReferencedDomain = 16; // initial allocation size
    SID_NAME_USE peUse;
    BOOL bSuccess = FALSE; // assume this function will fail

    __try {
        *Sid = (PSID)HeapAlloc(GetProcessHeap(), 0, cbSid);// initial memory allocations
        if (*Sid == NULL) __leave;

        ReferencedDomain = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, cchReferencedDomain * sizeof(TCHAR));
        if (ReferencedDomain == NULL) __leave;

        // Obtain the SID of the specified account on the specified system.
        while (!LookupAccountName(
            SystemName,         // machine to lookup account on
            AccountName,        // account to lookup
            *Sid,               // SID of interest
            &cbSid,             // size of SID
            ReferencedDomain,   // domain account was found on
            &cchReferencedDomain, &peUse)) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                TempSid = (LPTSTR)HeapReAlloc(GetProcessHeap(), 0, *Sid, cbSid);// reallocate memory
                if (TempSid == NULL) __leave;
                *Sid = TempSid;

                TempReferencedDomain = (LPTSTR)HeapReAlloc(GetProcessHeap(),
                                                           0,
                                                           ReferencedDomain,
                                                           cchReferencedDomain * sizeof(TCHAR));

                if (TempReferencedDomain == NULL) __leave;
                ReferencedDomain = TempReferencedDomain;
            } else __leave;
        }

        bSuccess = TRUE;// Indicate success.
    } // try
    __finally {// Cleanup and indicate failure, if appropriate.
        HeapFree(GetProcessHeap(), 0, ReferencedDomain);
        if (!bSuccess) {
            if (*Sid != NULL) {
                HeapFree(GetProcessHeap(), 0, *Sid);
                *Sid = NULL;
            }
        }
    } // finally

    return bSuccess;
}


void InitLsaString(PLSA_UNICODE_STRING LsaString, LPWSTR String)
{
    DWORD StringLength;

    if (String == NULL) {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = lstrlenW(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT)StringLength * sizeof(WCHAR);
    LsaString->MaximumLength = (USHORT)(StringLength + 1) * sizeof(WCHAR);
}


NTSTATUS OpenPolicy(LPWSTR ServerName, DWORD DesiredAccess, PLSA_HANDLE PolicyHandle)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;

    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));// Always initialize the object attributes to all zeroes.
    if (ServerName != NULL) {// Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        InitLsaString(&ServerString, ServerName);
        Server = &ServerString;
    } else {
        Server = NULL;
    }

    return LsaOpenPolicy(Server, &ObjectAttributes, DesiredAccess, PolicyHandle);// Attempt to open the policy.
}


EXTERN_C
__declspec(dllexport)
int WINAPI EnumerateAccountRights(int argc, char * argv[])
/*
���ܣ�ö���û������û������Ȩ��Ϣ���磺administrator����administrators.

���̼̳��û���Ȩ�ޡ�
����Ա���֪����������Ȩ��
�����еĽ��̵�ĳЩȨ���ǲ��׸㵽�ġ�
�磺Note that your process must have the SE_ASSIGNPRIMARYTOKEN_NAME and
SE_INCREASE_QUOTA_NAME privileges for successful execution of CreateProcessAsUser.
����취����LsaAddAccountRights/LsaRemoveAccountRights���û����/ɾ��Ȩ�ޡ�
Ȼ�����ó��õİ취��Ȩ��
������Ҫ��ע��/����ϵͳ��

�ǵ��и��취�ǲ��������ģ��������˷����������ˡ�
���Խ��Ǵ��ġ�
���ĵĹ�����ö���û������û����Ȩ�޵ġ�
�����޸��ԣ�Microsoft SDKs\Windows\v6.0\Samples\Security\LSAPolicy\LsaPrivs\LsaPrivs.c��

ע�⣺Unlike privileges, however, account rights are not supported by the LookupPrivilegeValue and LookupPrivilegeName functions.
�û�����Ȩ����Ҫ���磺
�޸Ĺ̼�����ֵ��
����������
�������񣬷���һ�㲻����ͨ�û����еġ�
�Բ���ϵͳ��ʽִ�С�
������Ϣ���뿴��
http://msdn.microsoft.com/en-us/library/windows/desktop/bb545671(v=vs.85).aspx  Account Rights Constants
http://msdn.microsoft.com/en-us/library/windows/desktop/bb530716(v=vs.85).aspx  Privilege Constants
http://www.microsoft.com/technet/prodtechnol/WindowsServer2003/Library/IIS/08bc7712-548c-4308-a49c-d551a4b5e245.mspx?mfr=true
�ȵȡ�

����Ĺ����뿴��
LsaEnumerateAccountsWithUserRight
LsaEnumerateTrustedDomains
LsaEnumerateTrustedDomainsEx
NetEnumerateServiceAccounts ��Windows 7/Windows Server 2008 R2��

made by correy
made at 2014.06.14
*/
{
    LSA_HANDLE PolicyHandle;
    WCHAR wComputerName[256] = L"";   // static machine name buffer
    TCHAR AccountName[256];         // static account name buffer
    PSID pSid;
    NTSTATUS Status;
    int iRetVal = RTN_ERROR;          // assume error from main

    if (argc == 1) {
        fprintf(stderr, "Usage: %s <Account> [TargetMachine]\n", argv[0]);
        return RTN_USAGE;
    }

    // Pick up account name on argv[1].
    // Assumes source is ANSI. Resultant string is ANSI or Unicode
    //�������û�Ҳ�������û��飬�磺administrator����administrators.
    _snwprintf_s(AccountName, 256, 255, TEXT("%hS"), argv[1]);

    // Pick up machine name on argv[2], if appropriate
    // assumes source is ANSI. Resultant string is Unicode.
    if (argc == 3) _snwprintf_s(wComputerName, 256, 255, L"%hS", argv[2]);//������������ޡ�

    // Open the policy on the target machine. 
    Status = OpenPolicy(
        wComputerName,      // target machine
        POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
        &PolicyHandle       // resultant policy handle
    );
    if (Status != STATUS_SUCCESS) {
        DisplayNtStatus("OpenPolicy", Status);
        return RTN_ERROR;
    }

    // Obtain the SID of the user/group.
    // Note that we could target a specific machine, but we don't.
    // Specifying NULL for target machine searches for the SID in the following order: 
    // well-known, Built-in and local, primary domain,trusted domains.
    if (GetAccountSid(
        NULL,       // default lookup logic
        AccountName,// account to obtain SID
        &pSid       // buffer to allocate to contain resultant SID
    )) {
        //�⼸�д������Լ��ġ�
        PLSA_UNICODE_STRING UserRights;
        ULONG CountOfRights;
        Status = LsaEnumerateAccountRights(PolicyHandle, pSid, &UserRights, &CountOfRights);
        if (Status != STATUS_SUCCESS) {
            DisplayNtStatus("LsaEnumerateAccountRights", Status);
            return RTN_ERROR;
        }

        ULONG i = 0;
        for (PLSA_UNICODE_STRING plun = UserRights; i < CountOfRights; i++) {
            fprintf(stderr, "%ws!\n", plun->Buffer);
            plun++;
        }

        Status = LsaFreeMemory(&UserRights);
        if (Status != STATUS_SUCCESS) {
            DisplayNtStatus("LsaFreeMemory", Status);
            return RTN_ERROR;
        }

        iRetVal = RTN_OK;
    } else {
        DisplayWinError("GetAccountSid", GetLastError());// Error obtaining SID.
    }

    LsaClose(PolicyHandle);// Close the policy handle.
    if (pSid != NULL) HeapFree(GetProcessHeap(), 0, pSid);// Free memory allocated for SID.
    return iRetVal;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
BOOL WINAPI GetCurrentUserAndDomain(PTSTR szUser, PDWORD pcchUser, PTSTR szDomain, PDWORD pcchDomain)
/*
//  FUNCTION:     GetCurrentUserAndDomain - This function looks up the user name and
                  domain name for the user account associated with the calling thread.
//  PARAMETERS:   szUser - a buffer that receives the user name
//                pcchUser - the size, in characters, of szUser
//                szDomain - a buffer that receives the domain name
//                pcchDomain - the size, in characters, of szDomain
//  RETURN VALUE: TRUE if the function succeeds.
                  Otherwise, FALSE and GetLastError() will return the failure reason.
                  If either of the supplied buffers are too small,
                  GetLastError() will return ERROR_INSUFFICIENT_BUFFER and pcchUser and
                  pcchDomain will be adjusted to reflect the required buffer sizes.

�÷�ʾ����
    wchar_t szUser[MAX_PATH] = {0};//�������ֵ���������
    wchar_t szDomain[MAX_PATH] = {0};//�������ֵ����������������ͼ������һ����
    DWORD d = MAX_PATH;
    bool b = GetCurrentUserAndDomain(szUser, &d, szDomain, &d);

made by correy
made at 2013.05.03

����ժ�ԣ�// http://support.microsoft.com/kb/111544/zh-cn
*/
{
    BOOL         fSuccess = FALSE;
    HANDLE       hToken = NULL;
    PTOKEN_USER  ptiUser = NULL;
    DWORD        cbti = 0;
    SID_NAME_USE snu;

    __try {
        // Get the calling thread's access token.
        if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) {
            if (GetLastError() != ERROR_NO_TOKEN) {
                __leave;
            }

            // Retry against process token if no thread token exists.
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
                __leave;
            }
        }

        // Obtain the size of the user information in the token.    
        if (GetTokenInformation(hToken, TokenUser, NULL, 0, &cbti)) {
            __leave;// Call should have failed due to zero-length buffer.   
        } else {// Call should have failed due to zero-length buffer.
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                __leave;
            }
        }

        // Allocate buffer for user information in the token.
        ptiUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, cbti);
        if (!ptiUser) {
            __leave;
        }

        // Retrieve the user information from the token.
        if (!GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti)) {
            __leave;
        }

        // Retrieve user name and domain name based on user's SID.
        if (!LookupAccountSid(NULL, ptiUser->User.Sid, szUser, pcchUser, szDomain, pcchDomain, &snu)) {
            __leave;
        }

        fSuccess = TRUE;
    } __finally {// Free resources.        
        if (hToken) {
            CloseHandle(hToken);
        }

        if (ptiUser) {
            HeapFree(GetProcessHeap(), 0, ptiUser);
        }
    }

    return fSuccess;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
HRESULT WINAPI GetSid(LPCWSTR wszAccName, PSID * ppSid)
/*
http://msdn.microsoft.com/en-us/library/windows/desktop/ms707085(v=vs.85).aspx
https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms707085(v=vs.85)?redirectedfrom=MSDN
*/
{
    if (wszAccName == NULL || ppSid == NULL) {// Validate the input parameters.
        return MQ_ERROR_INVALID_PARAMETER;
    }

    // Create buffers that may be large enough.If a buffer is too small, the count parameter will be set to the size needed.
    const DWORD INITIAL_SIZE = 32;
    DWORD cbSid = 0;
    DWORD dwSidBufferSize = INITIAL_SIZE;
    DWORD cchDomainName = 0;
    DWORD dwDomainBufferSize = INITIAL_SIZE;
    WCHAR * wszDomainName = NULL;
    SID_NAME_USE eSidType;
    DWORD dwErrorCode = 0;
    HRESULT hr = MQ_OK;

    *ppSid = (PSID) new BYTE[dwSidBufferSize];// Create buffers for the SID and the domain name.
    if (*ppSid == NULL) {
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }
    memset(*ppSid, 0, dwSidBufferSize);
    wszDomainName = new WCHAR[dwDomainBufferSize];
    if (wszDomainName == NULL) {
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }
    memset(wszDomainName, 0, dwDomainBufferSize * sizeof(WCHAR));

    for (; ; )// Obtain the SID for the account name passed.
    {   // Set the count variables to the buffer sizes and retrieve the SID.
        cbSid = dwSidBufferSize;
        cchDomainName = dwDomainBufferSize;
        if (LookupAccountNameW(
            NULL,            // Computer name. NULL for the local computer
            wszAccName,
            *ppSid,          // Pointer to the SID buffer. Use NULL to get the size needed,
            &cbSid,          // Size of the SID buffer needed.
            wszDomainName,   // wszDomainName,//������ܻ�ȡ����.
            &cchDomainName,
            &eSidType)) //��ʵ����������Ƿ���sid�������õı��ûɶ,��Ҫ����,�����������,���ϸ�����.
        {
            if (IsValidSid(*ppSid) == FALSE) {
                wprintf(L"The SID for %s is invalid.\n", wszAccName);
                dwErrorCode = MQ_ERROR;
            }
            break;
        }
        dwErrorCode = GetLastError();

        if (dwErrorCode == ERROR_INSUFFICIENT_BUFFER) // Check if one of the buffers was too small.
        {
            if (cbSid > dwSidBufferSize) {   // Reallocate memory for the SID buffer.
                wprintf(L"The SID buffer was too small. It will be reallocated.\n");
                FreeSid(*ppSid);
                *ppSid = (PSID) new BYTE[cbSid];
                if (*ppSid == NULL) {
                    return MQ_ERROR_INSUFFICIENT_RESOURCES;
                }
                memset(*ppSid, 0, cbSid);
                dwSidBufferSize = cbSid;
            }
            if (cchDomainName > dwDomainBufferSize) {   // Reallocate memory for the domain name buffer.
                wprintf(L"The domain name buffer was too small. It will be reallocated.\n");
                delete[] wszDomainName;
                wszDomainName = new WCHAR[cchDomainName];
                if (wszDomainName == NULL) {
                    return MQ_ERROR_INSUFFICIENT_RESOURCES;
                }
                memset(wszDomainName, 0, cchDomainName * sizeof(WCHAR));
                dwDomainBufferSize = cchDomainName;
            }
        } else {
            wprintf(L"LookupAccountNameW failed. GetLastError returned: %d\n", dwErrorCode);
            hr = HRESULT_FROM_WIN32(dwErrorCode);
            break;
        }
    }

    delete[] wszDomainName;
    return hr;
}


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
    PSID * ppSid = (PSID *)HeapAlloc(GetProcessHeap(), 0, 0x200);

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


int GetCurrentSid()
/*
�ο�:Microsoft SDKs\Windows\v7.1\Samples\security\authorization\textsid�������.
��ȡ��ǰ�û�(���̵�)SID����.��ʵҲ����ô��.
made at 2013.10.10
*/
{
    HANDLE hToken;
    BYTE buf[MY_BUFSIZE];
    PTOKEN_USER ptgUser = (PTOKEN_USER)buf;
    DWORD cbBuffer = MY_BUFSIZE;
    BOOL bSuccess;

    // obtain current process token
    if (!OpenProcessToken(
        GetCurrentProcess(), // target current process
        TOKEN_QUERY,         // TOKEN_QUERY access
        &hToken              // resultant hToken
    )) {
        return 1;
    }

    // obtain user identified by current process' access token
    bSuccess = GetTokenInformation(
        hToken,    // identifies access token
        TokenUser, // TokenUser info type
        ptgUser,   // retrieved info buffer
        cbBuffer,  // size of buffer passed-in
        &cbBuffer  // required buffer size
    );
    CloseHandle(hToken);
    if (!bSuccess) {
        return 1;
    }

    LPWSTR lpSid = NULL;
    ConvertSidToStringSid(ptgUser->User.Sid, &lpSid);

    //��ʱ�Ѿ���ȡ����,���Բ鿴��.

    LocalFree(lpSid);

    return 1;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


BOOL IsRunAsAdmin()
//ժ�ԣ�UAC self-elevation (CppUACSelfElevation)
//   PURPOSE: The function checks whether the current process is run as administrator. 
//   In other words, it dictates whether the primary access token of the process belongs to user account that is a member of the local Administrators group and it is elevated.
//
//   RETURN VALUE: Returns TRUE if the primary access token of the process belongs to user account that is a member of the local Administrators group and it is elevated. 
//   Returns FALSE if the token does not.
//
//   EXCEPTION: If this function fails, it throws a C++ DWORD exception which contains the Win32 error code of the failure.
//
//   EXAMPLE CALL:
//     try 
//     {
//         if (IsRunAsAdmin())
//             wprintf (L"Process is run as administrator\n");
//         else
//             wprintf (L"Process is not run as administrator\n");
//     }
//     catch (DWORD dwError)
//     {
//         wprintf(L"IsRunAsAdmin failed w/err %lu\n", dwError);
//     }
{
    BOOL fIsRunAsAdmin = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    PSID pAdministratorsGroup = NULL;

    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdministratorsGroup)) {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Determine whether the SID of administrators group is enabled in the primary access token of the process.
    if (!CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin)) {
        dwError = GetLastError();
        goto Cleanup;
    }

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (pAdministratorsGroup) {
        FreeSid(pAdministratorsGroup);
        pAdministratorsGroup = NULL;
    }

    // Throw the error if something failed in the function.
    if (ERROR_SUCCESS != dwError) {
        throw dwError;
    }

    return fIsRunAsAdmin;
}


BOOL IsUserInAdminGroup()
//ժ�ԣ�UAC self-elevation (CppUACSelfElevation)
//   PURPOSE: The function checks whether the primary access token of the process belongs to user account that is a member of the local Administrators group,
//            even if it currently is not elevated.
//
//   RETURN VALUE: Returns TRUE if the primary access token of the process belongs to user account that is a member of the local Administrators group. 
//   Returns FALSE if the token does not.
//
//   EXCEPTION: If this function fails, it throws a C++ DWORD exception which contains the Win32 error code of the failure.
//
//   EXAMPLE CALL:
//     try 
//     {
//         if (IsUserInAdminGroup())
//             wprintf (L"User is a member of the Administrators group\n");
//         else
//             wprintf (L"User is not a member of the Administrators group\n");
//     }
//     catch (DWORD dwError)
//     {
//         wprintf(L"IsUserInAdminGroup failed w/err %lu\n", dwError);
//     }
{
    BOOL fInAdminGroup = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hToken = NULL;
    HANDLE hTokenToCheck = NULL;
    DWORD cbSize = 0;
    OSVERSIONINFO osver = {sizeof(osver)};

    // Open the primary access token of the process for query and duplicate.
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hToken)) {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Determine whether system is running Windows Vista or later operating systems (major version >= 6) because they support linked tokens,
    // but previous versions (major version < 6) do not.
    if (!GetVersionEx(&osver)) {
        dwError = GetLastError();
        goto Cleanup;
    }

    if (osver.dwMajorVersion >= 6) {
        // Running Windows Vista or later (major version >= 6). 
        // Determine token type: limited, elevated, or default. 
        TOKEN_ELEVATION_TYPE elevType;
        if (!GetTokenInformation(hToken, TokenElevationType, &elevType, sizeof(elevType), &cbSize)) {
            dwError = GetLastError();
            goto Cleanup;
        }

        // If limited, get the linked elevated token for further check.
        if (TokenElevationTypeLimited == elevType) {
            if (!GetTokenInformation(hToken, TokenLinkedToken, &hTokenToCheck, sizeof(hTokenToCheck), &cbSize)) {
                dwError = GetLastError();
                goto Cleanup;
            }
        }
    }

    // CheckTokenMembership requires an impersonation token. 
    // If we just got a linked token, it already is an impersonation token.  
    // If we did not get a linked token, duplicate the original into an impersonation token for CheckTokenMembership.
    if (!hTokenToCheck) {
        if (!DuplicateToken(hToken, SecurityIdentification, &hTokenToCheck)) {
            dwError = GetLastError();
            goto Cleanup;
        }
    }

    // Create the SID corresponding to the Administrators group.
    BYTE adminSID[SECURITY_MAX_SID_SIZE];
    cbSize = sizeof(adminSID);
    if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &adminSID, &cbSize)) {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Check if the token to be checked contains admin SID.
    // http://msdn.microsoft.com/en-us/library/aa379596(VS.85).aspx:
    // To determine whether a SID is enabled in a token, that is, 
    // whether it has the SE_GROUP_ENABLED attribute, call CheckTokenMembership.
    if (!CheckTokenMembership(hTokenToCheck, &adminSID, &fInAdminGroup)) {
        dwError = GetLastError();
        goto Cleanup;
    }

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (hToken) {
        CloseHandle(hToken);
        hToken = NULL;
    }

    if (hTokenToCheck) {
        CloseHandle(hTokenToCheck);
        hTokenToCheck = NULL;
    }

    // Throw the error if something failed in the function.
    if (ERROR_SUCCESS != dwError) {
        throw dwError;
    }

    return fInAdminGroup;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


BOOL IsEnableUAC()
/*
Determine whether the current user has opened UAC (Vista or Win7 OS)

https://titanwolf.org/Network/Articles/Article?AID=c55a10c3-265e-42cd-b520-118ca46c2c60
*/
{
    BOOL isEnableUAC = FALSE;
    OSVERSIONINFO osversioninfo;
    ZeroMemory(&osversioninfo, sizeof(osversioninfo));
    osversioninfo.dwOSVersionInfoSize = sizeof(osversioninfo);
    BOOL bSuccess = GetVersionEx(&osversioninfo);
    if (bSuccess) {
        //window vista or windows server 2008 or later operating system
        if (osversioninfo.dwMajorVersion > 5) {
            HKEY hKEY = NULL;
            DWORD dwType = REG_DWORD;
            DWORD dwEnableLUA = 0;
            DWORD dwSize = sizeof(DWORD);
            LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       TEXT("SOFTWARE//Microsoft//Windows//CurrentVersion//Policies//System//"),
                                       0,
                                       KEY_READ,
                                       &hKEY);
            if (ERROR_SUCCESS == status) {
                status = RegQueryValueEx(hKEY,
                                         TEXT("EnableLUA"),
                                         NULL,
                                         &dwType,
                                         (BYTE *)&dwEnableLUA,
                                         &dwSize);
                if (ERROR_SUCCESS == status) {
                    isEnableUAC = (dwEnableLUA == 1) ? TRUE : FALSE;
                }
                RegCloseKey(hKEY);
            }
        }
    }

    return isEnableUAC;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


#define TARGET_SYSTEM_NAME L"mysystem"


LSA_HANDLE GetPolicyHandle()
/*
�򿪲��Զ�����

����� LSA ���Ժ���Ҫ��ʹ�� ���� ����ľ������ѯ���޸�ϵͳ��
��Ҫ��ȡ ���� ����ľ��������� LsaOpenPolicy ��ָ��Ҫ���ʵ�ϵͳ�����ƺ�����ķ���Ȩ�޼���

Ӧ�ó�������ķ���Ȩ��ȡ������ִ�еĲ�����
�й�ÿ����������Ȩ�޵���ϸ��Ϣ������� LSA ���Ժ����иú�����˵����

����� LsaOpenPolicy �ĵ��óɹ���������Ϊָ��ϵͳ���� ���� ����ľ����
Ȼ��Ӧ�ó����ں��� LSA ���Ժ��������д��ݴ˾���� ��Ӧ�ó�������Ҫ���ʱ����Ӧ���� LsaClose ���ͷ�����

�����ʾ����ʾ��δ� ���� ��������

��ǰ���ʾ���У�Ӧ�ó���������� _ ���� _ ���� Ȩ�ޡ�
�йص��� LsaOpenPolicyʱӦ�ó���Ӧ�������Ȩ�޵���ϸ��Ϣ�������Ӧ�ó��� ���� ���������ݵ��ĺ�����˵����

��Ҫ����������� ���� ����ľ��������� LsaCreateTrustedDomainEx (������) �����µ����ι�ϵ������� LsaOpenTrustedDomainByName (�������е���������) ��
����������������һ��ָ�� LSA _ �����ָ�룬Ȼ���������ں��� LSA ���Ժ���������ָ���þ����
�� LsaOpenPolicyһ����Ӧ�ó����ڲ�����Ҫ��������� ���� ����ľ��ʱ��Ӧ���� LsaClose ��

https://docs.microsoft.com/zh-cn/windows/win32/secmgmt/opening-a-policy-object-handle
*/
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR SystemName[] = TARGET_SYSTEM_NAME;
    USHORT SystemNameLength;
    LSA_UNICODE_STRING lusSystemName;
    NTSTATUS ntsResult;
    LSA_HANDLE lsahPolicyHandle;

    // Object attributes are reserved, so initialize to zeros.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    //Initialize an LSA_UNICODE_STRING to the server name.
    SystemNameLength = (USHORT)wcslen(SystemName);
    lusSystemName.Buffer = SystemName;
    lusSystemName.Length = SystemNameLength * sizeof(WCHAR);
    lusSystemName.MaximumLength = (SystemNameLength + 1) * sizeof(WCHAR);

    // Get a handle to the Policy object.
    ntsResult = LsaOpenPolicy(
        &lusSystemName,    //Name of the target system.
        &ObjectAttributes, //Object attributes.
        POLICY_ALL_ACCESS, //Desired access permissions.
        &lsahPolicyHandle  //Receives the policy handle.
    );

    if (ntsResult != STATUS_SUCCESS) {
        // An error occurred. Display it as a win32 error code.
        wprintf(L"OpenPolicy returned %lu\n",
                LsaNtStatusToWinError(ntsResult));
        return NULL;
    }
    return lsahPolicyHandle;
}


void AddPrivileges(PSID AccountSID, LSA_HANDLE PolicyHandle)
/*
�����ʻ�Ȩ��

LSA �ṩһЩ������Ӧ�ó���ɵ�����Щ������ö�ٻ������û�����ͱ������ʻ��� ��Ȩ ��

���Ӧ�ó�������ñ��ز��Զ���ľ������ �򿪲��Զ����������������Ӧ�ó�������ñ��� ���Զ���ľ����
���⣬��Ҫö�ٻ�༭�ʻ���Ȩ�ޣ��������и��ʻ��� ��ȫ��ʶ�� (SID) ��
Ӧ�ó�����Ը��� ���ƺ� Sid ���ת�������������Ҹ������ʻ����� SID��

��Ҫ���ʾ����ض�Ȩ�޵������ʻ�������� LsaEnumerateAccountsWithUserRight��
�˺���ʹ�þ���ָ��Ȩ�޵������ʻ��� Sid ������顣

��ȡ�ʻ��� SID �󣬿����޸���Ȩ�ޡ� ���� LsaAddAccountRights ����Ȩ����ӵ��ʻ���
���ָ�����ʻ������ڣ� LsaAddAccountRights ���������ʻ���
��Ҫ���ʻ���ɾ��Ȩ�ޣ������ LsaRemoveAccountRights��
������ʻ���ɾ������Ȩ�ޣ��� LsaRemoveAccountRights Ҳ��ɾ�����ʻ���

Ӧ�ó������ͨ������ LsaEnumerateAccountRights����鵱ǰ������ʻ���Ȩ�ޡ�
�˺������ LSA _ UNICODE _ �ַ��� �ṹ�����顣
ÿ���ṹ������ָ���ʻ����е���Ȩ�����ơ�

�����ʾ���� SeServiceLogonRight Ȩ����ӵ��ʻ���
�ڴ�ʾ���У�AccountSID ����ָ�����ʻ��� SID��
�й���β����ʻ� SID ����ϸ��Ϣ������� ���ƺ� Sid ���ת����

https://docs.microsoft.com/zh-cn/windows/win32/secmgmt/managing-account-permissions
*/
{
    LSA_UNICODE_STRING lucPrivilege;
    NTSTATUS ntsResult;

    // Create an LSA_UNICODE_STRING for the privilege names.
    InitLsaString(&lucPrivilege, (LPWSTR)L"SeServiceLogonRight");

    ntsResult = LsaAddAccountRights(
        PolicyHandle,  // An open policy handle.
        AccountSID,    // The target SID.
        &lucPrivilege, // The privileges.
        1              // Number of privileges.
    );
    if (ntsResult == STATUS_SUCCESS) {
        wprintf(L"Privilege added.\n");
    } else {
        wprintf(L"Privilege was not added - %lu \n", LsaNtStatusToWinError(ntsResult));
    }
}


void GetSIDInformation(LPWSTR AccountName, LSA_HANDLE PolicyHandle)
/*
���ƺ� Sid ���ת��

���ذ�ȫ���� (LSA) �ṩ���û�����ͱ���������֮�����ת���Ĺ��ܣ��Լ� (SID) ֵ����Ӧ ��ȫ��ʶ����
��Ҫ�����ʻ����ƣ������ LsaLookupNames ������ �˺����� RID/�������Ե���ʽ���� SID��
��Ҫ�Ե���Ԫ�ص���ʽ��ȡ SID������� LsaLookupNames2 ������
��Ҫ���� Sid������� LsaLookupSids��

��Щ�������Դӱ���ϵͳ���ε��κ���ת�����ƺ� SID ��Ϣ��

���Ӧ�ó�������ñ��ز��Զ���ľ������ �򿪲��Զ����������������Ӧ�ó�������ñ��� ���Զ���ľ����

�����ʾ���ڸ����ʻ����Ƶ�����²����ʻ��� SID��

��ǰ���ʾ���У����� InitLsaString �� unicode �ַ���ת��Ϊ LSA _ unicode _ �ַ��� �ṹ��
�˺����Ĵ����� ʹ�� LSA Unicode �ַ�������ʾ��

 ��ע

��Щת��������Ҫ��Ȩ�ޱ༭��������ʾ ���ʿ����б� (ACL) ��Ϣ��
Ȩ�ޱ༭��Ӧʼ��ʹ�� name �� security identifier SID ����ϵͳ�� Policy���������Щ������
���ȷ����ת��������������ȷ���������򼯡�

Windows Access Control ���ṩ���� Sid ���ʻ���֮��ִ��ת���ĺ����� LookupAccountName �� LookupAccountSid��
������Ӧ�ó�����Ҫ�����ʻ����ƻ� SID������ʹ������ LSA ���Թ��ܣ���ʹ�� Windows Access Control ���ܣ������� LSA ���Ժ�����
�й���Щ��������ϸ��Ϣ������� ���ʿ��ơ�

https://docs.microsoft.com/zh-cn/windows/win32/secmgmt/translating-between-names-and-sids
*/
{
    LSA_UNICODE_STRING lucName;
    PLSA_TRANSLATED_SID ltsTranslatedSID;
    PLSA_REFERENCED_DOMAIN_LIST lrdlDomainList;
    LSA_TRUST_INFORMATION myDomain;
    NTSTATUS ntsResult;
    PWCHAR DomainString = NULL;

    // Initialize an LSA_UNICODE_STRING with the name.
    InitLsaString(&lucName, AccountName);

    ntsResult = LsaLookupNames(
        PolicyHandle,     // handle to a Policy object
        1,                // number of names to look up
        &lucName,         // pointer to an array of names
        &lrdlDomainList,  // receives domain information
        &ltsTranslatedSID // receives relative SIDs
    );
    if (STATUS_SUCCESS != ntsResult) {
        wprintf(L"Failed LsaLookupNames - %lu \n", LsaNtStatusToWinError(ntsResult));
        return;
    }

    // Get the domain the account resides in.
    myDomain = lrdlDomainList->Domains[ltsTranslatedSID->DomainIndex];
    DomainString = (PWCHAR)LocalAlloc(LPTR, myDomain.Name.Length + 1);
    _ASSERTE(DomainString);
    wcsncpy_s(DomainString,
              myDomain.Name.Length + 1,
              myDomain.Name.Buffer,
              myDomain.Name.Length);

    // Display the relative Id. 
    wprintf(L"Relative Id is %lu in domain %ws.\n", ltsTranslatedSID->RelativeId, DomainString);

    LocalFree(DomainString);
    LsaFreeMemory(ltsTranslatedSID);
    LsaFreeMemory(lrdlDomainList);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

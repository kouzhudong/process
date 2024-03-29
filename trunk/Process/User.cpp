#include "pch.h"
#include "User.h"
#include <lm.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <NTSecAPI.h>

#pragma warning(disable : 6011)
#pragma warning(disable : 6387)
#pragma warning(disable : 4996)
#pragma warning(disable : 28159)
#pragma warning(disable : 26451)


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

注意还有个函数叫：IsUserAnAdmin

看到一些判断是不是管理员的小代码，没有细看。方法很多。
原来一个函数就能实现，关键在于知识，知识面，技术？
下面这两个是取自msdn，觉得最正宗的还是用微软的。
再详细的注释下：
是判断当前用户是不是管理员的组的成员，并不一定是administrator用户.
在win 7下非administrator的组的用户，以administrator组的成员的权限运行，需要输入密码的情况，别当另论。

https://docs.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-checktokenmembership
https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-isuseranadmin
*/
{
    BOOL b{};
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup{};
    b = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
    if (b) {
        if (!CheckTokenMembership(nullptr, AdministratorsGroup, &b)) {
            b = FALSE;
        }
        FreeSid(AdministratorsGroup);
    }

    return (b);
}


EXTERN_C
__declspec(dllexport)
BOOL WINAPI IsUserAnSystem()
//BOOL WINAPI CurrentUserIsLocalSystem()
/*
功能：判断进程是否运行在NT AUTHORITY\SYSTEM用户下。

https://www.cnblogs.com/idebug/p/11124664.html
*/
{
    BOOL bIsLocalSystem = FALSE;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID psidLocalSystem;

    BOOL fSuccess = ::AllocateAndInitializeSid(&ntAuthority, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &psidLocalSystem);
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

概要
若要确定是否在本地管理员帐户下运行一个线程，您必须检查与线程相关联的访问令牌。本文介绍如何执行此操作。

Windows 2000 及更高版本，您可以使用CheckTokenMembership() API 而不是在这篇文章中介绍的步骤。
有关其他信息，请参阅 Microsoft 平台 SDK 文档。
详细信息
默认情况下，与线程相关联的标记是进程的其包含。"用户上下文"被取代任何直接连接到该线程的标识。
因此，要确定线程的用户上下文，您应首先尝试获取具有OpenThreadToken函数的线程令牌。
如果此方法失败并且时出错函数报告 ERROR_NO_TOKEN，然后您可以使用OpenProcessToken函数的进程令牌。

获取当前用户的令牌之后，可以使用访问权限检查功能来检测用户是否为管理员。若要执行此操作，请按照下列步骤操作：
通过使用AllocateAndInitializeSid函数创建本地管理员组的安全标识符(SID)。
构建新的安全描述符(SD) 的自由访问控制列表(DACL)，其中包含访问控制项(ACE) 的管理员组的 SID。
调用与当前用户和新构造的 SD，来检测用户是否为管理员令牌访问权限检查。
下面的代码示例使用来测试是否当前线程运行本地计算机上的管理员的用户作为本文中前面提到的函数。
示例代码

https://support.microsoft.com/zh-cn/kb/118626
*/
{
    BOOL fReturn = FALSE;
    DWORD dwStatus{};
    DWORD dwAccessMask{};
    DWORD dwAccessDesired{};
    DWORD dwACLSize{};
    DWORD dwStructureSize = sizeof(PRIVILEGE_SET);
    PACL pACL = nullptr;
    PSID psidAdmin = nullptr;

    HANDLE hToken = nullptr;
    HANDLE hImpersonationToken = nullptr;

    PRIVILEGE_SET ps{};
    GENERIC_MAPPING GenericMapping{};

    PSECURITY_DESCRIPTOR psdAdmin = nullptr;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    /*
       Determine if the current thread is running as a user that is a member of the local admins group.
       To do this, create a security descriptor that
       has a DACL which has an ACE that allows only local aministrators access.
       Then, call AccessCheck with the current thread's token and the security descriptor.
       It will say whether the user could access an object if it had that security descriptor.
       Note: you do not need to actually create the object.
       Just checking access against the security descriptor alone will be sufficient.
    */
    //const DWORD ACCESS_READ = 1;//\Windows Kits\10\Include\10.0.22621.0\um\LMaccess.h 已经定义。
    //const DWORD ACCESS_WRITE = 2;

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
        if (!AllocateAndInitializeSid(&SystemSidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdmin))
            __leave;

        psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (psdAdmin == nullptr)
            __leave;

        if (!InitializeSecurityDescriptor(psdAdmin, SECURITY_DESCRIPTOR_REVISION))
            __leave;

        // Compute size needed for the ACL.
        dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psidAdmin) - sizeof(DWORD);
        pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
        if (pACL == nullptr)
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

        if (!AccessCheck(psdAdmin, hImpersonationToken, dwAccessDesired, &GenericMapping, &ps, &dwStructureSize, &dwStatus, &fReturn)) {
            fReturn = FALSE;
            __leave;
        }
    } __finally {
        // Clean up.
        if (pACL)
            LocalFree(pACL);
        if (psdAdmin)
            LocalFree(psdAdmin);
        if (psidAdmin)
            FreeSid(psidAdmin);
        if (hImpersonationToken)
            CloseHandle(hImpersonationToken);
        if (hToken)
            CloseHandle(hToken);
    }

    return fReturn;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
int WINAPI EnumCred()
/*
功能：获取本机的所有的凭据（包括用户名和密码），而且不需要输入计算机用户的密码。
*/
{
    DWORD Count = 0;
    PCREDENTIALW * Credential;
    BOOL ret = CredEnumerateW(nullptr, 0, &Count, &Credential);
    _ASSERTE(ret);

    printf("Count:%u.\n", Count);
    printf("\n");

    for (DWORD x = 0; x < Count; x++) {
        printf("第%u的信息：\n", x + 1);

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
        printf("CredentialBlobSize(字节，换字符请除以二):%u, CredentialBlob(密码):%ls.\n",
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
    LPTSTR ReferencedDomain = nullptr;
    LPTSTR TempReferencedDomain = nullptr;
    LPTSTR TempSid = nullptr;
    DWORD cbSid = 128;              // initial allocation attempt
    DWORD cchReferencedDomain = 16; // initial allocation size
    SID_NAME_USE peUse{};
    BOOL bSuccess = FALSE; // assume this function will fail

    __try {
        *Sid = (PSID)HeapAlloc(GetProcessHeap(), 0, cbSid); // initial memory allocations
        if (*Sid == nullptr)
            __leave;

        ReferencedDomain = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, cchReferencedDomain * sizeof(TCHAR));
        if (ReferencedDomain == nullptr)
            __leave;

        // Obtain the SID of the specified account on the specified system.
        while (!LookupAccountName(
            SystemName,       // machine to lookup account on
            AccountName,      // account to lookup
            *Sid,             // SID of interest
            &cbSid,           // size of SID
            ReferencedDomain, // domain account was found on
            &cchReferencedDomain,
            &peUse)) {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                TempSid = (LPTSTR)HeapReAlloc(GetProcessHeap(), 0, *Sid, cbSid); // reallocate memory
                if (TempSid == nullptr)
                    __leave;
                *Sid = TempSid;

                TempReferencedDomain = (LPTSTR)HeapReAlloc(GetProcessHeap(), 0, ReferencedDomain, cchReferencedDomain * sizeof(TCHAR));

                if (TempReferencedDomain == nullptr)
                    __leave;
                ReferencedDomain = TempReferencedDomain;
            } else
                __leave;
        }

        bSuccess = TRUE; // Indicate success.
    }                    // try
    __finally {          // Cleanup and indicate failure, if appropriate.
        HeapFree(GetProcessHeap(), 0, ReferencedDomain);
        if (!bSuccess) {
            if (*Sid != nullptr) {
                HeapFree(GetProcessHeap(), 0, *Sid);
                *Sid = nullptr;
            }
        }
    } // finally

    return bSuccess;
}


void InitLsaString(PLSA_UNICODE_STRING LsaString, LPWSTR String)
{
    DWORD StringLength{};

    if (String == nullptr) {
        LsaString->Buffer = nullptr;
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
    LSA_OBJECT_ATTRIBUTES ObjectAttributes{};
    LSA_UNICODE_STRING ServerString{};
    PLSA_UNICODE_STRING Server{};

    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes)); // Always initialize the object attributes to all zeroes.
    if (ServerName != nullptr) {                             // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        InitLsaString(&ServerString, ServerName);
        Server = &ServerString;
    } else {
        Server = nullptr;
    }

    return LsaOpenPolicy(Server, &ObjectAttributes, DesiredAccess, PolicyHandle); // Attempt to open the policy.
}


EXTERN_C
__declspec(dllexport)
int WINAPI EnumerateAccountRights(int argc, char * argv[])
/*
功能：枚举用户或者用户组的特权信息，如：administrator或者administrators.

进程继承用户的权限。
程序员大多知道给进程提权。
但是有的进程的某些权限是不易搞到的。
如：Note that your process must have the SE_ASSIGNPRIMARYTOKEN_NAME and
SE_INCREASE_QUOTA_NAME privileges for successful execution of CreateProcessAsUser.
解决办法是用LsaAddAccountRights/LsaRemoveAccountRights给用户添加/删除权限。
然后再用常用的办法提权。
可是这要求注销/重启系统。

记得有个办法是不用重启的，但是忘了方法和链接了。
所以谨记此文。
此文的功能是枚举用户或者用户组的权限的。
此文修改自：Microsoft SDKs\Windows\v6.0\Samples\Security\LSAPolicy\LsaPrivs\LsaPrivs.c。

注意：Unlike privileges, however, account rights are not supported by the LookupPrivilegeValue and LookupPrivilegeName functions.
用户的特权很重要，如：
修改固件环境值。
加载驱动。
启动服务，服务一般不是普通用户运行的。
以操作系统方式执行。
更多信息，请看：
http://msdn.microsoft.com/en-us/library/windows/desktop/bb545671(v=vs.85).aspx  Account Rights Constants
http://msdn.microsoft.com/en-us/library/windows/desktop/bb530716(v=vs.85).aspx  Privilege Constants
http://www.microsoft.com/technet/prodtechnol/WindowsServer2003/Library/IIS/08bc7712-548c-4308-a49c-d551a4b5e245.mspx?mfr=true
等等。

更多的功能请看：
LsaEnumerateAccountsWithUserRight
LsaEnumerateTrustedDomains
LsaEnumerateTrustedDomainsEx
NetEnumerateServiceAccounts （Windows 7/Windows Server 2008 R2）

made by correy
made at 2014.06.14
*/
{
    LSA_HANDLE PolicyHandle{};
    WCHAR wComputerName[256] = L""; // static machine name buffer
    TCHAR AccountName[256]{};       // static account name buffer
    PSID pSid{};
    NTSTATUS Status{};
    int iRetVal = RTN_ERROR; // assume error from main

    if (argc == 1) {
        fprintf(stderr, "Usage: %s <Account> [TargetMachine]\n", argv[0]);
        return RTN_USAGE;
    }

    // Pick up account name on argv[1].
    // Assumes source is ANSI. Resultant string is ANSI or Unicode
    //可以是用户也可以是用户组，如：administrator或者administrators.
    _snwprintf_s(AccountName, 256, 255, TEXT("%hS"), argv[1]);

    // Pick up machine name on argv[2], if appropriate
    // assumes source is ANSI. Resultant string is Unicode.
    if (argc == 3) {
        _snwprintf_s(wComputerName, 256, 255, L"%hS", argv[2]); //这个参数可以无。
    }

    // Open the policy on the target machine.
    Status = OpenPolicy(
        wComputerName, // target machine
        POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
        &PolicyHandle // resultant policy handle
    );
    if (Status != STATUS_SUCCESS) {
        DisplayNtStatus("OpenPolicy", Status);
        return RTN_ERROR;
    }

    // Obtain the SID of the user/group.
    // Note that we could target a specific machine, but we don't.
    // Specifying nullptr for target machine searches for the SID in the following order:
    // well-known, Built-in and local, primary domain,trusted domains.
    if (GetAccountSid(
            nullptr,     // default lookup logic
            AccountName, // account to obtain SID
            &pSid        // buffer to allocate to contain resultant SID
            )) {
        //这几行代码是自己的。
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
        DisplayWinError("GetAccountSid", GetLastError()); // Error obtaining SID.
    }

    LsaClose(PolicyHandle); // Close the policy handle.
    if (pSid != nullptr)
        HeapFree(GetProcessHeap(), 0, pSid); // Free memory allocated for SID.
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

用法示例：
    wchar_t szUser[MAX_PATH] = {0};//估计最大值不是这个。
    wchar_t szDomain[MAX_PATH] = {0};//估计最大值不是这个。这个好像和计算机名一样。
    DWORD d = MAX_PATH;
    bool b = GetCurrentUserAndDomain(szUser, &d, szDomain, &d);

made by correy
made at 2013.05.03

本文摘自：// http://support.microsoft.com/kb/111544/zh-cn
*/
{
    BOOL fSuccess = FALSE;
    HANDLE hToken = nullptr;
    PTOKEN_USER ptiUser = nullptr;
    DWORD cbti = 0;
    SID_NAME_USE snu{};

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
        if (GetTokenInformation(hToken, TokenUser, nullptr, 0, &cbti)) {
            __leave; // Call should have failed due to zero-length buffer.
        } else {     // Call should have failed due to zero-length buffer.
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
        if (!LookupAccountSid(nullptr, ptiUser->User.Sid, szUser, pcchUser, szDomain, pcchDomain, &snu)) {
            __leave;
        }

        fSuccess = TRUE;
    } __finally { // Free resources.
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
    if (wszAccName == nullptr || ppSid == nullptr) { // Validate the input parameters.
        return MQ_ERROR_INVALID_PARAMETER;
    }

    // Create buffers that may be large enough.If a buffer is too small, the count parameter will be set to the size needed.
    const DWORD INITIAL_SIZE = 32;
    DWORD cbSid = 0;
    DWORD dwSidBufferSize = INITIAL_SIZE;
    DWORD cchDomainName = 0;
    DWORD dwDomainBufferSize = INITIAL_SIZE;
    WCHAR * wszDomainName = nullptr;
    SID_NAME_USE eSidType{};
    DWORD dwErrorCode = 0;
    HRESULT hr = MQ_OK;

    *ppSid = (PSID) new BYTE[dwSidBufferSize]; // Create buffers for the SID and the domain name.
    if (*ppSid == nullptr) {
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }
    memset(*ppSid, 0, dwSidBufferSize);
    wszDomainName = new WCHAR[dwDomainBufferSize];
    if (wszDomainName == nullptr) {
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }
    memset(wszDomainName, 0, dwDomainBufferSize * sizeof(WCHAR));

    for (;;) // Obtain the SID for the account name passed.
    {        // Set the count variables to the buffer sizes and retrieve the SID.
        cbSid = dwSidBufferSize;
        cchDomainName = dwDomainBufferSize;
        if (LookupAccountNameW(
                nullptr, // Computer name. nullptr for the local computer
                wszAccName,
                *ppSid,        // Pointer to the SID buffer. Use nullptr to get the size needed,
                &cbSid,        // Size of the SID buffer needed.
                wszDomainName, // wszDomainName,//这个还能获取域名.
                &cchDomainName,
                &eSidType)) //其实这个函数就是返回sid和域名用的别的没啥,不要多想,下面的是垃圾,加上更完美.
        {
            if (IsValidSid(*ppSid) == FALSE) {
                wprintf(L"The SID for %s is invalid.\n", wszAccName);
                dwErrorCode = (DWORD)MQ_ERROR;
            }
            break;
        }
        dwErrorCode = GetLastError();

        if (dwErrorCode == ERROR_INSUFFICIENT_BUFFER) // Check if one of the buffers was too small.
        {
            if (cbSid > dwSidBufferSize) { // Reallocate memory for the SID buffer.
                wprintf(L"The SID buffer was too small. It will be reallocated.\n");
                FreeSid(*ppSid);
                *ppSid = (PSID) new BYTE[cbSid];
                if (*ppSid == nullptr) {
                    return MQ_ERROR_INSUFFICIENT_RESOURCES;
                }
                memset(*ppSid, 0, cbSid);
                dwSidBufferSize = cbSid;
            }
            if (cchDomainName > dwDomainBufferSize) { // Reallocate memory for the domain name buffer.
                wprintf(L"The domain name buffer was too small. It will be reallocated.\n");
                delete[] wszDomainName;
                wszDomainName = new WCHAR[cchDomainName];
                if (wszDomainName == nullptr) {
                    return MQ_ERROR_INSUFFICIENT_RESOURCES;
                }
                memset(wszDomainName, 0, cchDomainName * sizeof(WCHAR));
                dwDomainBufferSize = cchDomainName;
            }
        } else {
            wprintf(L"LookupAccountNameW failed. GetLastError returned: %u\n", dwErrorCode);
            hr = HRESULT_FROM_WIN32(dwErrorCode);
            break;
        }
    }

    delete[] wszDomainName;
    return hr;
}


EXTERN_C
__declspec(dllexport)
int WINAPI GetCurrentSid()
/*
参考:Microsoft SDKs\Windows\v7.1\Samples\security\authorization\textsid这个工程.
获取当前用户(进程的)SID更简单.其实也就这么简单.
made at 2013.10.10
*/
{
    HANDLE hToken{};
    BYTE buf[MY_BUFSIZE]{};
    PTOKEN_USER ptgUser = (PTOKEN_USER)buf;
    DWORD cbBuffer = MY_BUFSIZE;
    BOOL bSuccess{};

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

    LPWSTR lpSid = nullptr;
    ConvertSidToStringSid(ptgUser->User.Sid, &lpSid);

    //这时已经获取到了,可以查看了.

    LocalFree(lpSid);

    return 1;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


BOOL IsRunAsAdmin()
//摘自：UAC self-elevation (CppUACSelfElevation)
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
    PSID pAdministratorsGroup = nullptr;

    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdministratorsGroup)) {
        dwError = GetLastError();
        goto Cleanup;
    }

    // Determine whether the SID of administrators group is enabled in the primary access token of the process.
    if (!CheckTokenMembership(nullptr, pAdministratorsGroup, &fIsRunAsAdmin)) {
        dwError = GetLastError();
        goto Cleanup;
    }

Cleanup:
    // Centralized cleanup for all allocated resources.
    if (pAdministratorsGroup) {
        FreeSid(pAdministratorsGroup);
        pAdministratorsGroup = nullptr;
    }

    // Throw the error if something failed in the function.
    if (ERROR_SUCCESS != dwError) {
        throw dwError;
    }

    return fIsRunAsAdmin;
}


BOOL IsUserInAdminGroup()
//摘自：UAC self-elevation (CppUACSelfElevation)
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
    HANDLE hToken = nullptr;
    HANDLE hTokenToCheck = nullptr;
    DWORD cbSize = 0;
    OSVERSIONINFO osver = {sizeof(osver)};
    BYTE adminSID[SECURITY_MAX_SID_SIZE]{};

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
    cbSize = sizeof(adminSID);
    if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, &adminSID, &cbSize)) {
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
        hToken = nullptr;
    }

    if (hTokenToCheck) {
        CloseHandle(hTokenToCheck);
        hTokenToCheck = nullptr;
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
            HKEY hKEY = nullptr;
            DWORD dwType = REG_DWORD;
            DWORD dwEnableLUA = 0;
            DWORD dwSize = sizeof(DWORD);
            LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       TEXT("SOFTWARE//Microsoft//Windows//CurrentVersion//Policies//System//"),
                                       0,
                                       KEY_READ,
                                       &hKEY);
            if (ERROR_SUCCESS == status) {
                status = RegQueryValueEx(hKEY, TEXT("EnableLUA"), nullptr, &dwType, (BYTE *)&dwEnableLUA, &dwSize);
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


LSA_HANDLE GetPolicyHandle()
/*
打开策略对象句柄

大多数 LSA 策略函数要求使用 策略 对象的句柄来查询或修改系统。
若要获取 策略 对象的句柄，请调用 LsaOpenPolicy 并指定要访问的系统的名称和所需的访问权限集。

应用程序所需的访问权限取决于它执行的操作。
有关每个函数所需权限的详细信息，请参阅 LSA 策略函数中该函数的说明。

如果对 LsaOpenPolicy 的调用成功，则它将为指定系统返回 策略 对象的句柄。
然后，应用程序在后续 LSA 策略函数调用中传递此句柄。 当应用程序不再需要句柄时，它应调用 LsaClose 来释放它。

下面的示例演示如何打开 策略 对象句柄。

在前面的示例中，应用程序请求策略 _ 所有 _ 访问 权限。
有关调用 LsaOpenPolicy时应用程序应该请求的权限的详细信息，请参阅应用程序将 策略 对象句柄传递到的函数的说明。

若要打开受信任域的 策略 对象的句柄，请调用 LsaCreateTrustedDomainEx (以与域) 创建新的信任关系，或调用 LsaOpenTrustedDomainByName (访问现有的受信任域) 。
这两个函数都设置一个指向 LSA _ 句柄的指针，然后您可以在后续 LSA 策略函数调用中指定该句柄。
与 LsaOpenPolicy一样，应用程序在不再需要受信任域的 策略 对象的句柄时，应调用 LsaClose 。

https://docs.microsoft.com/zh-cn/windows/win32/secmgmt/opening-a-policy-object-handle
*/
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes{};
    WCHAR SystemName[] = L"mysystem";
    USHORT SystemNameLength{};
    LSA_UNICODE_STRING lusSystemName{};
    NTSTATUS ntsResult{};
    LSA_HANDLE lsahPolicyHandle{};

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
        wprintf(L"OpenPolicy returned %lu\n", LsaNtStatusToWinError(ntsResult));
        return nullptr;
    }
    return lsahPolicyHandle;
}


void AddPrivileges(PSID AccountSID, LSA_HANDLE PolicyHandle)
/*
管理帐户权限

LSA 提供一些函数，应用程序可调用这些函数来枚举或设置用户、组和本地组帐户的 特权 。

你的应用程序必须获得本地策略对象的句柄，如 打开策略对象句柄中所述，你的应用程序必须获得本地 策略对象的句柄。
此外，若要枚举或编辑帐户的权限，则必须具有该帐户的 安全标识符 (SID) 。
应用程序可以根据 名称和 Sid 间的转换中所述，查找给定了帐户名的 SID。

若要访问具有特定权限的所有帐户，请调用 LsaEnumerateAccountsWithUserRight。
此函数使用具有指定权限的所有帐户的 Sid 填充数组。

获取帐户的 SID 后，可以修改其权限。 调用 LsaAddAccountRights ，将权限添加到帐户。
如果指定的帐户不存在， LsaAddAccountRights 将创建该帐户。
若要从帐户中删除权限，请调用 LsaRemoveAccountRights。
如果从帐户中删除所有权限，则 LsaRemoveAccountRights 也会删除该帐户。

应用程序可以通过调用 LsaEnumerateAccountRights来检查当前分配给帐户的权限。
此函数填充 LSA _ UNICODE _ 字符串 结构的数组。
每个结构都包含指定帐户持有的特权的名称。

下面的示例将 SeServiceLogonRight 权限添加到帐户。
在此示例中，AccountSID 变量指定了帐户的 SID。
有关如何查找帐户 SID 的详细信息，请参阅 名称和 Sid 间的转换。

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
名称和 Sid 间的转换

本地安全机构 (LSA) 提供在用户、组和本地组名称之间进行转换的功能，以及 (SID) 值的相应 安全标识符。
若要查找帐户名称，请调用 LsaLookupNames 函数。 此函数以 RID/域索引对的形式返回 SID。
若要以单个元素的形式获取 SID，请调用 LsaLookupNames2 函数。
若要查找 Sid，请调用 LsaLookupSids。

这些函数可以从本地系统信任的任何域转换名称和 SID 信息。

你的应用程序必须获得本地策略对象的句柄，如 打开策略对象句柄中所述，你的应用程序必须获得本地 策略对象的句柄。

下面的示例在给定帐户名称的情况下查找帐户的 SID。

在前面的示例中，函数 InitLsaString 将 unicode 字符串转换为 LSA _ unicode _ 字符串 结构。
此函数的代码在 使用 LSA Unicode 字符串中显示。

 备注

这些转换函数主要由权限编辑器用来显示 访问控制列表 (ACL) 信息。
权限编辑器应始终使用 name 或 security identifier SID 所在系统的 Policy对象调用这些函数。
这可确保在转换过程中引用正确的受信任域集。

Windows Access Control 还提供了在 Sid 和帐户名之间执行转换的函数： LookupAccountName 和 LookupAccountSid。
如果你的应用程序需要查找帐户名称或 SID，但不使用其他 LSA 策略功能，请使用 Windows Access Control 功能，而不是 LSA 策略函数。
有关这些函数的详细信息，请参阅 访问控制。

https://docs.microsoft.com/zh-cn/windows/win32/secmgmt/translating-between-names-and-sids
*/
{
    LSA_UNICODE_STRING lucName{};
    PLSA_TRANSLATED_SID ltsTranslatedSID{};
    PLSA_REFERENCED_DOMAIN_LIST lrdlDomainList{};
    LSA_TRUST_INFORMATION myDomain{};
    NTSTATUS ntsResult{};
    PWCHAR DomainString = nullptr;

    InitLsaString(&lucName, AccountName); // Initialize an LSA_UNICODE_STRING with the name.

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
    if (DomainString) {
        wcsncpy_s(DomainString,
                  myDomain.Name.Length,
                  myDomain.Name.Buffer,
                  myDomain.Name.Length);

        // Display the relative Id.
        wprintf(L"Relative Id is %lu in domain %ws.\n", ltsTranslatedSID->RelativeId, DomainString);

        LocalFree(DomainString);
    } else {
    }

    LsaFreeMemory(ltsTranslatedSID);
    LsaFreeMemory(lrdlDomainList);
}


//////////////////////////////////////////////////////////////////////////////////////////////////


NTSTATUS
OpenPolicy(
    LPWSTR ServerName,       // machine to open policy on (Unicode)
    DWORD DesiredAccess,     // desired access to policy
    PLSA_HANDLE PolicyHandle // resultant policy handle
);

BOOL GetAccountSid(
    LPTSTR SystemName,  // where to lookup account
    LPTSTR AccountName, // account of interest
    PSID * Sid          // resultant buffer containing SID
);

NTSTATUS
SetPrivilegeOnAccount(
    LSA_HANDLE PolicyHandle, // open policy handle
    PSID AccountSid,         // SID to grant privilege to
    LPWSTR PrivilegeName,    // privilege to grant (Unicode)
    BOOL bEnable             // enable or disable
);

void InitLsaString(
    PLSA_UNICODE_STRING LsaString, // destination
    LPWSTR String                  // source (Unicode)
);


EXTERN_C
__declspec(dllexport) int WINAPI ManageUserPrivileges(int argc, char * argv[])
/*
\Windows-classic-samples\Samples\Win7Samples\security\lsapolicy\lsaprivs\LsaPrivs.c
*/
/*
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

Copyright (C) 1998 - 2000.  Microsoft Corporation.  All rights reserved.

LsaSamp.c

This sample demonstrates the use of the Lsa APIs to manage User Privileges.
This sample will work properly compiled ANSI or Unicode.
History:    12-Jul-95 Scott Field (sfield)    Created
            08-Jan-99 David Mowers (davemo)   Updated
*/
{
    LSA_HANDLE PolicyHandle{};
    WCHAR wComputerName[256] = L""; // static machine name buffer
    TCHAR AccountName[256]{};       // static account name buffer
    PSID pSid{};
    NTSTATUS Status{};
    int iRetVal = RTN_ERROR; // assume error from main

    if (argc == 1) {
        fprintf(stderr, "Usage: %s <Account> [TargetMachine]\n", argv[0]);
        return RTN_USAGE;
    }

    // Pick up account name on argv[1].
    // Assumes source is ANSI. Resultant string is ANSI or Unicode
    _snwprintf_s(AccountName, 256, 255, TEXT("%hS"), argv[1]);

    // Pick up machine name on argv[2], if appropriate
    // assumes source is ANSI. Resultant string is Unicode.
    if (argc == 3)
        _snwprintf_s(wComputerName, 256, 255, L"%hS", argv[2]);

    // Open the policy on the target machine.
    Status = OpenPolicy(
        wComputerName, // target machine
        POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
        &PolicyHandle // resultant policy handle
    );

    if (Status != STATUS_SUCCESS) {
        DisplayNtStatus("OpenPolicy", Status);
        return RTN_ERROR;
    }

    // Obtain the SID of the user/group.
    // Note that we could target a specific machine, but we don't.
    // Specifying nullptr for target machine searches for the SID in the
    // following order: well-known, Built-in and local, primary domain, trusted domains.
    if (GetAccountSid(
            nullptr,     // default lookup logic
            AccountName, // account to obtain SID
            &pSid        // buffer to allocate to contain resultant SID
            )) {
        // We only grant the privilege if we succeeded in obtaining the
        // SID. We can actually add SIDs which cannot be looked up, but
        // looking up the SID is a good sanity check which is suitable for most cases.

        // Grant the SeServiceLogonRight to users represented by pSid.
        Status = SetPrivilegeOnAccount(
            PolicyHandle,                   // policy handle
            pSid,                           // SID to grant privilege
            (LPWSTR)L"SeServiceLogonRight", // Unicode privilege
            TRUE                            // enable the privilege
        );
        if (Status == STATUS_SUCCESS)
            iRetVal = RTN_OK;
        else
            DisplayNtStatus("SetPrivilegeOnAccount", Status);
    } else {
        DisplayWinError("GetAccountSid", GetLastError()); // Error obtaining SID.
    }

    LsaClose(PolicyHandle); // Close the policy handle.

    // Free memory allocated for SID.
    if (pSid != nullptr)
        HeapFree(GetProcessHeap(), 0, pSid);

    return iRetVal;
}


//BOOL GetAccountSid(LPTSTR SystemName, LPTSTR AccountName, PSID * Sid)
///*++
//This function attempts to obtain a SID representing the supplied
//account on the supplied system.
//
//If the function succeeds, the return value is TRUE. A buffer is
//allocated which contains the SID representing the supplied account.
//This buffer should be freed when it is no longer needed by calling
//HeapFree(GetProcessHeap(), 0, buffer)
//
//If the function fails, the return value is FALSE. Call GetLastError()
//to obtain extended error information.
//
//--*/
//{
//    LPTSTR ReferencedDomain = nullptr;
//    LPTSTR TempReferencedDomain = nullptr;
//    LPTSTR TempSid = nullptr;
//    DWORD cbSid = 128;    // initial allocation attempt
//    DWORD cchReferencedDomain = 16; // initial allocation size
//    SID_NAME_USE peUse;
//    BOOL bSuccess = FALSE; // assume this function will fail
//
//    __try {
//
//        //
//        // initial memory allocations
//        //
//        *Sid = (PSID)HeapAlloc(GetProcessHeap(), 0, cbSid);
//
//        if (*Sid == nullptr) __leave;
//
//        ReferencedDomain = (LPTSTR)HeapAlloc(
//            GetProcessHeap(),
//            0,
//            cchReferencedDomain * sizeof(TCHAR)
//        );
//
//        if (ReferencedDomain == nullptr) __leave;
//
//        //
//        // Obtain the SID of the specified account on the specified system.
//        //
//        while (!LookupAccountName(
//            SystemName,         // machine to lookup account on
//            AccountName,        // account to lookup
//            *Sid,               // SID of interest
//            &cbSid,             // size of SID
//            ReferencedDomain,   // domain account was found on
//            &cchReferencedDomain,
//            &peUse
//        )) {
//            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
//                //
//                // reallocate memory
//                //
//                TempSid = (PSID)HeapReAlloc(
//                    GetProcessHeap(),
//                    0,
//                    *Sid,
//                    cbSid
//                );
//
//                if (TempSid == nullptr) __leave;
//                *Sid = TempSid;
//
//                TempReferencedDomain = (LPTSTR)HeapReAlloc(
//                    GetProcessHeap(),
//                    0,
//                    ReferencedDomain,
//                    cchReferencedDomain * sizeof(TCHAR)
//                );
//
//                if (TempReferencedDomain == nullptr) __leave;
//                ReferencedDomain = TempReferencedDomain;
//
//            } else __leave;
//        }
//
//        //
//        // Indicate success.
//        //
//        bSuccess = TRUE;
//
//    } // try
//    __finally {
//
//        //
//        // Cleanup and indicate failure, if appropriate.
//        //
//
//        HeapFree(GetProcessHeap(), 0, ReferencedDomain);
//
//        if (!bSuccess) {
//            if (*Sid != nullptr) {
//                HeapFree(GetProcessHeap(), 0, *Sid);
//                *Sid = nullptr;
//            }
//        }
//
//    } // finally
//
//    return bSuccess;
//}


NTSTATUS
SetPrivilegeOnAccount(
    LSA_HANDLE PolicyHandle, // open policy handle
    PSID AccountSid,         // SID to grant privilege to
    LPWSTR PrivilegeName,    // privilege to grant (Unicode)
    BOOL bEnable             // enable or disable
)
{
    LSA_UNICODE_STRING PrivilegeString;

    // Create a LSA_UNICODE_STRING for the privilege name.
    InitLsaString(&PrivilegeString, PrivilegeName);

    // grant or revoke the privilege, accordingly
    if (bEnable) {
        return LsaAddAccountRights(
            PolicyHandle,     // open policy handle
            AccountSid,       // target SID
            &PrivilegeString, // privileges
            1                 // privilege count
        );
    } else {
        return LsaRemoveAccountRights(
            PolicyHandle,     // open policy handle
            AccountSid,       // target SID
            FALSE,            // do not disable all rights
            &PrivilegeString, // privileges
            1                 // privilege count
        );
    }
}


//void InitLsaString(PLSA_UNICODE_STRING LsaString,LPWSTR String)
//{
//    DWORD StringLength;
//
//    if (String == nullptr) {
//        LsaString->Buffer = nullptr;
//        LsaString->Length = 0;
//        LsaString->MaximumLength = 0;
//        return;
//    }
//
//    StringLength = lstrlenW(String);
//    LsaString->Buffer = String;
//    LsaString->Length = (USHORT)StringLength * sizeof(WCHAR);
//    LsaString->MaximumLength = (USHORT)(StringLength + 1) * sizeof(WCHAR);
//}


//NTSTATUS OpenPolicy(LPWSTR ServerName, DWORD DesiredAccess, PLSA_HANDLE PolicyHandle)
//{
//    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
//    LSA_UNICODE_STRING ServerString;
//    PLSA_UNICODE_STRING Server;
//
//    //
//    // Always initialize the object attributes to all zeroes.
//    //
//    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
//
//    if (ServerName != nullptr) {
//        //
//        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
//        //
//        InitLsaString(&ServerString, ServerName);
//        Server = &ServerString;
//    } else {
//        Server = nullptr;
//    }
//
//    //
//    // Attempt to open the policy.
//    //
//    return LsaOpenPolicy(
//        Server,
//        &ObjectAttributes,
//        DesiredAccess,
//        PolicyHandle
//    );
//}


//////////////////////////////////////////////////////////////////////////////////////////////////


void DisplayErrorText(DWORD dwLastError)
/*
查找错误代码号的文本
项目
2023/06/13
4 个参与者
有时需要显示与网络相关函数返回的错误代码关联的错误文本。 可能需要使用系统提供的网络管理功能执行此任务。

这些消息的错误文本位于名为 Netmsg.dll 的消息表文件中，该文件位于 %systemroot%\system32 中。
此文件包含NERR_BASE (2100) 到 MAX_NERR (NERR_BASE+899) 范围内的错误消息。
这些错误代码在 SDK 头文件 lmerr.h 中定义。

LoadLibrary 和 LoadLibraryEx 函数可以加载Netmsg.dll。
FormatMessage 函数将错误代码映射到消息文本，给定Netmsg.dll文件的模块句柄。

下面的示例演示了如何显示与网络管理功能关联的错误文本，以及显示与系统相关的错误代码关联的错误文本。
如果提供的错误号在特定范围内，则会加载netmsg.dll消息模块，并使用 FormatMessage 函数查找指定的错误号。

int __cdecl main(int argc, char * argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <error number>\n", argv[0]);
        return RTN_USAGE;
    }

    DisplayErrorText(atoi(argv[1]));

    return RTN_OK;
}

https://learn.microsoft.com/zh-cn/windows/win32/netmgmt/looking-up-text-for-error-code-numbers
*/
{
    HMODULE hModule = NULL; // default to system source
    LPSTR MessageBuffer;
    DWORD dwBufferLength;
    DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_IGNORE_INSERTS |
                          FORMAT_MESSAGE_FROM_SYSTEM;

    // If dwLastError is in the network range, load the message source.
    if (dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) {
        hModule = LoadLibraryEx(TEXT("netmsg.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    // Call FormatMessage() to allow for message
    //  text to be acquired from the system or from the supplied module handle.
    dwBufferLength = FormatMessageA(
        dwFormatFlags,
        hModule, // module to get message from (NULL == system)
        dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
        (LPSTR)&MessageBuffer,
        0,
        NULL);
    if (dwBufferLength) {
        DWORD dwBytesWritten;

        // Output message string on stderr.
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), MessageBuffer, dwBufferLength, &dwBytesWritten, NULL);

        LocalFree(MessageBuffer); // Free the buffer allocated by the system.
    }

    // If we loaded a message source, unload it.
    if (hModule != NULL)
        FreeLibrary(hModule);
}


EXTERN_C
__declspec(dllexport)
int WINAPI ChangePassword(int argc, wchar_t * argv[])
/*
在没有密码或者已知密码的情况下修改密码.
用途是可以用脚本或者编程以快速的方式改变密码.

https://learn.microsoft.com/zh-cn/windows/win32/api/lmaccess/nf-lmaccess-netuserchangepassword
*/
{
    if (argc != 4) {
        fwprintf(stderr, L"Usage: %s \\\\UserName OldPassword NewPassword\n", argv[0]);
        exit(1);
    }

    NET_API_STATUS nStatus = NetUserChangePassword(0, argv[1], argv[2], argv[3]); //在没有密码的情况下第三个参数可以为空即:L"".
    if (nStatus == NERR_Success) {
        fwprintf(stderr, L"User password has been changed successfully\n");
    } else {
        fprintf(stderr, "A system error has occurred: %d\n", nStatus);
    }

    return 0;
}


EXTERN_C
__declspec(dllexport)
void WINAPI SetPassword()
/*
设置用户密码，级别 1003
下面的代码片段演示如何通过调用 NetUserSetInfo 函数将用户的密码设置为已知值。
USER_INFO_1003主题包含其他信息。

心得，这个API比NetUserChangePassword好，也比IADsUser接口的SetPassword好。

注意事项：
1.权限。
2.确保用户的设置是可以修改密码的。
3.

https://learn.microsoft.com/zh-cn/windows/win32/netmgmt/changing-elements-of-user-information
https://learn.microsoft.com/zh-cn/windows/win32/api/lmaccess/ns-lmaccess-user_info_1003
https://learn.microsoft.com/zh-cn/windows/win32/api/lmaccess/nf-lmaccess-netusersetinfo
https://learn.microsoft.com/zh-cn/windows/win32/api/iads/nf-iads-iadsuser-setpassword
*/
{
#define PASSWORD L"new_password" //长度不能超过 PWLEN 字节。按照惯例，密码长度限制为LM20_PWLEN个字符。

    USER_INFO_1003 usriSetPassword;
    // Set the usri1003_password member to point to a valid Unicode string.
    // SERVER and USERNAME can be hard-coded strings or pointers to Unicode strings.
    usriSetPassword.usri1003_password = (LPWSTR)PASSWORD;

    WCHAR UserName[UNLEN + 1]{};
    DWORD Size{_ARRAYSIZE(UserName)};
    GetUserName(UserName, &Size);

    NET_API_STATUS netRet = NetUserSetInfo(NULL, UserName, 1003, (LPBYTE)&usriSetPassword, NULL);
    if (netRet == NERR_Success)
        printf("Success with level 1003!\n");
    else
        printf("ERROR: %d returned from NetUserSetInfo level 1003\n", netRet);
}


void set_password_expired(void)
/*
强制用户更改登录密码
项目
2023/06/13
3 个参与者
此代码示例演示如何使用 NetUserGetInfo 和 NetUserSetInfo 函数以及 USER_INFO_3 结构强制用户在下次登录时更改 登录 密码。
请注意，从 Windows XP 开始，建议改用 USER_INFO_4 结构。

使用以下代码片段将 USER_INFO_3 结构的 usri3_password_expired 成员设置为非零值：

https://learn.microsoft.com/zh-cn/windows/win32/netmgmt/forcing-a-user-to-change-the-logon-password
*/
{
#define USERNAME L"your_user_name"
#define SERVER L"\\\\server"
    PUSER_INFO_3 pUsr = NULL;
    NET_API_STATUS netRet = 0;
    DWORD dwParmError = 0;

    // First, retrieve the user information at level 3. This is
    // necessary to prevent resetting other user information when the NetUserSetInfo call is made.
    netRet = NetUserGetInfo(SERVER, USERNAME, 3, (LPBYTE *)&pUsr);
    if (netRet == NERR_Success) {
        // The function was successful; set the usri3_password_expired value to
        // a nonzero value. Call the NetUserSetInfo function.
        pUsr->usri3_password_expired = TRUE;
        netRet = NetUserSetInfo(SERVER, USERNAME, 3, (LPBYTE)pUsr, &dwParmError);

        // A zero return indicates success.
        // If the return value is ERROR_INVALID_PARAMETER,
        //  the dwParmError parameter will contain a value indicating the
        //  invalid parameter within the user_info_3 structure.
        //  These values are defined in the lmaccess.h file.
        if (netRet == NERR_Success)
            printf("User %S will need to change password at next logon", USERNAME);
        else
            printf("Error %d occurred. Parm Error %d returned.\n", netRet, dwParmError);

        NetApiBufferFree(pUsr); // Must free the buffer returned by NetUserGetInfo.
    } else
        printf("NetUserGetInfo failed: %d\n", netRet);
}


BOOL AddMachineAccount(LPWSTR wTargetComputer, LPWSTR MachineAccount, DWORD AccountType)
/*
创建新计算机帐户
项目
2023/06/13
4 个参与者

下面的代码示例演示如何使用 NetUserAdd 函数创建新的计算机帐户。

以下是管理计算机帐户的注意事项：

1.为了与帐户管理实用工具保持一致，计算机帐户名称应全部为大写。
2.计算机帐户名称始终具有尾随美元符号 ($) 。
  用于管理计算机帐户的任何函数都必须生成计算机名称，以便计算机帐户名称的最后一个字符是美元符号 ($) 。
  对于域间信任，帐户名称为 TrustingDomainName$。
3.最大计算机名称长度为 MAX_COMPUTERNAME_LENGTH (15) 。 此长度不包括尾随美元符号 ($) 。
4.新计算机帐户的密码应为计算机帐户名称的小写表示形式，不带尾随美元符号 ($) 。
  对于域间信任，密码可以是与关系的信任端指定的值匹配的任意值。
5.最大密码长度为 LM20_PWLEN (14) 。 如果计算机帐户名超过此长度，应将密码截断为此长度。
6.创建计算机帐户时提供的密码仅在计算机帐户在域中变为活动状态之前有效。
  信任关系激活期间会建立新密码。

调用帐户管理功能的用户必须在目标计算机上具有管理员权限。
对于现有计算机帐户，帐户的创建者可以管理帐户，而不考虑管理成员身份。
有关调用需要管理员权限的函数的详细信息，请参阅 使用特殊特权运行。

可以在目标计算机上授予 SeMachineAccountPrivilege，以便指定用户能够创建计算机帐户。
这使非管理员能够创建计算机帐户。 调用方需要在添加计算机帐户之前启用此权限。
有关帐户特权的详细信息，请参阅 特权 和 授权常量。

https://learn.microsoft.com/zh-cn/windows/win32/netmgmt/creating-a-new-computer-account
*/
/*
;此文是修改网上的汇编代码。修改成我喜欢的方式。
;羞于贴出来，但为了知识，还是发出来。
;此文虽小，但发现一个知识宝库，等待去挖掘。
.386
.model flat, stdcall
option casemap :none

include windows.inc
include Netapi32.inc
includelib Netapi32.lib

.code

ui1 USER_INFO_1 <offset szUser,offset szPass,0,USER_PRIV_USER,0,0,UF_NORMAL_ACCOUNT,0>
lmi3 LOCALGROUP_MEMBERS_INFO_3 <offset szUser>
dwErr DWORD 0
szUser dw "c","o","r","r","e","y",0
szPass dw "c","o","r","r","e","y",0
szAdministrators dw "A","d","m","i","n","i","s","t","r","a","t","o","r","s",0

start:invoke NetUserAdd,NULL, 1,addr ui1,addr dwErr
invoke NetLocalGroupAddMembers,NULL,addr szAdministrators,3,addr lmi3,1
ret ;invoke ExitProcess,0
end start
;made at 2011,10.16
*/
{
    LPWSTR wAccount;
    LPWSTR wPassword;
    USER_INFO_1 ui;
    DWORD cbAccount;
    DWORD cbLength;
    DWORD dwError;

    // Ensure a valid computer account type was passed.
    if (AccountType != UF_WORKSTATION_TRUST_ACCOUNT &&
        AccountType != UF_SERVER_TRUST_ACCOUNT &&
        AccountType != UF_INTERDOMAIN_TRUST_ACCOUNT) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    cbLength = cbAccount = lstrlenW(MachineAccount); // Obtain number of chars in computer account name.

    // Ensure computer name doesn't exceed maximum length.
    if (cbLength > MAX_COMPUTERNAME_LENGTH) {
        SetLastError(ERROR_INVALID_ACCOUNT_NAME);
        return FALSE;
    }

    // Allocate storage to contain Unicode representation of computer account name + trailing $ + NULL.
    wAccount = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (cbAccount + 1 + 1) * sizeof(WCHAR) // Account + '$' + NULL
    );

    if (wAccount == NULL)
        return FALSE;

    // Password is the computer account name converted to lowercase;
    //  you will convert the passed MachineAccount in place.
    wPassword = MachineAccount;

    // Copy MachineAccount to the wAccount buffer allocated while
    //  converting computer account name to uppercase.
    //  Convert password (in place) to lowercase.
    while (cbAccount--) {
        wAccount[cbAccount] = towupper(MachineAccount[cbAccount]);
        wPassword[cbAccount] = towlower(wPassword[cbAccount]);
    }

    // Computer account names have a trailing Unicode '$'.
#pragma warning(push)
#pragma warning(disable : 6386) //写入 "wAccount" 时缓冲区溢出。
    wAccount[cbLength] = L'$';
    wAccount[cbLength + 1] = L'\0'; // terminate the string
#pragma warning(pop)

    // If the password is greater than the max allowed, truncate.
    if (cbLength > LM20_PWLEN)
        wPassword[LM20_PWLEN] = L'\0';

    // Initialize the USER_INFO_1 structure.
    ZeroMemory(&ui, sizeof(ui));

    ui.usri1_name = wAccount;
    ui.usri1_password = wPassword;

    ui.usri1_flags = AccountType | UF_SCRIPT;
    ui.usri1_priv = USER_PRIV_USER;

    dwError = NetUserAdd(
        wTargetComputer, // target computer name
        1,               // info level
        (LPBYTE)&ui,     // buffer
        NULL);

    // Free allocated memory.
    if (wAccount)
        HeapFree(GetProcessHeap(), 0, wAccount);

    // Indicate whether the function was successful.
    if (dwError == NO_ERROR)
        return TRUE;
    else {
        SetLastError(dwError);
        return FALSE;
    }
}


BOOL GetFullName(char * UserName, char * Domain, char * dest)
/*
查找用户的全名
项目
2023/06/13
2 个参与者
可以将计算机组织到域中，该 域是计算机网络的集合。 域管理员维护集中式用户和组帐户信息。

若要查找用户的全名，请指定用户名和域名：

将用户名和域名转换为 Unicode（如果它们不是 Unicode 字符串）。
通过调用 NetGetDCName (DC) 查找域控制器的计算机名称。
通过调用 NetUserGetInfo 在 DC 计算机上查找用户名。
将完整用户名转换为 ANSI，除非程序预期使用 Unicode 字符串。
以下示例代码是 getFullName) (函数，它采用前两个参数中的用户名和域名，并在第三个参数中返回用户的全名。

https://learn.microsoft.com/zh-cn/windows/win32/netmgmt/looking-up-a-users-full-name
*/
{
    WCHAR wszUserName[UNLEN + 1]; // Unicode user name
    WCHAR wszDomain[256];
    LPBYTE ComputerName;
    //    struct _SERVER_INFO_100 *si100;  // Server structure
    struct _USER_INFO_2 * ui; // User structure

    // Convert ANSI user name and domain to Unicode
    MultiByteToWideChar(CP_ACP, 0, UserName, (int)strlen(UserName) + 1, wszUserName, sizeof(wszUserName) / sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, Domain, (int)strlen(Domain) + 1, wszDomain, sizeof(wszDomain) / sizeof(WCHAR));

    NetGetDCName(NULL, wszDomain, &ComputerName); // Get the computer name of a DC for the domain.

    // Look up the user on the DC.
    if (NetUserGetInfo((LPWSTR)ComputerName, (LPWSTR)&wszUserName, 2, (LPBYTE *)&ui)) {
        wprintf(L"Error getting user information.\n");
        return (FALSE);
    }

    // Convert the Unicode full name to ANSI.
    WideCharToMultiByte(CP_ACP, 0, ui->usri2_full_name, -1, dest, 256, NULL, NULL);

    return (TRUE);
}


NET_API_STATUS NetSample(LPWSTR lpszDomain,
                         LPWSTR lpszUser,
                         LPWSTR lpszPassword,
                         LPWSTR lpszLocalGroup)
/*
创建本地组和添加用户
项目
2023/06/13
2 个参与者
若要创建新的本地组，请调用 NetLocalGroupAdd 函数。
若要将用户添加到该组，请调用 NetLocalGroupAddMembers 函数。

以下程序允许您创建用户和本地组，并将用户添加到本地组。

int main()
{
    NET_API_STATUS err = 0;

    printf("Calling NetSample.\n");
    err = NetSample(L"SampleDomain", L"SampleUser", L"SamplePswd", L"SampleLG");
    printf("NetSample returned %d\n", err);
    return(0);
}

https://learn.microsoft.com/zh-cn/windows/win32/netmgmt/creating-a-local-group-and-adding-a-user
*/
{
    USER_INFO_1 user_info;
    LOCALGROUP_INFO_1 localgroup_info;
    LOCALGROUP_MEMBERS_INFO_3 localgroup_members;
    LPWSTR lpszPrimaryDC = NULL;
    NET_API_STATUS err = 0;
    DWORD parm_err = 0;

    // First get the name of the primary domain controller.
    // Be sure to free the returned buffer.
    err = NetGetDCName(NULL,                      // local computer
                       lpszDomain,                // domain name
                       (LPBYTE *)&lpszPrimaryDC); // returned PDC
    if (err != 0) {
        printf("Error getting DC name: %d\n", err);
        return (err);
    }

    // Set up the USER_INFO_1 structure.
    user_info.usri1_name = lpszUser;
    user_info.usri1_password = lpszPassword;
    user_info.usri1_priv = USER_PRIV_USER;
    user_info.usri1_home_dir = (LPWSTR)TEXT("");
    user_info.usri1_comment = (LPWSTR)TEXT("Sample User");
    user_info.usri1_flags = UF_SCRIPT;
    user_info.usri1_script_path = (LPWSTR)TEXT("");
    err = NetUserAdd(lpszPrimaryDC,      // PDC name
                     1,                  // level
                     (LPBYTE)&user_info, // input buffer
                     &parm_err);         // parameter in error
    switch (err) {
    case 0:
        printf("User successfully created.\n");
        break;
    case NERR_UserExists:
        printf("User already exists.\n");
        err = 0;
        break;
    case ERROR_INVALID_PARAMETER:
        printf("Invalid parameter error adding user; parameter index = %d\n", parm_err);
        NetApiBufferFree(lpszPrimaryDC);
        return (err);
    default:
        printf("Error adding user: %d\n", err);
        NetApiBufferFree(lpszPrimaryDC);
        return (err);
    }

    // Set up the LOCALGROUP_INFO_1 structure.
    localgroup_info.lgrpi1_name = lpszLocalGroup;
    localgroup_info.lgrpi1_comment = (LPWSTR)TEXT("Sample local group.");
    err = NetLocalGroupAdd(lpszPrimaryDC,            // PDC name
                           1,                        // level
                           (LPBYTE)&localgroup_info, // input buffer
                           &parm_err);               // parameter in error
    switch (err) {
    case 0:
        printf("Local group successfully created.\n");
        break;
    case ERROR_ALIAS_EXISTS:
        printf("Local group already exists.\n");
        err = 0;
        break;
    case ERROR_INVALID_PARAMETER:
        printf("Invalid parameter error adding local group; parameter index = %d\n", err); //, parm_err
        NetApiBufferFree(lpszPrimaryDC);
        return (err);
    default:
        printf("Error adding local group: %d\n", err);
        NetApiBufferFree(lpszPrimaryDC);
        return (err);
    }

    // Now add the user to the local group.
    localgroup_members.lgrmi3_domainandname = lpszUser;
    err = NetLocalGroupAddMembers(lpszPrimaryDC,               // PDC name
                                  lpszLocalGroup,              // group name
                                  3,                           // name
                                  (LPBYTE)&localgroup_members, // buffer
                                  1);                          // count
    switch (err) {
    case 0:
        printf("User successfully added to local group.\n");
        break;
    case ERROR_MEMBER_IN_ALIAS:
        printf("User already in local group.\n");
        err = 0;
        break;
    default:
        printf("Error adding user to local group: %d\n", err);
        break;
    }

    NetApiBufferFree(lpszPrimaryDC);
    return (err);
}


EXTERN_C
__declspec(dllexport)
int WINAPI EnumUser(int argc, wchar_t * argv[])
/*
下面的代码示例演示如何通过调用 NetUserEnum 函数检索有关服务器上的用户帐户的信息。
此示例调用 NetUserEnum，指定信息级别 0 (USER_INFO_0) 仅枚举全局用户帐户。
如果调用成功，代码将循环访问条目并输出每个用户帐户的名称。
最后，代码示例释放为信息缓冲区分配的内存，并打印枚举的用户总数。

注释：效果类似于net user命令。
注意：会包含已经禁用的账户。

https://learn.microsoft.com/zh-cn/windows/win32/api/lmaccess/nf-lmaccess-netuserenum
*/
{
    LPUSER_INFO_0 pBuf = NULL;
    LPUSER_INFO_0 pTmpBuf;
    DWORD dwLevel = 0;
    DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;
    DWORD i;
    DWORD dwTotalCount = 0;
    NET_API_STATUS nStatus;
    LPTSTR pszServerName = NULL;

    if (argc > 2) {
        fwprintf(stderr, L"Usage: %s [\\\\ServerName]\n", argv[0]);
        exit(1);
    }
    // The server is not the default local computer.
    if (argc == 2)
        pszServerName = (LPTSTR)argv[1];
    wprintf(L"\nUser account on %s: \n", pszServerName);

    // Call the NetUserEnum function, specifying level 0;
    //   enumerate global user account types only.
    do // begin do
    {
        nStatus = NetUserEnum((LPCWSTR)pszServerName,
                              dwLevel,
                              FILTER_NORMAL_ACCOUNT, // global users
                              (LPBYTE *)&pBuf,
                              dwPrefMaxLen,
                              &dwEntriesRead,
                              &dwTotalEntries,
                              &dwResumeHandle);
        if ((nStatus == NERR_Success) || (nStatus == ERROR_MORE_DATA)) { // If the call succeeds,
            if ((pTmpBuf = pBuf) != NULL) {
                for (i = 0; (i < dwEntriesRead); i++) { // Loop through the entries.
                    assert(pTmpBuf != NULL);
                    if (pTmpBuf == NULL) {
                        fprintf(stderr, "An access violation has occurred\n");
                        break;
                    }

                    wprintf(L"\t-- %s\n", pTmpBuf->usri0_name); //  Print the name of the user account.

                    pTmpBuf++;
                    dwTotalCount++;
                }
            }
        } else // Otherwise, print the system error.
            fprintf(stderr, "A system error has occurred: %d\n", nStatus);

        // Free the allocated buffer.
        if (pBuf != NULL) {
            NetApiBufferFree(pBuf);
            pBuf = NULL;
        }
    }
    // Continue to call NetUserEnum while there are more entries.
    while (nStatus == ERROR_MORE_DATA); // end do

    // Check again for allocated memory.
    if (pBuf != NULL)
        NetApiBufferFree(pBuf);

    // Print the final count of users enumerated.
    fprintf(stderr, "\nTotal of %d entries enumerated\n", dwTotalCount);

    return 0;
}


EXTERN_C
__declspec(dllexport)
void WINAPI EnumGroup(int argc, char * argv[])
/*
下面的代码示例演示如何使用调用 NetQueryDisplayInformation 函数返回组帐户信息。
如果用户指定服务器名称，则示例首先调用 MultiByteToWideChar 函数，以将该名称转换为 Unicode。
此示例调用 NetQueryDisplayInformation，指定信息级别 3 (NET_DISPLAY_GROUP) 来检索组帐户信息。
如果有要返回的条目，示例将返回数据并打印组信息。
最后，代码示例释放为信息缓冲区分配的内存。

NetQueryDisplayInformation 函数提供了一种用于枚举全局组的有效机制。
如果可能，建议使用 NetQueryDisplayInformation 而不是 NetGroupEnum 函数。

此函数相当于net GROUP命令。

https://learn.microsoft.com/zh-cn/windows/win32/api/lmaccess/nf-lmaccess-netquerydisplayinformation
https://learn.microsoft.com/zh-cn/windows/win32/api/lmaccess/nf-lmaccess-netgroupenum
*/
{
    PNET_DISPLAY_GROUP pBuff, p;
    DWORD res, dwRec, i = 0;

    // You can pass a NULL or empty string to retrieve the local information.
    TCHAR szServer[255] = TEXT("");

    if (argc > 1)
        // Check to see if a server name was passed;
        //  if so, convert it to Unicode.
        MultiByteToWideChar(CP_ACP, 0, argv[1], -1, szServer, 255);

    do // begin do
    {
        // Call the NetQueryDisplayInformation function;
        //   specify information level 3 (group account information).
        res = NetQueryDisplayInformation(szServer, 3, i, 1000, MAX_PREFERRED_LENGTH, &dwRec, (PVOID *)&pBuff);
        if ((res == ERROR_SUCCESS) || (res == ERROR_MORE_DATA)) { // If the call succeeds,
            p = pBuff;
            for (; dwRec > 0; dwRec--) {
                // Print the retrieved group information.
                printf("Name:      %S\n"
                       "Comment:   %S\n"
                       "Group ID:  %u\n"
                       "Attributes: %u\n"
                       "--------------------------------\n",
                       p->grpi3_name,
                       p->grpi3_comment,
                       p->grpi3_group_id,
                       p->grpi3_attributes);

                // If there is more data, set the index.
                i = p->grpi3_next_index;
                p++;
            }

            NetApiBufferFree(pBuff); // Free the allocated memory.
        } else
            printf("Error: %u\n", res);

        // Continue while there is more data.
    } while (res == ERROR_MORE_DATA); // end do
}


EXTERN_C
__declspec(dllexport) 
int WINAPI EnumLocalGroup()
/*
此函数的功能类似于net LOCALGROUP命令。

https://learn.microsoft.com/zh-cn/windows/win32/api/lmaccess/nf-lmaccess-netlocalgroupenum
*/
{
    LPBYTE bufptr{};
    DWORD prefmaxlen = MAX_PREFERRED_LENGTH;
    DWORD entriesread{};
    DWORD totalentries{};
    ULONG_PTR resumehandle{};
    NET_API_STATUS Status = NetLocalGroupEnum(NULL, 1, &bufptr, prefmaxlen, &entriesread, &totalentries, &resumehandle);
    if (NERR_Success == Status) {
        LPLOCALGROUP_INFO_1 Info = (LPLOCALGROUP_INFO_1)bufptr;

        for (DWORD i = 0; i < entriesread; i++, Info++) {
            printf("index:%02u, name:%ls, comment:%ls\n", i + 1, Info->lgrpi1_name, Info->lgrpi1_comment);
        }
    } else {
    }

    NetApiBufferFree(bufptr);

    return Status;
}


EXTERN_C
__declspec(dllexport)
int WINAPI DisabledAccount(_In_opt_  LPCWSTR servername OPTIONAL, _In_ LPCWSTR username)
/*
下面的代码示例演示如何通过调用 NetUserSetInfo 函数来禁用用户帐户。
代码示例填充 USER_INFO_1008 结构的 usri1008_flags 成员，并指定值UF_ACCOUNTDISABLE。
然后，该示例调用 NetUserSetInfo，并将信息级别指定为 0。

测试心得：
设置之前先获取已有的属性，再与上设置的属性，否则原有的属性会消失。

https://learn.microsoft.com/zh-cn/windows/win32/api/lmaccess/nf-lmaccess-netusersetinfo
*/
{
    DWORD dwLevel = 1008;
    USER_INFO_1008 ui;

    // Fill in the USER_INFO_1008 structure member.
    // UF_SCRIPT: required.
    ui.usri1008_flags = UF_SCRIPT | UF_ACCOUNTDISABLE;
    // Call the NetUserSetInfo function to disable the account, specifying level 1008.
    NET_API_STATUS nStatus = NetUserSetInfo(servername, username, dwLevel, (LPBYTE)&ui, NULL);
    // Display the result of the call.
    if (nStatus == NERR_Success)
        fwprintf(stderr, L"User account %s has been disabled\n", username);
    else
        fprintf(stderr, "A system error has occurred: %d\n", nStatus);

    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

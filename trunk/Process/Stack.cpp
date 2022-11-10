#include "pch.h"
#include "Stack.h"


//////////////////////////////////////////////////////////////////////////////////////////////////


PVOID GetFunctionAddressByReturnAddress64(PVOID ReturnAddress)
/*
���ܣ����ݷ��ص�ַ��call֮��ĵ�һ��ָ��ĵ�ַ����ȡҪcall�ĺ����ĵ�ַ��
*/
{
    PVOID FunctionAddress = nullptr;
    int Offset = 0;

    if (!ReturnAddress) {
        return FunctionAddress;
    }

    PVOID NearCallAddress = (PVOID)((PBYTE)ReturnAddress - 5);
    PVOID FarCallAddress = (PVOID)((PBYTE)ReturnAddress - 6);

    if (0xe8 != *(PBYTE)NearCallAddress && htons(0xff15) != *(PWORD)FarCallAddress) {

        return FunctionAddress;
    }

    if (0xe8 == *(PBYTE)NearCallAddress) {
        Offset = *(PULONG)((PBYTE)NearCallAddress + 1);    
    }

    if (htons(0xff15) == *(PWORD)FarCallAddress) {
        Offset = *(PULONG)((PBYTE)FarCallAddress + 2);        
    }

    if (!Offset) {

        return FunctionAddress;
    }    

    if (Offset) {
        FunctionAddress = (PVOID)((SIZE_T)ReturnAddress + Offset);
    } else {
        Offset = !Offset;
        Offset++;
        FunctionAddress = (PVOID)((SIZE_T)ReturnAddress - Offset);
    }

    return FunctionAddress;
}


//////////////////////////////////////////////////////////////////////////////////////////////////


EXTERN_C
__declspec(dllexport)
void WINAPI DumpStack()
/*
���ܣ���ӡ������windbg��kv���

*/
{
    //In Windows XP and Windows Server 2003, the sum of the FramesToSkip and FramesToCapture parameters must be less than 63.
    ULONG FramesToSkip = 0;//The number of frames to skip from the start of the back trace. 
    ULONG FramesToCapture = 62;//The number of frames to be captured.     

    //http://msdn.microsoft.com/zh-cn/library/windows/hardware/Dn613940(v=vs.85).aspx
    int stacklength = 32 * 1024;//ȡX86/X64/IA64��ջ�����ֵ��

    PVOID CallersAddress = _ReturnAddress();

    PVOID * BackTrace = 0;//An array of pointers captured from the current stack trace. 
    BackTrace = (PVOID *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, stacklength); 
    _ASSERTE(BackTrace);

    USHORT ncf = 0;//The number of captured frames
    ULONG BackTraceHash = 0;
    ncf = RtlCaptureStackBackTrace(FramesToSkip, FramesToCapture, BackTrace, &BackTraceHash);

    USHORT i = 0;
    for (PVOID * pBackTrace = BackTrace; i < ncf; pBackTrace++, i++) {
        PVOID FunctionAddress = GetFunctionAddressByReturnAddress64(*pBackTrace);
        printf("index:%d\tRetAddr:%p\tFunctionAddress:%p.\r\n", i, *pBackTrace, FunctionAddress);
    }

    HeapFree(GetProcessHeap(), 0, BackTrace);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

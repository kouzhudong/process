#include "Performance.h"


void PerformanceTest()
/*

这里测试的，包括注释掉的，都是成功的代码。
*/
{
    //CollectPerformanceData(L"\\Processor(*)\\% Processor Time", PDH_FMT_DOUBLE);
    //CollectPerformanceData(L"\\Processor(*)\\% User Time", PDH_FMT_DOUBLE);

    //CollectPerformanceData(L"\\Processor Information(*)\\% Processor Time", PDH_FMT_DOUBLE);//类似：L"\\Processor(*)\\% Processor Time"

    //CollectPerformanceData(L"\\Memory\\% Committed Bytes In Use", PDH_FMT_DOUBLE);

    //CollectPerformanceData(L"\\PhysicalDisk(*)\\% Disk Time", PDH_FMT_DOUBLE);
    //CollectPerformanceData(L"\\PhysicalDisk(*)\\% Idle Time", PDH_FMT_DOUBLE);

    //CollectPerformanceDatas(L"\\Process(*)\\% Processor Time", PDH_FMT_DOUBLE);
    //CollectPerformanceDatas(L"\\Process(notepad)\\% Processor Time", PDH_FMT_DOUBLE);
    //CollectPerformanceDatas(L"\\Process(svchost#1)\\% Processor Time", PDH_FMT_DOUBLE);//是索引不是PID，从1开始。
    //CollectPerformanceDatas(L"\\Process(procexp_2648)\\% Processor Time", PDH_FMT_DOUBLE);
    //CollectPerformanceDatas(L"\\Process(*)\\Working Set", PDH_FMT_LARGE);
    //CollectPerformanceDatas(L"\\Process(*)\\ID Process", PDH_FMT_LONG);//类似于PS命令：(Get-Counter "\Process(notepad*)\ID Process").CounterSamples
    //CollectPerformanceDatas(L"\\Process(svchost#1)\\ID Process", PDH_FMT_LONG);
    //CollectPerformanceDatas(L"\\.NET CLR Memory(*)\\Process ID", PDH_FMT_LONG);//仅仅列出.NET进程。

    //https://github.com/dotnet/docs/issues/17858
    //HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\PerfProc\Performance
    //"ProcessNameFormat"=dword:00000002
    //CollectPerformanceDatas(L"\\Process(svchost_1176)\\ID Process", PDH_FMT_LONG);

    //EnumeratingProcessObjects();
    //EnumObjectItems(L"Process");
    //EnumObjectItems(L"Thread");

    //EnumCountersObjects();
    //EnumCountersMachines();
}

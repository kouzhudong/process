#include "Performance.h"


void PerformanceTest()
/*

������Եģ�����ע�͵��ģ����ǳɹ��Ĵ��롣
*/
{
    //CollectPerformanceData(L"\\Processor(*)\\% Processor Time", PDH_FMT_DOUBLE);
    //CollectPerformanceData(L"\\Processor(*)\\% User Time", PDH_FMT_DOUBLE);

    //CollectPerformanceData(L"\\Processor Information(*)\\% Processor Time", PDH_FMT_DOUBLE);//���ƣ�L"\\Processor(*)\\% Processor Time"

    //CollectPerformanceData(L"\\Memory\\% Committed Bytes In Use", PDH_FMT_DOUBLE);

    //CollectPerformanceData(L"\\PhysicalDisk(*)\\% Disk Time", PDH_FMT_DOUBLE);
    //CollectPerformanceData(L"\\PhysicalDisk(*)\\% Idle Time", PDH_FMT_DOUBLE);

    CollectPerformanceDatas(L"\\Process(*)\\% Processor Time", PDH_FMT_DOUBLE);
    //CollectPerformanceDatas(L"\\Process(notepad)\\% Processor Time", PDH_FMT_DOUBLE);
    //CollectPerformanceDatas(L"\\Process(svchost#1)\\% Processor Time", PDH_FMT_DOUBLE);//����������PID����1��ʼ��
    //CollectPerformanceDatas(L"\\Process(*)\\Working Set", PDH_FMT_LARGE);
    //CollectPerformanceDatas(L"\\Process(*)\\ID Process", PDH_FMT_LONG);//������PS���(Get-Counter "\Process(notepad*)\ID Process").CounterSamples
    //CollectPerformanceDatas(L"\\Process(svchost#1)\\ID Process", PDH_FMT_LONG);

    //EnumeratingProcessObjects();
    //EnumObjectItems(L"Process");

    //EnumCountersObjects();
    //EnumCountersMachines();
}

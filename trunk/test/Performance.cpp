#include "Performance.h"


void PerformanceTest()
{
    //CollectPerformanceData(L"\\Processor(*)\\% Processor Time");
    //CollectPerformanceData(L"\\Processor(*)\\% User Time");

    //CollectPerformanceData(L"\\Processor Information(*)\\% Processor Time");//���ƣ�L"\\Processor(*)\\% Processor Time"

    //CollectPerformanceDatas(L"\\Process(*)\\% Processor Time");
    //CollectPerformanceDatas(L"\\Process(notepad)\\% Processor Time");
    CollectPerformanceDatas(L"\\Process(svchost#1)\\% Processor Time");//����������PID����1��ʼ��
    //CollectPerformanceDatas(L"\\Process(*)\\Working Set");

    //CollectPerformanceData(L"\\Memory\\% Committed Bytes In Use");

    //CollectPerformanceData(L"\\PhysicalDisk(*)\\% Disk Time");
    //CollectPerformanceData(L"\\PhysicalDisk(*)\\% Idle Time");

    //EnumeratingProcessObjects();
    //EnumObjectItems(L"Process");

    //EnumCountersObjects();
    //EnumCountersMachines();
}

/*
���ܣ����ܲ��ԡ�

PDH����֤Ҫ������ܼ�����������������������Դ��������

����������Դ��������ʾ����һ�µģ�������Դ�������ǻ������ܼ��������ġ�

�����������ʾ��CPU�����Ǹ������ܼ�������ֵ��
���������������ʾ��CPU���Ǵ��ڸ������̵�CPU�ĺͣ�������Щ���̵�CPU����1%����

�����������������������Դ������������ڴ�ʹ�������Ǵ������ܼ�������ֵ

���Ҫ�г�Memory�µļ����������е����������֣�������ps�����У�
(Get-Counter -ListSet * | where {$_.CounterSetName -eq 'Memory'}).Paths
ע�⣺�����Ҫ�ȴ������ӡ������ű����������
https://wazuh.com/blog/monitoring-windows-resources-with-performance-counters/

����PID����Ϣ��˼���������Ľ����
 Tip
Starting in Windows 10 20H2, you can avoid this issue by using the new Process V2 counterset. 
The Process V2 counterset includes the process ID in the instance name. 
This avoids the inconsistent results that appear with the original Process counterset.
https://learn.microsoft.com/en-us/windows/win32/perfctrs/collecting-performance-data
*/


#pragma once

#include "..\inc\Process.h"
#include "pch.h"

class Performance
{

};

void PerformanceTest();
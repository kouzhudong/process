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



*/


#pragma once

#include "..\inc\Process.h"
#include "pch.h"

class Performance
{

};

void PerformanceTest();
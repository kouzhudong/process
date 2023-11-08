/*
 Performance Data Helper (PDH) library

 ��ʾ�Ͳ�ѯ����PID/TID�����ܼ�������
 HKLM\System\CurrentControlSet\Services\Perfproc\Performance
 ProcessNameFormat �� ThreadNameFormat
 ���ǣ�������������ϵͳ������Ч��Ӧ���������еİ汾��
 https://learn.microsoft.com/zh-cn/windows/win32/perfctrs/handling-duplicate-instance-names
*/

/*
����PdhGetFormattedCounterValue����PDH_CALC_NEGATIVE_DENOMINATOR�Ĵ���

https://learn.microsoft.com/zh-cn/windows/win32/perfctrs/pdh-error-codes
https://learn.microsoft.com/zh-cn/windows/win32/perfctrs/checking-pdh-interface-return-values
https://github.com/DerellLicht/derbar/blob/master/system.cpp
https://support.microfocus.com/kb/doc.php?id=7010545

 In these situations, you have an option to ignore this return value and retry a bit later like 1 second to get the new values.
*/

#pragma once

#include <pdh.h> //������LMErrlog.hͬʱ��������
#pragma comment (lib,"pdh.lib")

# process
进程相关的一些代码<br>
<br>
本库说明：<br>
本库主要包含进程相关的内容，如：<br>
1.进程及环境变量，Job，PEB<br>
2.线程，TEB，APC，TLS，模拟/代理<br>
3.模块，<br>
4.服务，<br>
5.计划任务，<br>
6.关机<br>
7.内存，堆，栈，文件映射<br>
8.句柄，对象<br>
9.Token，用户，回话等。<br>
<br>
特别注释：文件映射不属于文件，属于内存管理（对象或句柄）的概念范畴。<br>
<br>
<br>
本库的设计的几个规则：<br>
1.尽量不调用日志函数。<br>
  刚开始的时候还考虑是否使用日志，以及何种日志。<br>
2.因为上一条，所以代码失败要返回详细的信息，也就是失败的原因。<br>
3.因为上两条的原因，所以使用者要检查返回值，并记录日志。<br>
4.代码不会主动抛异常。代码尽量屏蔽异常。但是不代表代码中没有异常。<br>
  代码尽量捕捉异常并返回信息。<br>
5.导出（对外公开的）的函数都是NTAPI调用格式。<br>
6.代码尽量使用SAL（source-code annotation language:源代码批注语言）。<br>
7.代码格式类似go和python.<br>
8.代码尽量不使用硬编码。<br>
9.代码开启静态的语法检查（启用Microsoft Code Analysis, 关闭Clang-Tidy）。<br>
10.警告的等级是4级，且将警告视为错误。<br>
11.代码的运行要过：驱动程序校验器/应用程序校验器/gflags.exe.<br>
12.禁止使用断言（估计现在代码中还有不少断言）。<br>
13.接口的参数只有基本类型和指针（没有类，模板和引用）。<br>
14.只依赖操作系统的库，不再依赖第三方的库，包括CRT。<br>
15.所有接口皆为C接口，即EXTERN_C。<br>
16.C语言标准选择：ISO C17 (2018)标准 (/std:c17)。  <br>
17.C++语言标准选择：ISO C++17 标准 (/std:c++17) 或者：预览 - 最新 C++ 工作草案中的功能 (/std:c++latest)。  <br>
<br>
<br>
特别说明：<br>
本库只提供c/c++调用接口文档，<br>
其他的，如：ASM，C#，GO，PYTHON，JAVA，PHP等自行定义。<br>
<br>
<br>
注意：<br>
接口或者回调的调用方式。__stdcall or __cdecl or __fastcall。<br>
<br>

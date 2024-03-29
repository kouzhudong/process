/*
这里的Memory仅仅指Virtual memory
但是，排除Heap和File mapping
可以包括：Data Execution Prevention (DEP)和Address Windowing Extensions (AWE)。
*/

#pragma once


#include "pch.h"


// Use to convert bytes to KB
#define DIV 1024

// Specify the width of the field in which to print the numbers. 
// The asterisk in the format specifier "%*I64d" takes an integer 
// argument and uses it to pad and right justify the number.
#define WIDTH 2

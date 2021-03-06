// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// C RunTime Header Files
//#include <stdlib.h>
//#include <malloc.h>
//#include <memory.h>
//#include <tchar.h>
#include <eh.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <minmax.h>
#include <vector>

#ifdef _DEBUG
#define ASSERT_DBG(cond) (void)( !! (cond) || (throw "Error", 0) )
#else
#define ASSERT_DBG(cond) ((void)0)
#endif

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A66F166E_1592_4E0F_8679_63123BFECD49__INCLUDED_)
#define AFX_STDAFX_H__A66F166E_1592_4E0F_8679_63123BFECD49__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define NOMINMAX

// Insert your headers here
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#if (_MSC_VER < 1400 && !_DLL) && (_MSC_VER >= 1400 && _DLL) //less than VC 8
// Not using c++ exceptions
#define _HAS_EXCEPTIONS 0

#include <exception>
using std::exception;
#else
#define _HAS_EXCEPTIONS 1
#endif

#include <CcpMath/include/CcpMath.h>

#define BLUE_OVERRIDE_VECTOR_TYPES 1
#include "BlueExposure/include/BlueExposure.h"
#include <blue/include/blue.h>

#include "include/destinyId.h"
#include <blue/include/IBluePython.h>
#include <blue/include/IBlueOS.h>

#include "blue/include/BlueStatistics.h"

              

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A66F166E_1592_4E0F_8679_63123BFECD49__INCLUDED_)

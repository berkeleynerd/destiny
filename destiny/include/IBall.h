/* 
	*************************************************************************

	ITriFunction.h

	Author:    Hilmar Veigar Pétursson
	Created:   August 2001
	OS:        Win32
	Project:   Trinity

	Description:   

		Yeap


	Dependencies:

		DirectX 8.0, Probably more, ytbd.

	(c) CCP 2000

	*************************************************************************
*/

#ifndef _IBall_H_
#define _IBall_H_

#include <trinity/include/ITriFunction.h>

BLUE_INTERFACE(IBall) : IRoot
{	
	virtual void GetDistances(
		double *surfaceDist, double *centerDist
		) = 0;
};

#endif
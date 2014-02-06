////////////////////////////////////////////////////////////
//
//    Creator:   Filipp Pavlov
//    Created:   May 2013
//    Copyright: CCP 2013
//

#pragma once
#ifndef Quaternion_H
#define Quaternion_H

#define BLUE_OVERRIDE_VECTOR_TYPES 1

// --------------------------------------------------------------------------------------
// Description:
//   A simple Quaternion type for destiny compatible with trinity interfaces.
// --------------------------------------------------------------------------------------
struct Quaternion
{
	Quaternion()
	{
	}

	Quaternion( float vx, float vy, float vz, float vw )
		:x( vx ),
		y( vy ),
		z( vz ),
		w( vw )
	{
	}

	float x, y, z, w;
};

#endif
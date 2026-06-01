// Copyright © 2023 CCP ehf.

#pragma once
#ifndef COLLISION_BALL_PROPERTIES_H
#include "CollisionStructures.h"
#include "Ball.h"

// To save CPU cycles, cache the dynamical information for each ball
// through the entire checks.  This information is anyway calculated
// later in the Evolve call to update the information.  Keep track of
// all collisions.
class CollisionBallProperties
{
public:
	// The fudge factor we use when comparing times.
	const double m_fuzzyDelta;

	// We need to know what we are working with
	Ball* m_ball;

	// Stores ball locations for at least t=0 and t=dt.
	// Stored in order, pre-allocated memory for the maximum number of iterations.
	// Any addition of an impact invalidates later location, so having it in order
	// is perfectly fine.
	std::vector<BallLocation> m_locations;

	// This is the maximum time allowed.  Set when initialized.
	double m_maxTime;

	// Keep track of times to check for new collisions
	double m_tStart, m_tEnd;

	// Pre-calculate for convenience
	float m_tau;

	// Default initialization of the variables
	CollisionBallProperties();

	// Invalidates all data and calculates the locations for t=0 and t=t_end
	// t_end should be the end of the timestep, which is stored for later reference.
	void Initialize( Ball* ball, double t_end );

	// Adds the impact to the current structure.
	// Invalidates any later impacts and sets the index to this impact as
	// the latest index.
	// Returns times and IDs of the colliders of any removed impact that must be
	// checked for free balls.
	std::vector<std::pair<double, ID>> AddImpact( const CollisionImpact& impact );

	// Removes the impact from the structure.  Needed for when
	// collision with another free ball is invalidated.
	// Returns IDs and times to colliders of any other impact removed.
	std::vector<std::pair<double, ID>> RemoveImpact( double time, ID collider );

	// This is entended to be used to update the calculations at dt, given in the initialization.
	// Adds one location at m_maxTime, assuming it is larger then the current
	// last point.  Updates index_to_last to point to this
	void CalculateEnd();

	// Returns true if colliderID is already involved in collisions within fuzzy delta
	// Time of impact is fraction in last step from locations[-2] to locations[-1]
	bool IgnoreCollision( ID colliderID, double timeOfImpact );

	// Find the index in the locations, given time
	// The index should point to a place in the location so time falls
	// in the range [index, index+1), with a fuzzy boundary.
	// If the index is the last one, it is because it is equal to the last one.
	size_t FindLocationsIndex( double time );

	// Calculate a position for any time.
	void CalculatePosition( Vector3d& position, Vector3d& velocity, double time );

	// Get rotations for any time
	void CalculateRotation( Quaternion& rotation, Vector3& omega, double time );
};

#define COLLISION_BALL_PROPERTIES_H
#endif

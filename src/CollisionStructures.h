// Copyright © 2023 CCP ehf.

#pragma once
#ifndef COLLISION_STRUCTURES_H
#define COLLISION_STRUCTURES_H


#include "Partitionable.h"

// This is for convenience to pass into the CollisionBallProperties
struct CollisionImpact
{
	double timeOfImpact;
	ID collider;
	Vector3d velocityChange;
	Vector3 angularVelocityChange;
};

// To save CPU cycles, only calculate the impact that happens first for each
// ball.  Need only store the time of impact, the normal, the collider, and the
// indices to the miniballs involved in the impact.  mbIndex2 is set to -2 if
// the collider is static.
struct CollisionItem
{
	double timeOfImpact; // The time of impact, measure from current new time.
	Vector3d normal; // This should point towards the current ball, always.
	Partitionable* collider; // The item we are colliding with.
	int mbIndex1, mbIndex2; // The miniball indices in the collision, second only if colliding with another free ball
	std::unique_ptr<CollisionImpact> impact;

	// Proper initialization of mbIndex2
	CollisionItem();
};

// Structure for storing information on a single point in the calculation
struct BallLocation
{
	double time; // The time of the calculation
	ID collider; // Set to -1 if there is no collision, applies to start and end only
	Vector3d p, v; // Position at impact, velocity after impact
	Vector3 omega; // rotational velocity after impact
	Quaternion rot; // Rotation at impact
	// Need to set time and collider to default values
	BallLocation();
};

#endif

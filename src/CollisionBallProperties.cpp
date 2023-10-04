#include "CollisionBallProperties.h"
#include "Ballpark.h"
#include "Settings.h"

CollisionBallProperties::CollisionBallProperties() :
	m_fuzzyDelta( 1e-4 ),
	m_ball( nullptr )
{
	m_locations.reserve( s_collisionMaxIterations + 2 );
}

void CollisionBallProperties::Initialize( Ball* _ball, double tEnd )
{
	m_ball = _ball;
	m_maxTime = tEnd;
	m_tStart = 0.0;
	m_tEnd = tEnd;

	// This function invalidates WITHOUT CHECKS any previous collisions
	m_locations.resize( 2 );

	// First location is at t=0
	m_locations[0].time = 0.0;
	m_locations[0].collider = -1;
	m_locations[0].p = m_ball->mNewPos;
	m_locations[0].v = m_ball->mNewVel;

	// Second location at t=tEnd
	m_locations[1].time = tEnd;
	m_locations[1].collider = -1;
	m_locations[1].p = m_locations[0].p;
	m_locations[1].v = m_locations[0].v;

	m_ball->mPark->Integrate( m_locations[1].p, m_locations[1].v, m_ball->mLastG + m_ball->mLastC, m_ball->mMass * m_ball->mAgility, m_ball->mPark->mFriction, m_ball->mTimeFactor, tEnd );
}

std::vector<std::pair<double, ID>> CollisionBallProperties::AddImpact( const CollisionImpact& impact )
{
	std::vector<std::pair<double, ID>> invalidated;

	// We only add impacts that are newer than the end time
	// This can only happen if they are simultaneous with different free balls
	if( m_tEnd <= impact.timeOfImpact )
		return invalidated;

	// Start by finding the right place to put this impact and add any invalidated to the output
	size_t index = m_locations.size();
	for( ; index > 0; --index )
	{
		if( impact.timeOfImpact >= m_locations[index - 1].time )
			break;

		if( m_locations[index - 1].collider != -1 )
			invalidated.push_back( std::pair<double, ID>( m_locations[index - 1].time, m_locations[index - 1].collider ) );
	}

	// Resize the locations vector and add the impact
	m_locations.resize( index + 1 );

	m_locations[index].collider = impact.collider;
	m_locations[index].time = impact.timeOfImpact;

	// If the index is 0, our job is easy
	if( index == 0 )
	{
		m_locations[0].p = m_ball->mNewPos;
		m_locations[0].v = m_ball->mNewVel + impact.velocityChange;
	}
	else
	{
		// Use the previous location as a starting point
		const auto& prevLoc = m_locations[index - 1];
		auto& currLoc = m_locations[index];

		currLoc.p = prevLoc.p;
		currLoc.v = prevLoc.v;

		m_ball->mPark->Integrate( currLoc.p, currLoc.v, m_ball->mLastG + m_ball->mLastC, m_ball->mMass * m_ball->mAgility, m_ball->mPark->mFriction, m_ball->mTimeFactor, currLoc.time - prevLoc.time );
		currLoc.v += impact.velocityChange;
	}

	// Only the end time is updated to match the added collision
	m_tEnd = impact.timeOfImpact;

	return invalidated;
}

std::vector<std::pair<double, ID>> CollisionBallProperties::RemoveImpact( double time, ID collider )
{
	std::vector<std::pair<double, ID>> invalidated;

	// Make sure this impact exists in our history
	size_t index = 0;
	bool found = true;

	for( size_t i = 0; i < m_locations.size(); ++i )
	{
		if( m_locations[i].collider == collider && std::fabs( m_locations[i].time - time ) < m_fuzzyDelta )
		{
			index = i;
			found = true;
			break;
		}
	}

	// Nothing to do if index not found.
	if( !found )
		return invalidated;

	if( index == 0 )
	{
		// Apparently this can happen, just fix the starting point
		// The positions and rotation are correct.
		m_locations[0].collider = -1;
		m_locations[0].v = m_ball->mNewVel;
	}

	// Invalidate any later collisions
	for( size_t i = index + 1; i < m_locations.size(); ++i )
	{
		if( m_locations[i].collider != -1 )
			invalidated.push_back( std::pair<double, ID>( m_locations[i].time, m_locations[i].collider ) );
	}

	// We need a final step at the end, always have to recalculate
	m_locations.resize( index > 0 ? index : 1 );
	CalculateEnd();

	return invalidated;
}

void CollisionBallProperties::CalculateEnd()
{
	// Only add this if needed
	if( m_maxTime > m_locations.back().time )
	{
		const auto& prevLoc = m_locations.back();
		m_locations.push_back( BallLocation() );

		auto& currLoc = m_locations.back();

		currLoc.time = m_maxTime;
		currLoc.p = prevLoc.p;
		currLoc.v = prevLoc.v;

		m_ball->mPark->Integrate( currLoc.p, currLoc.v, m_ball->mLastG + m_ball->mLastC, m_ball->mMass * m_ball->mAgility, m_ball->mPark->mFriction, m_ball->mTimeFactor, m_maxTime - prevLoc.time );

		// Update the new search times.
		m_tEnd = m_maxTime;
		m_tStart = prevLoc.time;
	}
}

bool CollisionBallProperties::IgnoreCollision( ID colliderID, double timeOfImpact )
{
	if( m_locations.size() < 2 || timeOfImpact > m_fuzzyDelta )
		return false;

	// See if we already have an impact with the collider at this point.
	auto rit = m_locations.rbegin() + 1;
	const double timeLowerBound = rit->time - m_fuzzyDelta;
	for( ; rit != m_locations.rend(); ++rit )
	{
		if( rit->time < timeLowerBound )
			break;
		if( rit->collider == colliderID )
			return true;
	}
	return false;
}

size_t CollisionBallProperties::FindLocationsIndex( double time )
{
	// No need to do anything fancy because the maximum size is small.
	size_t index = 0;
	while( index < m_locations.size() && m_locations[index].time + m_fuzzyDelta < time )
		++index;

	// We stop when the index is one too large, or at first index.
	if( index != 0 )
		--index;

	return index;
}

void CollisionBallProperties::CalculatePosition( Vector3d& position, Vector3d& velocity, double time )
{
	// Find the index
	size_t i = FindLocationsIndex( time );

	position = m_locations[i].p;
	velocity = m_locations[i].v;

	// We can only be equal in time to the location at index.
	if( std::fabs( m_locations[i].time - time ) >= m_fuzzyDelta )
	{
		// Need to integrate to get position
		m_ball->mPark->Integrate( position, velocity, m_ball->mLastG + m_ball->mLastC, m_ball->mMass * m_ball->mAgility, m_ball->mPark->mFriction, m_ball->mTimeFactor, ( time - m_locations[i].time ) );
	}
}

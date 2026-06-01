// Copyright © 2023 CCP ehf.

#include "CollisionStructures.h"


CollisionItem::CollisionItem() :
	timeOfImpact( -1.0 ),
	collider( nullptr ),
	mbIndex1( -2 ),
	mbIndex2( -2 ),
	impact( nullptr )
{
}

BallLocation::BallLocation() :
	time( -1.0 ),
	collider( -1 )
{
}

////////////////////////////////////////////////////////////////////////////////
//
// Creator:		Snorri Sturluson
// Created:		October 2012
// Copyright:	CCP 2012
//

#include "StdAfx.h"
#include "Ballpark.h"

const Be::ClassInfo* Ballpark::ExposeToBlue()
{

	EXPOSURE_BEGIN(Ballpark, "")
		MAP_INTERFACE(IEveBallpark)

		////////////////////////////////////////////////////////////////////////////
		//               Ego

		MAP_ATTRIBUTE
		(
			"ego",
			mEgo,
			"id of ball",
			Be::READWRITE| Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               Ego

		MAP_ATTRIBUTE
		(
			"probe",
			mProbe,
			"id of probe ball",
			Be::READWRITE| Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               solarsystemID
		MAP_ATTRIBUTE
		(
			"solarsystemID",
			mSolarsystemID,
			"ID of solarsystem",
			Be::READWRITE | Be::PERSIST
		)


		////////////////////////////////////////////////////////////////////////////
		//               isMaster
		MAP_ATTRIBUTE
		(
			"isMaster",
			isMaster,
			"True if this is a server",
			Be::READWRITE | Be::PERSIST
		)
		////////////////////////////////////////////////////////////////////////////
		//               time
		MAP_ATTRIBUTE
		(
			"time",
			mTime,
			"na",
			Be::READWRITE | Be::PERSIST
		)

		////////////////////////////////////////////////////////////////////////////
		//               Current Time
		MAP_ATTRIBUTE
		(
			"currentTime",
			mCurrentTime,
			"Time stamp for current state",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               lag damping
		MAP_ATTRIBUTE
		(
			"lagDamping",
			mLagDamping,
			"Dampening factor for camera lag",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               friction coefficient
		MAP_ATTRIBUTE
		(
			"friction",
			mFriction,
			"Friction coefficient of space",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               friction coefficient
		MAP_ATTRIBUTE
		(
			"warpSpeed",
			mWarpSpeed,
			"Warp speed in AU/s",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               time step of simulation in msec
		MAP_ATTRIBUTE
		(
			"tickInterval",
			mTickInterval,
			"Time step of simulation (milliseconds)",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"isRunning",
			mHaveTicks,
			"True if simulation is being ticked",
			Be::READ
		)
		////////////////////////////////////////////////////////////////////////////
		//               custom coefficient 1
		MAP_ATTRIBUTE
		(
			"para1",
			mPara1,
			"custom 1",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               custom coefficient 2
		MAP_ATTRIBUTE
		(
			"para2",
			mPara2,
			"custom 2",
			Be::READWRITE
		)
		////////////////////////////////////////////////////////////////////////////
		//               custom int 1
		MAP_ATTRIBUTE
		(
			"int1",
			mInt1,
			"Custom int 1",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               Custom int 2
		MAP_ATTRIBUTE
		(
			"int2",
			mInt2,
			"Custom int 2",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               Moribund Ball Removals Per Frame
		MAP_ATTRIBUTE
		(
			"moribundBallRemovalCount",
			mMoribundBallRemovalCount,
			"Controls how many moribund balls can be prematurely removed during one frame",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               Moribund Ball Removal Time Buffer
		MAP_ATTRIBUTE
		(
			"moribundBallRemovalBuffer",
			mMoribundBallRemovalBuffer,
			"Controls the number of ticks, during which Destiny is allowed to prematurely remove moribund balls.",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               keys
		MAP_ATTRIBUTE
		(
			"balls",
			mBalls.mDict,
			"Dict of balls",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               keys
		MAP_ATTRIBUTE
		(
			"globals",
			mGlobals.mDict,
			"Dict of global balls",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               keys
		MAP_ATTRIBUTE
		(
			"bubbles",
			bubbles,
			"Dict of balls",
			Be::READ
		)

		////////////////////////////////////////////////////////////////////////////
		//               keys
		MAP_ATTRIBUTE
		(
			"bubbleInteractives",
			bubbleInteractives,
			"dict of interactives in bubble",
			Be::READ
		)

		////////////////////////////////////////////////////////////////////////////
		//               keys
		MAP_ATTRIBUTE
		(
			"bubbleKeepAlives",
			bubbleKeepAlives,
			"set of keep-alive ballIDs in bubble",
			Be::READ
		)

		////////////////////////////////////////////////////////////////////////////
		//               keys
		MAP_ATTRIBUTE
		(
			"bubbleKeepAliveBalls",
			bubbleKeepAliveBalls,
			"Set of ballIDs that automatically keep bubbles alive",
			Be::READ
		)

        ////////////////////////////////////////////////////////////////////////////
		//               keys
		MAP_ATTRIBUTE
		(
			"bubbleSubs",
			mBubbleSubscriptions,
			"Bubble that park subscribes to",
			Be::READ
		)

		////////////////////////////////////////////////////////////////////////////
		//               Roll Speed Acceleration
		MAP_ATTRIBUTE
		(
			"rollSpeedAcceleration",
			mRollSpeedAcceleration,
			"",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               Roll Speed Decay
		MAP_ATTRIBUTE
		(
			"rollSpeedDecay",
			mRollSpeedDecay,
			"",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               Roll Acceleration
		MAP_ATTRIBUTE
		(
			"rollAcceleration",
			mRollAcceleration,
			"",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               Roll Decay
		MAP_ATTRIBUTE
		(
			"rollDecay",
			mRollDecay,
			"",
			Be::READWRITE
		)

		////////////////////////////////////////////////////////////////////////////
		//               __init__
		MAP_METHOD_AS_METHOD
		(
			"__init__",
			Py__init__,
			"Constructor arguments"
		)
		MAP_METHOD_AS_METHOD
		(
			"AddBall",
			PyAddBall,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"AddCapsule",
			PyAddCapsule,
			"Add a capsule shaped collision object to the ballpark. Create the object if necessary."
		)
		MAP_METHOD_AS_METHOD
		(
			"AddOrientedBox",
			PyAddOrientedBox,
			"Add a box shaped collision object to the ballpark. Create the object if necessary."
		)
		MAP_METHOD_AS_METHOD
		(
			"FollowBall",
			PyFollowBall,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"MissileFollow",
			PyMissileFollow,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"WarpTo",
			PyWarpTo,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"GotoPoint",
			PyGotoPoint,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"GotoDirection",
			PyGotoDirection,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"Orbit",
			PyOrbit,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"Stop",
			PyStop,
			"no comment"
		)
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"RemoveBall",
			RemoveBall,
			1,
			"Schedules the Ball for removal during the next evolution."
		)
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"RemoveCapsule",
			RemoveCapsule,
			1,
			"Schedules the capsule for removal during the next evolution."
		)
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"RemoveOrientedBox",
			RemoveOrientedBox,
			1,
			"Schedules the oriented box for removal during the next evolution."
		)
		MAP_METHOD_AS_METHOD
		(
		    "GetBallIdsInRangeOfTriangle",
			GetBallIdsInRangeOfTriangle,
			"Gets the balls that are within the given range of the triangle specified by the balls position and two vectors that point from the center of the ball to the other two points in the triangle."
		)
		MAP_METHOD_AS_METHOD
		(
		    "GetBallIdsInCapsule",
			GetBallIdsInCapsule,
			"Gets the balls that are within the capsule specified by the balls position, a vector that points from the center of the ball to the other end of the capsule, and a radius."
		)
		MAP_METHOD_AS_METHOD
		(
		    "GetBallIdsInCone",
			GetBallIdsInCone,
			"Gets the balls that are within the spherical cone specified by the balls position, a vector that points from the center of the ball to the far end of the cone, and the angle from that vector that the cone should cover, in radians."
		)
		MAP_METHOD_AS_METHOD
		(
			"ClearAll",
			PyClearAll,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetBallMass",
			PySetBallMass,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetBallAgility",
			PySetBallAgility,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetBallRadius",
			PySetBallRadius,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetMaxSpeed",
			PySetMaxSpeed,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetBallGlobal",
			PySetBallGlobal,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetBallMassive",
			PySetBallMassive,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetBallInteractive",
			PySetBallInteractive,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetBallFree",
			PySetBallFree,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetBallVelocity",
			PySetBallVelocity,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetSpeedFraction",
			PySetSpeedFraction,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetNotificationRange",
			PySetNotificationRange,
			"no comment"
		)
        MAP_METHOD_AS_METHOD
        (
			"SetTargetTracking",
				PySetTargetTracking,
			"(srcID, targetID, trackingRange) => Void This sets the target tracking notification that will cause a callback to DoTargetTracking when the range is crossed."
        )
		MAP_METHOD_AS_METHOD
		(
			"SetBallPosition",
			PySetBallPosition,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"SetBallHarmonic",
			PySetBallHarmonic,
			"no comment"
		)

		MAP_METHOD_AS_METHOD
		(
			"InitializeBubbles",
			PyInitializeBubbles,
			"no comment"
		)

		MAP_METHOD_AS_METHOD
		(
			"WriteFullStateToStream",
			PyWriteFullStateToStream,
			"no comment"
		)

		MAP_METHOD_AS_METHOD
		(
			"WriteBallsToStream",
			PyWriteBallsToStream,
			"no comment"
		)

		MAP_METHOD_AS_METHOD
		(
			"ReadFullStateFromStream",
			PyReadFullStateFromStream,
			"no comment"
		)

		MAP_METHOD_AS_METHOD
		(
			"AddProximitySensor",
			PyAddProximitySensor,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"AddMushroom",
			PyAddMushroom,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"RemoveProximitySensor",
			PyRemoveProximitySensor,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"CheckVisibility",
			PyCheckVisibility,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"CloakBall",
			PyCloakBall,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"UncloakBall",
			PyUncloakBall,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"GetAccuracy",
			PyGetAccuracy,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"Start",
			PyStart,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"Pause",
			PyPause,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
			"Evolve",
			PyEvolve,
			"no comment"
		)
		MAP_METHOD_AS_METHOD
		(
		    "GetBallIdsInRange",
			PyGetBallIdsInRange,
			"Get all ball ids within range of given ball"
	    )

		MAP_METHOD_AS_METHOD
		(
		    "GetBallIdsAndDistInRange",
			PyGetBallIdsAndDistInRange,
			"Get all ball ids and distance squared within range of given ball"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "CopyBubbles",
			PyCopyBubbles,
			"Copy bubbles"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "AdditionsAndDeletions",
			PyAdditionsAndDeletions,
			"Diffs of bubbles mapped on interactives"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "SetBoid",
			PySetBoid,
			"Turns boid mode on off"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "GetSurfaceDist",
			PyGetSurfaceDist,
			"Returns distance between ball surfaces (can be negative)"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "GetCenterDist",
			PyGetCenterDist,
			"Returns distance between ball centers"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "ScanCone",
			PyScanCone,
			"Returns list of balls within a cone"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "GetBallBox",
			PyGetBallBox,
			"Returns the (width, (x,y,z)) of the box of the given ball"
	    )
		MAP_METHOD_AS_METHOD
		(
			"GetStaticCollidableBox",
			PyGetStaticCollidableBox,
			"Returns the (width, (x,y,z)) of the box of the given static collision object"
		)
		MAP_METHOD_AS_METHOD
		(
		    "GetRemoteFollowers",
			PyGetRemoteFollowers,
			"Returns a list of all balls following a ball in another bubble"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "GetFollowers",
			PyGetFollowers,
			"Returns a list of all balls following a given ball"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "GetActiveBoxes",
			PyGetActiveBoxes,
			"Returns a list of active partition boxes for a given level"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "SetBoxObject",
			PySetBoxObject,
			"Stores a Python object in a partition box"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "GetBoxObject",
			PyGetBoxObject,
			"Retrieves a Python object stored in a partition box"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "GetBubbleAtCoordinates",
			PyGetBubbleAtCoordinates,
			"Gets the bubbleID of a bubble at the given coordinates, if any. Returns Py_None if there isn't one."
	    )
		MAP_METHOD_AS_METHOD
		(
		    "GetBoxCenter",
			PyGetBoxCenter,
			"Returns center of partition box for given coordinate and level"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "SetBallTroll",
			PySetBallTroll,
			"Sets a ball in troll mode"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "SetBallRigid",
			PySetBallRigid,
			"Sets a ball in rigid mode"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "GetCurrentEgoPos",
			PyGetCurrentEgoPos,
			"Returns the absolute interpolated position of the current ego ball"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "LaunchMissile",
			PyLaunchMissile,
			"Launches a missile"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "SetBallFormation",
			PySetBallFormation,
			"Sets the formation of a ball"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "FormationFollow",
			PyFormationFollow,
			"Joins an existing formation"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "LoadFormations",
			PyLoadFormations,
			"Load formation static data"
	    )
		MAP_METHOD_AS_METHOD
		(
		    "SetBallNotInParkCallback",
			PySetBallNotInParkCallback,
			"Set a python callback to use when you get a ball not in park error"
		)
	    MAP_METHOD_AS_METHOD
		(
			"EntityWarpIn",
			PyEntityWarpIn,
			"Warps an entity into a position without having to enter warp first"
		)
		MAP_METHOD_AS_METHOD
		(
			"AdjustTimes",
			PyAdjustTimes,
			"When you need to move all the time tracking for the park and its balls, this is your man.  If you're not doing this from an OnSimClockRebase, you're probably doing it wrong."
		)
		MAP_METHOD_AS_METHOD
		(
			"GetBallBoxKeys",
			PyGetBallBoxKeys,
			"Get a list of box keys [(level, ix, iy, iz)...] that a ball is in"
		)
		MAP_METHOD_AS_METHOD
		(
			"GetBoxChildren",
			PyGetBoxChildren,
			"Describes the contents of a box: ([child box_keys...], [ballIDs...], [staticCollidableIDs]). Intended mostly as debugging aid"
		)
	EXPOSURE_END()
}
// Copyright © 2000 CCP ehf.

#ifndef _DSTCONSTANTS_H_
#define _DSTCONSTANTS_H_

#include "IDstConstants.h"

void AddDstConstants(PyObject *d);

#define VAL(v) BeCast(v)

#define KV(_k, _d)\
	{#_k, VAL(_k), _d}


static Be::VarChooser DstConstants[] =
{
	{
		"DSTLOCALBALLS",
		VAL(DSTLOCALBALLS),
		"Starting ID for local ball range"
	},
	{
		"DSTNORMALCLOAK",
		VAL(DSTNORMALCLOAK),
		"isCloaked value for normal cloak"
	},
	{
		"DSTRESTORECLOAK",
		VAL(DSTRESTORECLOAK),
		"isCloaked value for restoreing cloak"
	},
	{
		"DSTGMCLOAK",
		VAL(DSTGMCLOAK),
		"isCloaked value for GM cloak"
	},
	{0}
};
BLUE_REGISTER_ENUM( "DstConstants", DSTCONSTANTS, DstConstants );

static Be::VarChooser DstBallMode[] =
{
	{
		"DSTBALL_GOTO",
		VAL(DSTBALL_GOTO),
		"Makes ball attempt to reach the goto point"
	},
	{
		"DSTBALL_FOLLOW",
		VAL(DSTBALL_FOLLOW),
		"Makes ball follow another ball"
	},
	{
		"DSTBALL_STOP",
		VAL(DSTBALL_STOP),
		"Brings ball to a stop"
	},
	{
		"DSTBALL_WARP",
		VAL(DSTBALL_WARP),
		"Moves ball very fast to another location"
	},
	{
		"DSTBALL_ORBIT",
		VAL(DSTBALL_ORBIT),
		"Orbits another ball at some given range"
	},
	{
		"DSTBALL_MISSILE",
		VAL(DSTBALL_MISSILE),
		"Missile tracking a target"
	},
	{
		"DSTBALL_MUSHROOM",
		VAL(DSTBALL_MUSHROOM),
		"Expanding gravity wall"
	},
	{
		"DSTBALL_BOID",
		VAL(DSTBALL_BOID),
		"Swarm like behavior"
	},
	{
		"DSTBALL_TROLL",
		VAL(DSTBALL_TROLL),
		"Free ball that will become fixed after awhile"
	},
	{
		"DSTBALL_FIELD",
		VAL(DSTBALL_FIELD),
		"Force field ball"
	},
	{
		"DSTBALL_MINIBALL",
		VAL(DSTBALL_MINIBALL),
		"Ball flagged as a miniball"
	},
	{
		"DSTBALL_RIGID",
		VAL(DSTBALL_RIGID),
		"A ball that will never move"
	},
	{
		"DSTBALL_FORMATION",
		VAL(DSTBALL_FORMATION),
		"A ball part of a formation"
	},
	{0}
};
BLUE_REGISTER_ENUM( "DstBallMode", DSTBALLMODE, DstBallMode );

static Be::VarChooser DstEventTypeChooser[] =
{
	KV(DST_CREATE,"DoCreate"),
	KV(DST_DESTROY,"DoDestroy"),
	KV(DST_PROXIMITY,"DoProximity"),
	KV(DST_PRETICK,"DoPreTick"),
	KV(DST_POSTTICK,"DoPostTick"),
	KV(DST_COLLISION,"DoCollision"),
	KV(DST_RANGE,"DoRange"),
	KV(DST_MODECHANGE,"DoModeChange"),
	KV(DST_PARTITION,"DoPartition"),
	KV(DST_WARPACTIVATION,"OnActivatingWarp"),
	KV(DST_WARPEXIT,"OnExitWarp"),
	{0}
};
BLUE_REGISTER_ENUM( "DstEventType", DSTMESSAGE, DstEventTypeChooser );

#endif

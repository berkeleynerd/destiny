// Copyright © 2000 CCP ehf.

#ifndef _IDstConstants_H_
#define _IDstConstants_H_

////////////////////////////////////////////////////////////////////////////
// Ball constants
////////////////////////////////////////////////////////////////////////////

// These are the dynamical states of a Ball in Destiny
enum DSTBALLMODE
{
	DSTBALL_GOTO,
	DSTBALL_FOLLOW,
	DSTBALL_STOP,
	DSTBALL_WARP,
	DSTBALL_ORBIT,
	DSTBALL_MISSILE,
	DSTBALL_MUSHROOM,
	DSTBALL_BOID,
	DSTBALL_TROLL,
	DSTBALL_MINIBALL,	
	DSTBALL_FIELD,
	DSTBALL_RIGID,
	DSTBALL_FORMATION
};

enum DESTINYPACKETS
{
	DESTINY_FULLSTATE,
	DESTINY_BALLS
};


enum BALLFLAGS
{
	DSTBALL_ISFREE        = 0x00000001,
	DSTBALL_ISGLOBAL      = 0x00000002,
	DSTBALL_ISMASSIVE     = 0x00000004,
	DSTBALL_ISINTERACTIVE = 0x00000008,
	DSTBALL_ISSPACEJUNK   = 0x00000010,
	DSTBALL_HASMINIBOXES  = 0x00000020,
	DSTBALL_HASMINIBALLS  = 0x00000040,
	DSTBALL_HASMINICAPSULES  = 0x00000080
};

//--------------------------------------------------------------------
// DSTMESSAGE
//--------------------------------------------------------------------
enum DSTMESSAGE
{
	DST_INVALID  = -1,

	// only 'mEvent' member is valid
	DST_RAW		= 0,


	//////////////////////////////////////////
	// Ball messages
	//////////////////////////////////////////
	DST_CREATE,
	DST_DESTROY,
	DST_PROXIMITY,
	DST_PRETICK,
	DST_POSTTICK,
	DST_COLLISION,
	DST_RANGE,
	DST_MODECHANGE,
	DST_PARTITION,
	DST_WARPACTIVATION,
	DST_WARPEXIT
};

enum DSTCONSTANTS
{
	DSTLOCALBALLS = -1073741824,
	DSTNORMALCLOAK = 1,
	DSTRESTORECLOAK = 2,
	DSTGMCLOAK = 3
};

#endif
// Copyright © 2014 CCP ehf.
//
// The park-update wire cores (D-04): Python-free serialization shared by
// the classic thunker wrappers and the embedded recorder. Format doc:
// docs/park-update-stream.md.

#include "stdafx.h"
#include "Ballpark.h"
#include "Ball.h"
#include "Box.h"
#include "Settings.h"

#include <IBluePersist.h>

static CcpLogChannel_t s_chPark = CCP_LOG_DEFINE_CHANNEL( "BallPark" );
static CcpLogChannel_t s_chPThunk = CCP_LOG_DEFINE_CHANNEL( "ParkThunkers" );

size_t destinyParkStreamByteCount = 0;
static size_t& byteCount = destinyParkStreamByteCount;

bool Ballpark::ReadFullStateFromStream(IBlueStreamPtr s, int partial)
{
	int32_t timestamp;

	byteCount += s->Read(&timestamp,sizeof(timestamp));

	mCurrentTime = timestamp;
	Ball *ball;
	VectorOfBalls l;
	while( ( ball = ReadBallFromStream(s, partial) ) )
	{
		l.push_back(ball);
	}

	VectorOfBalls::iterator addition;
	Ball* b;
	for(addition=l.begin(); addition != l.end(); ++addition)
	{
		b = *addition;
		switch(b->mMode)
		{
		case DSTBALL_FOLLOW:
		case DSTBALL_MISSILE:
		case DSTBALL_ORBIT:
		case DSTBALL_FORMATION:
			{
				Ball *dest = mBalls[ID(b->mFollowId)];
				CCP_ASSERT(dest);
				if (!dest) {
					CCP_LOGERR_CH( s_chPark, "ball %I64d to follow not found for ball %I64d", b->mFollowId, b->mId);
					b->mMode = DSTBALL_GOTO;
				} else {
					b->mFollowPtr = dest;
					dest->mFollowers.insert(b->mId);
				}
				break;
			}
		default:
			{
				break;
			}
		}
	}
	return false;
}

Ball* Ballpark::ReadBallFromStream(IBlueStreamPtr s, int partial)
{
	Ball *ball = 0;
	ID id;
	size_t tmpCnt;
	if((tmpCnt = s->Read(&id,sizeof(id))) > 0)
	{
		double mass = 1.0e34;
		float radius;
		Vector3d pos;
		uint8_t flags;
		int8_t isCloaked = 0;
		uint8_t mode;
		float maxVel = 0.0;
		Vector3d vel;
		float agility = 1.0;
		float speedFraction = 0.0;
		int64_t harmonic = -1;
		int32_t corporationID = -1;
		int32_t allianceID = -1;
		Quaternion rot( 0.f, 0.f, 0.f, 1.f );
		Vector3 angVel( 0.f, 0.f, 0.f );
		float rotAgility( 1.0f );
		float maxAngVel( 0.0f );

		byteCount += tmpCnt;

		byteCount += s->Read(&mode           , sizeof(mode));
		byteCount += s->Read(&radius         , sizeof(radius));
		byteCount += s->Read(&pos.x          , sizeof(Vector3d));
		byteCount += s->Read(&flags          , sizeof(flags));

		if(mode!=DSTBALL_RIGID)
		{
			byteCount += s->Read(&mass           , sizeof(mass));
			byteCount += s->Read(&isCloaked      , sizeof(isCloaked));
			byteCount += s->Read(&harmonic       , sizeof(harmonic));
			byteCount += s->Read(&corporationID  , sizeof(corporationID));
			byteCount += s->Read(&allianceID     , sizeof(allianceID));
		}

		if(flags & DSTBALL_ISFREE)
		{
			byteCount += s->Read(&maxVel,sizeof(maxVel));
			byteCount += s->Read(&vel.x,sizeof(Vector3d));
			byteCount += s->Read(&agility,sizeof(agility));
			byteCount += s->Read(&speedFraction,sizeof(speedFraction));
			if( g_useDynamicalOrientation )
			{
				byteCount += s->Read( &rot.x, sizeof( Quaternion ) );
				byteCount += s->Read( &maxAngVel, sizeof( maxAngVel ) );
				byteCount += s->Read( &angVel.x, sizeof( Vector3 ) );
				byteCount += s->Read( &rotAgility, sizeof( rotAgility ) );
			}
		}

		if( g_useDynamicalOrientation )
		{
			ball = AddDynamicallyOrientedBall(
				ID( id ), mass, radius, maxVel,
				maxAngVel,
				bool( flags & DSTBALL_ISFREE ),
				!!( flags & DSTBALL_ISGLOBAL ),
				!!( flags & DSTBALL_ISMASSIVE ),
				!!( flags & DSTBALL_ISINTERACTIVE ),
				!!( flags & DSTBALL_ISSPACEJUNK ),
				pos.x, pos.y, pos.z,
				vel.x, vel.y, vel.z,
				rot.x, rot.y, rot.z, rot.w,
				angVel.x, angVel.y, angVel.z,
				agility,
				rotAgility,
				speedFraction
			);
		}
		else
		{
			ball = AddOldStyleOrientedBall(
				ID(id), mass, radius, maxVel,
				bool(flags & DSTBALL_ISFREE),
				!!(flags & DSTBALL_ISGLOBAL),
				!!(flags & DSTBALL_ISMASSIVE),
				!!(flags & DSTBALL_ISINTERACTIVE),
				!!(flags & DSTBALL_ISSPACEJUNK),
				pos.x, pos.y, pos.z,
				vel.x, vel.y, vel.z,
				agility,
				speedFraction);
		}
		int8_t formationID;
		byteCount += s->Read(&formationID, sizeof(formationID));
		ball->mFormationID = formationID;

		if(partial!=1 && !(flags&DSTBALL_ISFREE))
        {
            for(ssize_t i=ball->mMiniBalls.GetSize()-1 ; i >= 0; i--)
            {
                MiniBall *mb = (MiniBall *)(void *)(ball->mMiniBalls.GetAt(i));
                RemoveBall(mb->mId);
            }
			ball->mMiniBalls.Remove(-1);

			for(ssize_t i=ball->mMiniCapsules.GetSize()-1 ; i >= 0; i--)
			{
				MiniCapsule *mc = (MiniCapsule *)(void *)(ball->mMiniCapsules.GetAt(i));
				RemoveCapsule(mc->mId);
			}
			ball->mMiniCapsules.Remove(-1);

			for(ssize_t i=ball->mMiniBoxes.GetSize()-1 ; i >= 0; i--)
			{
				MiniBox *mbo = (MiniBox *)(void *)(ball->mMiniBoxes.GetAt(i));
				RemoveOrientedBox(mbo->mId);
			}
			ball->mMiniBoxes.Remove(-1);
        }

		ball->mHarmonic = harmonic;
		ball->mCorporationID = corporationID;
		ball->mAllianceID = allianceID;
		ball->mMode = (DSTBALLMODE)mode;
		ball->isCloaked = isCloaked;
		int32_t effectStamp;
		
		switch(mode)
		{
		case DSTBALL_FOLLOW:
			{

				byteCount += s->Read(&ball->mFollowId,    sizeof(ball->mFollowId)    );
				byteCount += s->Read(&ball->mFollowRange, sizeof(ball->mFollowRange) );

				break;
			}
		case DSTBALL_FORMATION:
			{

				byteCount += s->Read(&ball->mFollowId,    sizeof(ball->mFollowId)    );
				byteCount += s->Read(&ball->mFollowRange, sizeof(ball->mFollowRange) );
				byteCount += s->Read(&effectStamp, sizeof(effectStamp) );
				ball->mEffectStamp = effectStamp;

				break;
			}
		case DSTBALL_MISSILE:
			{

				byteCount += s->Read(&ball->mFollowId,    sizeof(ball->mFollowId)    );
				byteCount += s->Read(&ball->mFollowRange, sizeof(ball->mFollowRange) );
				byteCount += s->Read(&ball->mOwnerId,     sizeof(ball->mOwnerId)     );
				byteCount += s->Read(&effectStamp, sizeof(effectStamp) );
				ball->mEffectStamp = effectStamp;
				byteCount += s->Read(&ball->mGoto.x,         sizeof(ball->mGoto)           );

				break;
			}
		case DSTBALL_ORBIT:
			{

				byteCount += s->Read(&ball->mFollowId,    sizeof(ball->mFollowId)    );
				byteCount += s->Read(&ball->mFollowRange, sizeof(ball->mFollowRange) );
				break;
			}
		case DSTBALL_GOTO:
			{
				byteCount += s->Read(&ball->mGoto.x,    sizeof(ball->mGoto)      );

				break;
			}
		case DSTBALL_WARP:
			{
				byteCount += s->Read(&ball->mGoto.x,      sizeof(ball->mGoto)        ); // Warp destination point
				byteCount += s->Read(&effectStamp, sizeof(effectStamp) ); // Timestamp at start of warp
				ball->mEffectStamp = effectStamp;
				byteCount += s->Read(&ball->mLastCollision, sizeof(ball->mLastCollision) ); // Total length of warp
				byteCount += s->Read(&ball->mFollowId,    sizeof(ball->mFollowId)    ); // Minimum range (as double)
				byteCount += s->Read(&ball->mOwnerId,    sizeof(ball->mOwnerId)     ); // Warp factor (speed multiplier)

				break;
			}
		case DSTBALL_MUSHROOM:
			{
				double span;

				byteCount += s->Read(&ball->mFollowRange, sizeof(ball->mFollowRange) );
				byteCount += s->Read(&span              , sizeof(span)               );
				byteCount += s->Read(&effectStamp, sizeof(effectStamp) );
				ball->mEffectStamp = effectStamp;
				byteCount += s->Read(&ball->mOwnerId,     sizeof(ball->mOwnerId)     );
				ball->mGoto.x = span;

				break;
			}
		case DSTBALL_TROLL:
			{
				byteCount += s->Read(&effectStamp, sizeof(effectStamp) );
				ball->mEffectStamp = effectStamp;

				break;
			}
		case DSTBALL_STOP:
		case DSTBALL_FIELD:
		case DSTBALL_RIGID:
			{
				// Nothing to do here
				break;
			}
		default:
			{
				CCP_LOGERR_CH( s_chPThunk,"ReadBallFromStream: Unknown ball mode %d in dump", mode);
				return 0;
				break;
			}
		}

		// Read in miniballs if any
		//CCP_LOG_CH( s_chPThunk,"ReadBallFromStream: ball %I64d has miniballflag set to %d", id, !!(flags&DSTBALL_HASMINIBALLS));
		if(flags&DSTBALL_HASMINIBALLS)
		{
			uint16_t cnt;
			byteCount += s->Read(&cnt, sizeof(cnt));
			for(int i = 0; i < cnt; i++)
			{
				Vector3d center;
				float radius;
				byteCount += s->Read(&center.x, sizeof(center));
				byteCount += s->Read(&radius, sizeof(radius));
				
				if(partial!=1)
                {
                    //CCP_LOG_CH( s_chPThunk,"Adding miniball %d for %I64d", i, id);
					ball->AddMiniBall(center.x, center.y, center.z, radius);
                }
			}
		}
		if(flags&DSTBALL_HASMINICAPSULES)
		{
			uint16_t capsuleCnt;
			byteCount += s->Read(&capsuleCnt, sizeof(capsuleCnt));
			for(int i = 0; i < capsuleCnt; i++)
			{
				Vector3d hemisphereA;
				Vector3d hemisphereB;
				float radius;
				byteCount += s->Read(&hemisphereA, sizeof(hemisphereA));
				byteCount += s->Read(&hemisphereB, sizeof(hemisphereB));
				byteCount += s->Read(&radius, sizeof(radius));

				if(partial!=1)
				{
					ball->AddMiniCapsule(
						hemisphereA.x, hemisphereA.y, hemisphereA.z, 
						hemisphereB.x, hemisphereB.y, hemisphereB.z, 
						radius);
				}
			}
		}
		if(flags&DSTBALL_HASMINIBOXES)
		{
			uint16_t boxCnt;
			byteCount += s->Read(&boxCnt, sizeof(boxCnt));
			for(int i = 0; i < boxCnt; i++)
			{
				Vector3d corner;
				Vector3d localX;
				Vector3d localY;
				Vector3d localZ;
				byteCount += s->Read(&corner, sizeof(corner));
				byteCount += s->Read(&localX, sizeof(localX));
				byteCount += s->Read(&localY, sizeof(localY));
				byteCount += s->Read(&localZ, sizeof(localZ));

				if(partial!=1)
				{
					ball->AddMiniBox(corner, localX, localY, localZ);
				}
			}
		}
	}

	return ball;
}

void Ballpark::WriteBallToStream(Ball* b, IBlueStreamPtr s)
{
	// Don't write moribund balls to the state stream.
	// They're actually not helping you recreate any state, 
	// since they're supposed to be fake.
	if( b->isMoribund )
	{
		return;
	}

	ID id = b->mId;
	double mass = b->mMass;
	float radius = b->mRadius;
	Vector3d pos = b->mNewPos;
	uint8_t flags = (b->isFree?DSTBALL_ISFREE:0) |(b->isGlobal?DSTBALL_ISGLOBAL:0) |(b->isMassive?DSTBALL_ISMASSIVE:0)|(b->isInteractive?DSTBALL_ISINTERACTIVE:0)|(b->isSpaceJunk?DSTBALL_ISSPACEJUNK:0);
	int8_t isCloaked = b->isCloaked;
	uint16_t miniBallCnt = (unsigned short)(b->mMiniBalls.GetSize());
	uint16_t miniBoxCnt = (unsigned short)(b->mMiniBoxes.GetSize());
	uint16_t miniCapsuleCnt = (unsigned short)(b->mMiniCapsules.GetSize());
    //CCP_LOG_CH( s_chPThunk,"WriteBallToStream: ball %d has %d miniballs", id, miniBallCnt);

	if(miniBallCnt)
		flags = flags | DSTBALL_HASMINIBALLS;
	if(miniCapsuleCnt)
		flags = flags | DSTBALL_HASMINICAPSULES;
	if(miniBoxCnt)
		flags = flags | DSTBALL_HASMINIBOXES;

	uint8_t mode = b->mMode;
	float maxVel = b->mMaxVel;
	Vector3d vel = b->mNewVel;
	float agility = b->mAgility;
	float speedFraction = b->mSpeedFraction;
	Quaternion rot = b->mNewRot;
	Vector3 angVel = b->mNewAngVel;
	float rotAgility = b->mRotAgility;
	float maxAngVel = b->mMaxAngVel;

	byteCount += s->Write(&id       , sizeof(id));
	byteCount += s->Write(&mode     , sizeof(mode));
	byteCount += s->Write(&radius   , sizeof(radius));
	byteCount += s->Write(&pos.x    , sizeof(Vector3d));
	byteCount += s->Write(&flags    , sizeof(flags));

	if(mode != DSTBALL_RIGID)
	{
		byteCount += s->Write(&mass     , sizeof(mass));
		byteCount += s->Write(&isCloaked, sizeof(isCloaked));
		byteCount += s->Write(&b->mHarmonic, sizeof(b->mHarmonic));
		int32_t corporationID = int32_t( b->mCorporationID );
		byteCount += s->Write(&corporationID, sizeof(corporationID));
		int32_t allianceID = int32_t( b->mAllianceID );
		byteCount += s->Write(&allianceID, sizeof(allianceID));
	}

	if(flags & DSTBALL_ISFREE)
	{
		byteCount += s->Write(&maxVel,sizeof(maxVel));
		byteCount += s->Write(&vel.x,sizeof(Vector3d));
		byteCount += s->Write(&agility,sizeof(agility));
		byteCount += s->Write(&speedFraction,sizeof(speedFraction));
		if( g_useDynamicalOrientation )
		{
			byteCount += s->Write( &rot.x, sizeof( Quaternion ) );
			byteCount += s->Write( &maxAngVel, sizeof( maxAngVel ) );
			byteCount += s->Write( &angVel.x, sizeof( Vector3 ) );
			byteCount += s->Write( &rotAgility, sizeof( rotAgility ) );
		}
	}

	int8_t formationID = int8_t( b->mFormationID );
	byteCount += s->Write(&formationID, sizeof(formationID));
	int32_t effectStamp = int32_t( b->mEffectStamp );

	switch(mode)
	{
	case DSTBALL_FOLLOW:
		{

			byteCount += s->Write(&b->mFollowId,    sizeof(b->mFollowId)    );
			byteCount += s->Write(&b->mFollowRange, sizeof(b->mFollowRange) );

			break;
		}
	case DSTBALL_FORMATION:
		{

			byteCount += s->Write(&b->mFollowId,    sizeof(b->mFollowId)    );
			byteCount += s->Write(&b->mFollowRange, sizeof(b->mFollowRange) );
			byteCount += s->Write(&effectStamp, sizeof(effectStamp) );

			break;
		}
	case DSTBALL_MISSILE:
		{

			byteCount += s->Write(&b->mFollowId,    sizeof(b->mFollowId)    );
			byteCount += s->Write(&b->mFollowRange, sizeof(b->mFollowRange) );
			byteCount += s->Write(&b->mOwnerId,     sizeof(b->mOwnerId)     );
			byteCount += s->Write(&effectStamp, sizeof(effectStamp) );
			byteCount += s->Write(&b->mGoto.x,      sizeof(b->mGoto)        );

			break;
		}
	case DSTBALL_ORBIT:
		{

			byteCount += s->Write(&b->mFollowId,    sizeof(b->mFollowId)    );
			byteCount += s->Write(&b->mFollowRange, sizeof(b->mFollowRange) );
			
			break;
		}
	case DSTBALL_GOTO:
		{
			byteCount += s->Write(&b->mGoto.x,    sizeof(b->mGoto)      );

			break;
		}
	case DSTBALL_WARP:
		{
			byteCount += s->Write(&b->mGoto.x,      sizeof(b->mGoto)        ); // Warp destination point
			byteCount += s->Write(&effectStamp, sizeof(effectStamp) );
			byteCount += s->Write(&b->mLastCollision, sizeof(b->mLastCollision) ); // Total length of warp
			byteCount += s->Write(&b->mFollowId,    sizeof(b->mFollowId)    ); // Minimum range (as double)
			byteCount += s->Write(&b->mOwnerId,     sizeof(b->mOwnerId)     ); // Warp factor (speed multiplier)

			break;
		}
	case DSTBALL_MUSHROOM:
		{
			double span = b->mGoto.x;

			byteCount += s->Write(&b->mFollowRange, sizeof(b->mFollowRange) );
			byteCount += s->Write(&span           , sizeof(span)               );
			byteCount += s->Write(&effectStamp, sizeof(effectStamp) );
			byteCount += s->Write(&b->mOwnerId,     sizeof(b->mOwnerId)     );

			break;
		}
	case DSTBALL_TROLL:
		{
			byteCount += s->Write(&effectStamp, sizeof(effectStamp) );

			break;
		}
	case DSTBALL_STOP:
	case DSTBALL_FIELD:
	case DSTBALL_RIGID:
		{
			// Nohting to do here

			break;
		}
	default:
		{
			CCP_LOGERR_CH( s_chPThunk,"WriteBallToStream: Unknown ball mode %d in dump", mode);
			return;
			break;
		}
	}


	// Write out any miniballs
	if(miniBallCnt)
		byteCount += s->Write(&miniBallCnt, sizeof(miniBallCnt));

	for(int i = 0; i < miniBallCnt; i++)
	{
		MiniBall *mb = (MiniBall *)(void *)(b->mMiniBalls.GetAt(i));
		byteCount += s->Write(&mb->mPos.x, sizeof(mb->mPos));
		byteCount += s->Write(&mb->mRadius, sizeof(mb->mRadius));
	}

	// Minicapsules
	if(miniCapsuleCnt)
		byteCount += s->Write(&miniCapsuleCnt, sizeof(miniCapsuleCnt));

	for(int i =0; i < miniCapsuleCnt; i++)
	{
		MiniCapsule *mc = (MiniCapsule *)(void *)(b->mMiniCapsules.GetAt(i));
		byteCount += s->Write(&mc->mHemisphereA, sizeof(mc->mHemisphereA));
		byteCount += s->Write(&mc->mHemisphereB, sizeof(mc->mHemisphereB));
		byteCount += s->Write(&mc->mRadius, sizeof(mc->mRadius));
	}

	// Miniboxes
	if(miniBoxCnt)
		byteCount += s->Write(&miniBoxCnt, sizeof(miniBoxCnt));

	for(int i = 0; i < miniBoxCnt; i++)
	{
		MiniBox *mc = (MiniBox *)(void *)(b->mMiniBoxes.GetAt(i));
		byteCount += s->Write(&mc->mCorner, sizeof(mc->mCorner));
		byteCount += s->Write(&mc->mLocalX, sizeof(mc->mLocalX));
		byteCount += s->Write(&mc->mLocalY, sizeof(mc->mLocalY));
		byteCount += s->Write(&mc->mLocalZ, sizeof(mc->mLocalZ));
	}
}

#ifdef DESTINY_EMBEDDED
size_t Ballpark::DestinyEmbeddedWriteFullState( IBlueStreamPtr s )
{
	int32_t timestamp = int32_t( mCurrentTime );
	char packet = DESTINY_FULLSTATE;
	s->Seek( 0, ICcpStream::SO_BEGIN );
	byteCount = s->Write( &packet, sizeof( packet ) );
	byteCount += s->Write( &timestamp, sizeof( timestamp ) );
	BallIterator it( &mBalls );
	Ball* b;
	while( ( b = it++ ) )
	{
		if( b->mId < DSTLOCALBALLS )
			continue;
		WriteBallToStream( b, s );
	}
	s->Seek( 0, ICcpStream::SO_BEGIN );
	return byteCount;
}
#endif // DESTINY_EMBEDDED

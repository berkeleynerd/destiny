// Copyright © 2000 CCP ehf.

#ifndef _DYNAMIC_H_
#define _DYNAMIC_H_

#include "IEveBallpark.h"
#include "IDstConstants.h"

struct Vector3d;

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iterator>

class Ball;
class ClientBall;
class Partition;
class Box;
class OrientedBox;

TYPEDEF_BLUECLASS(Ball);
TYPEDEF_BLUECLASS(ClientBall);

#include "MapOfBalls.h"
#include "Capsule.h"
#include "CollisionBallProperties.h"
#include "CollisionStructures.h"
#include "OrientedBox.h"

const double AU = .1495978707e12;

/*------------------------------------------------------------------------
   The ballpark is an instance of a space partitioning, along with some
   arbitrary number of balls in various dynamical states. The ballpark
   allows the dynamics of all balls to be calculated, and updates the
   space partitioning accordingly.

   The ballpark defines 3 essential parameters:

   n: is the number of boxes in each dimension. The total number of boxes
      is n*n*n. The boundary boxes actually contain any ball that goes
	  beyond bounds	, so that the whole space is covered. This number should
	  at least be 1, and must be an odd number, such that a box can be
	  centered around the origin.

   width: This is the side length of boxes.

   dt: This is the time step used in time evolution of the Balls.

   These 3 parameters need to be chosen such that a box can contain the
   largest ball in the park, and that its movement during one time step
   does not take it farther than one box.

   Various methods are given to add, remove and modify state of balls.
   Each time one of these methods is called, this defines a new initial
   condition for the whole dynamical system.
------------------------------------------------------------------------*/
typedef std::unordered_map<ID, Ball *> DictOfBalls;
typedef std::map<ID, Ball *> DictOfFreeBalls;
typedef std::pair<ID, Ball*> DictEntry;
typedef std::unordered_set<Ball *> SetOfBalls;
typedef std::vector<Ball *> VectorOfBalls;
typedef std::vector<Capsule *> VectorOfCapsules;
typedef std::back_insert_iterator< VectorOfBalls > BallVectorInsertor;
typedef std::vector<Box *> VectorOfBoxes;
typedef std::back_insert_iterator< VectorOfBoxes > BoxVectorInsertor;
typedef std::vector<Vector3d> VectorOfVectors;
typedef std::unordered_set<Capsule *> SetOfCapsules;
typedef std::unordered_set<OrientedBox *> SetOfOrientedBoxes;

class Ballpark :
	public IEveBallpark,
	public IBlueEvents
{

friend class Ball;
friend class ClientBall;
friend class Capsule;
friend class CollisionBallProperties;
friend class OrientedBox;
public://Exposed

	static PyObject* s_ballNotInParkCallback;
	Be::Time mTime; // Last known timestamp
	int mTickInterval; // Time step of simulation

	long mCurrentTime; // Current simulation counter
	double mLagDamping;
	ID mEgo;
	ID mProbe;
	ID mLocalCnt;
	ID mLocalHiCnt;
	double mFriction; // Friction coefficient of space
	double mWarpSpeed;
	ID mSolarsystemID;

	float mSomeWeirdHackToFixSomething;

	double mPara1;
	double mPara2;
	int mInt1;
	int mInt2;

	int mMoribundBallRemovalCount; // The number of balls that can be removed per tick before the absolute lifetime is up.
	int mMoribundBallRemovalBuffer; // The number of seconds before absolute lifetime that we will start removing balls.

	double mRollSpeedAcceleration;
	double mRollSpeedDecay;
	double mRollAcceleration;
	double mRollDecay;

private://MEMBERS

	double dt; // time interval of simulation
	ID mLastEgo;
	ClientBall *mLastEgoBall;

	MapOfBalls mBalls; // Internal list of balls
	MapOfBalls mGlobals; // of which the following are global balls
	std::unordered_map<ID, StaticCollidable*> mStaticCollidables;
	std::unordered_map<ID, Capsule*> mCapsules;
	std::unordered_map<ID, OrientedBox*> mOrientedBoxes;
	SetOfBalls mBubbleConflicts;
	PyObject *bubbleInteractives;
    PyObject *bubbleKeepAlives; // Per bubble, a set of ballIDs which keep the bubble alive even when no interatives are present
	PyObject *bubbleKeepAliveBalls; // balls that automatically keep their current bubbles alive
	PyObject *bubbles;
	PyObject *mBubbleSubscriptions;

	SetOfBalls moribundBalls;
	SetOfCapsules moribundCapsules;
	SetOfOrientedBoxes moribundOrientedBoxes;

	bool mHaveTicks;
	bool mFirstTime;
	bool isMaster;
	bool inEvolve;

	Partition *mPartition;
	Vector3d mLastReference;
	Vector3d mOldReference;
	Vector3d mLastDelta;
	Be::Time mLastReferenceTime;
	DictOfFreeBalls mFreeBalls;
	DictOfBalls mProximityBalls;

	std::vector<VectorOfVectors> mFormations;

	long mBubbleId;

	Ball* AddBall(
		const ID& objectId,
		double mass,
		float radius,
		float maxVel,
		bool isFree,
		bool isGlobal,
		bool isMassive,
		bool isInteractive,
		bool isSpaceJunk,
		double x,
		double y,
		double z,
		double vx,
		double vy,
		double vz,
		float agility,
		float speedFraction );

	void ClearBubbles(); // Release the bubble dict, if it exists

public://FUNCTIONS
	EXPOSE_TO_BLUE();

	Ballpark(IRoot* lockobj = NULL);
	~Ballpark();


	/////////////////////////////////////////////////////////////////////////////////////
	// Ballpark
	/////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////
	// AddBall is the basic method to add a Ball to the Ballpark.
	//
	// post: A ball with the given parameters has been created and inserted into
	//       the ballpark. A new initial condition has been defined for the whole
	//       ballpark. Depending on whether the ballpark is a master or slave,
	//       the following is true:
	//
	//////////////////////////////////////////////////////////////////////////////

	Ball* AddBall(
		Ball* ball );

	Ball* AddOldStyleOrientedBall(
		const ID& objectId,
		double mass,
		float radius,
		float maxVel,
		bool isFree,
		bool isGlobal,
		bool isMassive,
		bool isInteractive,
		bool isSpaceJunk,
		double x,
		double y,
		double z,
		double vx,
		double vy,
		double vz,
		float agility,
		float speedFraction );

	Ball* AddDynamicallyOrientedBall(
		const ID& objectId,
		double mass,
		float radius,
		float maxVel,
		float maxAngVel,
		bool isFree,
		bool isGlobal,
		bool isMassive,
		bool isInteractive,
		bool isSpaceJunk,
		double x,
		double y,
		double z,
		double vx,
		double vy,
		double vz,
		float rx,
		float ry,
		float rz,
		float rw,
		float wx,
		float wy,
		float wz,
		float agility,
		float rotAgility,
		float speedFraction);

	Ball * AddMiniball(
		const ID& parentObjectId,
		double mass,
		float radius,
		double x,
		double y,
		double z
		);

	Capsule * AddCapsule(
		const ID& objectId,
		const ID& parentObjectId,
		double ax,
		double ay,
		double az,
		double bx,
		double by,
		double bz,
		float radius
		);

	OrientedBox * AddOrientedBox(
		const ID& objectId,
		const ID& parentObjectId,
		double corner_0,
		double corner_1,
		double corner_2,
		double x0,
		double x1,
		double x2,
		double y0,
		double y1,
		double y2,
		double z0,
		double z1,
		double z2
		);


	//////////////////////////////////////////////////////////////////////////////
	// FollowBall makes 'srcId' follow 'dstId' while
	// attempting to maintain 'range' distance between the surfaces of the balls
	// A ball can only follow one other ball at a time. Calling
	// this function when already following another object
	// will stop following the old and start following the new.
	//
	// pre:  'srcId' != 'dstId' and both exist. 'range' >= 0.0.
	// post: Ball 'srcId' will now attemp to follow 'dstId'. Any other ball
	//       mode has been cancelled.
	//
	//////////////////////////////////////////////////////////////////////////////
	void FollowBall(
		const ID& srcId,
		const ID& dstId,
		float range
		);

	//////////////////////////////////////////////////////////////////////////////
	// MissileFollow makes 'srcId' follow 'dstId' ignoring
	// collisions from the destination as well as 'ownerId'
	// A ball can only follow one other ball at a time. Calling
	// this function when already following another object
	// will stop following the old and start following the new.
	//
	//////////////////////////////////////////////////////////////////////////////
	void MissileFollow(
		const ID& srcId,
		const ID& dstId,
		const ID& ownerId
		);

	void FormationFollow(
		const ID& srcId,
		const ID& dstId,
		char slot
	);

	//////////////////////////////////////////////////////////////////////////////
	// WarpTo makes 'srcId' go very fast to a destination point (x,y,z)
	//
	// pre:  'srcId' exists.
	// post: Ball 'srcId' is now in the state of going very fast to the point (x,y,z)
	//       Once it is within range of the point, the ship will come to a stop.
	//       If the point was within  range to start with, a goto command
	//       is issued instead.
	//////////////////////////////////////////////////////////////////////////////
	void WarpTo(
		const ID& srcId,
		double x,
		double y,
		double z,
		double minRange,
		int warpFactor
		);

	void RealWarp(Ball *ball);
	//////////////////////////////////////////////////////////////////////////////
	// GotoPoint makes 'srcId' go to the specified point
	// There is no guarantee that it will actually make it
	// nor that it will choose the shortest way, as it must
	// avoid any obstacles in its path, but it will certainly
	// make an honest attempt.
	//
	// pre:  'srcId' exists. 'range' >= 1.0.
	// post: Ball 'srcId' will now attempt to reach the given point. Any other
	//       ball mode has been cancelled.
	//
	//////////////////////////////////////////////////////////////////////////////
	void GotoPoint(
		const ID& srcId,
		double x,
		double y,
		double z
		);

	void GotoPoint(
		Ball *ball,
		const Vector3d& p
		);

	//////////////////////////////////////////////////////////////////////////////
	// GotoDirection makes 'srcId' go to the specified direction
	//
	// The direction is specified as a unit vector pointing to the direction
	// needed
	//
	// pre:  'srcId' exists. sqrt(x^2+y^2+z^2) = 1
	// post: Ball 'srcId' will now attempt to reach the a point far away that is
	//       in the direction specified. Any other
	//       ball mode has been cancelled.
	//////////////////////////////////////////////////////////////////////////////
	void GotoDirection(
		const ID& srcId,
		double x,
		double y,
		double z
		);

	void GotoDirection(
		Ball *ball,
		Vector3d dir
		);

	//////////////////////////////////////////////////////////////////////////////
	// Orbit makes 'srcId' stumble around 'dstId' at at specified range indefinitely
	//
	// The orbiting is done such that once initiated, the ship will continously
	// choose a new point to follow somewhere on a sphere surrounding the target
	// ship. The range is specified as distance between the surfaces of the spheres
	// radii.
	//
	// pre:  'srcId' and 'dstId' exist. range >= 0.
	// post: Ball 'srcId' will now randomly choose points to goto on a sphere of
	//       radius range+(r1+r2) centered at ball 'dstId'. Any other ball mode
	//       has been cancelled.
	//////////////////////////////////////////////////////////////////////////////
	void Orbit(
		const ID& srcId,
		const ID& dstId,
		float range
		);

	//////////////////////////////////////////////////////////////////////////////
	// Stop makes 'srcId' stop any current ball mode
	// This will eventually bring it to a stop, except for bumping by other Balls.
	//
	// pre:  'srcId' exists.
	// post: All ball modes have been cancelled.
	//
	//////////////////////////////////////////////////////////////////////////////
	void Stop(
		const ID& srcId
		);

	void Stop(
		Ball *ball
		);

	//////////////////////////////////////////////////////////////////////////////
	// RemoveBall removes 'srcId' from the Ballpark.
	// Any Balls following it will be stopped.
	//
	// pre:  'srcId' exists.
	// post: The ball has been removed from the Ballpark. Any ball following this
	//       ball have been put into Stop mode.
	//
	//////////////////////////////////////////////////////////////////////////////

	void RemoveBall(
		const ID& srcId,
		int delay =0
		);

	//////////////////////////////////////////////////////////////////////////////
	// RemoveCapsule removes 'srcId' from the Ballpark.
	//
	// pre:  'srcId' exists and is a capsule.
	// post: The capsule has been removed from the Ballpark.
	//
	//////////////////////////////////////////////////////////////////////////////

	void RemoveCapsule(
		const ID& srcId,
		int delay =0
		);

	//////////////////////////////////////////////////////////////////////////////
	// RemoveOrientedBox removes 'srcId' from the Ballpark.
	//
	// pre:  'srcId' exists and is an OBB.
	// post: The OBB has been removed from the Ballpark.
	//
	//////////////////////////////////////////////////////////////////////////////

	void RemoveOrientedBox(
		const ID& srcId,
		int delay =0
		);


	//////////////////////////////////////////////////////////////////////////////
	// SetBallMass sets the mass of ball 'srcId' from the Ballpark.
	//
	// pre:  'srcId' exists and mass > 0
	// post: The ball's mass has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallMass(
		const ID& srcId,
		double mass
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetBallRadius sets the radius of ball 'srcId' from the Ballpark.
	//
	// pre:  'srcId' exists and radius > 0.
	// post: The ball's radius has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallRadius(
		const ID& srcId,
		float radius
		);

	void SetBallRadius(
		Ball *ball,
		float radius
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetMaxSpeed sets the maximum velocity of ball 'srcId' from the Ballpark.
	//
	// pre:  'srcId' exists and maxVel >= 0.
	// post: The ball's maximum velocity has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetMaxSpeed(
		const ID& srcId,
		float maxVel
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetMaxAngularSpeed sets the maximum angular speed of ball 'srcId' from the Ballpark.
	//
	// pre:  'srcId' exists and maxAngVel >= 0.
	// post: The ball's maximum angular speed has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetMaxAngularSpeed(
		const ID& srcId,
		float maxAngVel );

	//////////////////////////////////////////////////////////////////////////////
	// SetBallPosition sets the position of ball 'srcId' from the Ballpark.
	//
	// pre:  'srcId' exists
	// post: The ball's position has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallPosition(
		const ID& srcId,
		double x,
		double y,
		double z
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetBallVelocity sets the velocity of ball 'srcId' from the Ballpark.
	//
	// pre:  'srcId' exists
	// post: The ball's velocity has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallVelocity(
		const ID& srcId,
		double vx,
		double vy,
		double vz
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetBallAngularVelocity sets the angular velocity of ball 'srcId' from the Ballpark.
	//
	// pre:  'srcId' exists
	// post: The ball's angular velocity has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallAngularVelocity(
		const ID& srcId,
		double wx,
		double wy,
		double wz );

	//////////////////////////////////////////////////////////////////////////////
	// SetBallRotation sets the rotation quaternion of ball 'srcId' from the Ballpark.
	//
	// pre:  'srcId' exists
	// post: The ball's rotation quaternion has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallRotation(
		const ID& srcId,
		double rx,
		double ry,
		double rz,
		double rw );

	//////////////////////////////////////////////////////////////////////////////
	// SetBallFree sets the position 'isFree' status
	//
	// pre:  'srcId' exists
	// post: The ball's 'isFree' status has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallFree(
		const ID& srcId,
		bool flag
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetBallMassive sets the position 'isMassive' status
	//
	// pre:  'srcId' exists
	// post: The ball's 'isMassive' status has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallMassive(
		const ID& srcId,
		bool flag
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetBallInteractive sets the balls 'isInteractive' status
	//
	// pre:  'srcId' exists
	// post: The ball's 'isInteractive' status has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallInteractive(
		const ID& srcId,
		bool flag
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetBallGlobal sets the position 'isGlobal' status
	//
	// pre:  'srcId' exists
	// post: The ball's 'isGlobal' status has been updated.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallGlobal(
		const ID& srcId,
		bool flag
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetBallAgility sets the agility modifier for this ball
	//
	// pre:  'srcId' exists
	// post: The ball's agility modifier has been set
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallAgility(
		const ID& srcId,
		float agility
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetBallAngularAgility sets the rotational agility modifier for this ball
	//
	// pre:  'srcId' exists
	// post: The ball's agility modifier has been set
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallAngularAgility(
		const ID& srcId,
		float rotAgility );

	//////////////////////////////////////////////////////////////////////////////
	// AddMushroom add an expanding ball, pushing all other ball except owner
	//
	// pre:  'ownerId' exists, 'range' > 0.0, time > 0
	// post: The ball will start expanding from radius 0 to range during time*seconds
	//       without affecting 'ownerId'
	//
	//////////////////////////////////////////////////////////////////////////////

	void AddMushroom(
		const ID& srcId,
		float range,
		double time
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetSpeedFraction sets the speed fraction for this ball
	//
	// pre:  'srcId' exists, 'fraction' is [0,1]
	// post: The ball's speed fraction has been set
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetSpeedFraction(
		const ID& srcId,
		float fraction
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetBallTroll puts a ball in troll mode
	//
	// pre:  'srcId' exists, 'delay' > 0
	// post: The ball has been made interactive and free, but in the mode DSTBALL_TROLL.
	//		 It will be made fixed and non-interactive after 'delay' time steps and put
	//       in mode DSTBALL_STOP.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallTroll(
		const ID& srcId,
		int delay
		);

	void SetBallRigid(
	const ID& srcId
	);

	//////////////////////////////////////////////////////////////////////////////
	// SetBallHarmonic sets the harmonic value of a ball
	//
	// pre:  'srcId' exists,
	// post: The ball has the harmonic value specified. If 'field' was true
	//       that ball is now in the state DSTBALL_FIELD, else the mode is unchanged, except if it was in the mode DSTBALL_FIELD in which
	//       case it is now in the mode DSTBALL_STOP.
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetBallHarmonic(
		const ID& srcId,
		int64_t harmonic,
		int corporationID,
		int allianceID,
		bool field
		);

	void SetBallFormation(
		const ID& srcId,
		char formationID
		);

	//////////////////////////////////////////////////////////////////////////////
	// SetNotificationRange sets the notification range used for this ball in
	// various follow modes
	//
	// pre:  'srcId' exists, range is >= 0.0
	// post: The ball's notification range has been set
	//
	//////////////////////////////////////////////////////////////////////////////
	void SetNotificationRange(
		const ID& srcId,
		double notificationRange
		);

    //////////////////////////////////////////////////////////////////////////////
    // SetTargetTracking This will set the target tracking range.
    // a range of 0 or a targetID of 0 will disable the tracking.
    //
    // pre:  'srcID' targetID  exists, range is >= 0.0
    // post: The ball's notification range has been set
    //
    // Will cause a callback to DoTargetTracking when the range is crossed.
    //////////////////////////////////////////////////////////////////////////////
    void SetTargetTracking(const ID& srcID, const ID& targetID, double trackingRange);

	//////////////////////////////////////////////////////////////////////////////
	// AddProximitySensor adds a proximity sensor to the given ball
	//
	// pre:  'srcId' exists, 'range' is positive
	// post: Proximity events will be generated.
	//
	//////////////////////////////////////////////////////////////////////////////

	void AddProximitySensor(
		const ID& srcId,
		float range,
		double period,
		int shuffle,
		bool onlyInteractives
		);

	//////////////////////////////////////////////////////////////////////////////
	// RemoveProximitySensor removes an existing sensor
	//
	// pre:  'id' exists
	// post: Proximity sensor for given ball has been deactivated
	//
	//////////////////////////////////////////////////////////////////////////////

	void RemoveProximitySensor(
		const ID& srcId
		);

	//////////////////////////////////////////////////////////////////////////////
	// CheckVisibility checks for visibility between two balls in the same 'bubble'
	//
	// pre:  'srcId' and 'dstId' exist
	// post: returns the id of one of possibly many occluding ball if the two
	//       balls are occluded, otherwise -1 if the balls are visible and -2
	//       if there was some error.
	//
	//////////////////////////////////////////////////////////////////////////////

	ID CheckVisibility(
		const ID& srcId,
		const ID& dstId
		);

	//////////////////////////////////////////////////////////////////////////////
	// GetAccuracy return accuracy factor between two balls with given accuracy
	// and weapon settings
	//
	//////////////////////////////////////////////////////////////////////////////

	double GetAccuracy(
		const ID& srcId,
		const ID& dstId,
		double& optimalRange, // note input/output parameter
		double falloff,
		double& trackingSpeed, // note input/output parameter
		double signatureRadius,
		double optimalSignatureRadius
		);

	//////////////////////////////////////////////////////////////////////////////
	// CloakBall turns on cloak mode for this ball
	//
	// pre:  'srcId'  exist and 'cloakMode' is larger than zero
	// post: ball will now be in cloaked mode until explicitly uncloaked.
	//
	//////////////////////////////////////////////////////////////////////////////

	void CloakBall(
		const ID& srcId,
		char cloakMode,
		float range
		);
	void UncloakBall(
		const ID& srcId
		);

	//////////////////////////////////////////////////////////////////////////////
	// Evolve advances the dynamical simulation by one
	// time step 'dt', as well as incrementing a time
	// counter 'theTime'.
	//
	// pre:  none.
	// post: All balls have been evolved according to their current dynamical mode.
	//
	//       MASTER: All udpate events have been distributed to slaves.
	//
	//////////////////////////////////////////////////////////////////////////////
	void Evolve(Be::Time timestamp);

    // Does some setup for evolution on a ball, then calls into one of the mode evolve function (below)
    void EvolveBehaviorForBall(Ball* ball);

    // Evolve functions for ball modes
    Vector3d EvolveWarp(Ball* ball);
    Vector3d EvolveGoto(Ball* ball);
    Vector3d EvolveMissile(Ball* ball);
    Vector3d EvolveFormation(Ball* ball);
    Vector3d EvolveFollow(Ball* ball);
    Vector3d EvolveOrbit(Ball* ball, long currentTime);
	Vector3d EvolveOldStyleOrbit( Ball* ball, long currentTime );
	Vector3d EvolveNewStyleOrbit( Ball* ball, long currentTime );
    void EvolveStop(Ball* ball);    // This one operates directly on the velocity, no need for return value

	//////////////////////////////////////////////////////////////////////////////
	// InitializeBubbles recalculates the bubble distribution of balls
	//
	// pre:  none.
	// post: All balls now have an updated bubble ID.
	//
	//
	//////////////////////////////////////////////////////////////////////////////

	void InitializeBubbles();
	void UpdateBallBubble(const ID& srcID);
	size_t GetInteractiveCnt(Ball *ball);

	void AddTransitionToList(const Ball *ball, PyObject *transitions);
	void NotifyOfBubbleTransitions(const PyObject* transitions);

	void Pause();
	void Start();
	bool InDeadBubble(Ball *ball);
	void ClearAll();

	void AddFormation(PyObject *);

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueEvents
	/////////////////////////////////////////////////////////////////////////////////////
	void OnTick(
		Be::Time realTime,
		Be::Time simTime,
		void* cookie
		) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveBallpark
	/////////////////////////////////////////////////////////////////////////////////////
	void Delta(
		Vector3* ref,
		Vector3* sref,
		Be::Time time
		) override;

	void DeltaVel(
		Vector3* vel,
		Be::Time time
		) override;

	float GetUnitBase(
		) override {return mSomeWeirdHackToFixSomething;};

	void SetUnitBase(
		float unit
		) override {mSomeWeirdHackToFixSomething = unit;};

	Vector3d* GetReferencePoint(Vector3d* out, Be::Time time) override;

    void EntityWarpIn(const ID& srcId, double x, double y, double z, int warpFactor);

    void AdjustTimes(Be::Time timeDelta);

private:
	void InsertInBox(Box *box,Partitionable *partitionable);
	void InsertInBoxes(Partitionable *Partitionable);
	void InsertBallInBoxes(Ball *ball);
	void InsertStaticCollidableInBoxes(StaticCollidable* collidable);
	void IncreaseInteractiveCnt(Partitionable *p, long inBubble);
	void DecreaseInteractiveCnt(Partitionable *p, long inBubble);
	void UpdateInteractiveCnt(Partitionable *p, long oldBubble, long newBubble);
	void AddKeepAliveBall(PyObject *ballId, long inBubble);
	void RemoveKeepAliveBall(PyObject *ballId, long inBubble);
	void UpdateKeepAliveBalls(Partitionable *p, long oldBubble, long newBubble);
	bool TrollReady(Ball *ball);
	void PetrifyTroll(Ball *ball);
	void CapAcceleration(const Ball *ball, Vector3d& a);
	void SetBallTimeFactor(Ball *ball);
	ID GetIDForCollisionObject(ID objectId);

	void CopyBubbles();
	void ResolveBubbleConflicts();
	void AddToBubble(Partitionable *partitionable);
	void StopAllFollowers(Ball *ball);
	void DeleteBallFromBoxes(Ball *b);
	double GetMaxRadius(double);
	void HandleProximities();
	void BringOutTheDeads(); // Clean up moribund collidables
	void BringOutDeadBalls(); // Clean up moribund balls
	void BringOutDeadCapsules();

	ClientBall *GetEgo(); // Gets the latest ego..possibly cached
	double WarpDistance(Ball *b,Vector3d& p, Vector3d& v,double t,bool interpolating=false);
    void SetupWarpConstants(
            double warpFactor,
            double warpDistance,
            double &accelDuration,
            double &cruiseDuration,
            double &decelDuration,
            double &accelDistance,
            double &cruiseDistance,
            double &decelDistance,
            double &accelRate,
            double &decelRate,
            double &warpSpeed
            );
	bool BallIsValidForIteration(ID ballID, Ball*& ball, const Ball* refBall);
	int GetNextValidBallInBubble(PyObject* theBubble, Py_ssize_t& pos, PyObject*& key, PyObject*& value, Ball*& otherBall, const Ball* refBall);
	PyObject* GetActiveBubbleForBall(const Ball* ball);
	PyObject* GetBallIdsInRange(PyObject* args, bool includeDist);
	PyObject* GetBallIdsInRangeOfTriangle(PyObject* args);
	PyObject* GetBallIdsInCone(PyObject* args);
	PyObject* GetBallIdsInCapsule(PyObject* args);

	void Gradient(Ball* b);
	void Potential(Ball *me, Ball *other, int recurionsDepth = 0);
	void CalculateIterativeCollisionResponses();

	// Store the information in a map for fast lookup
	std::unordered_map<ID, CollisionBallProperties> mCollisionLocations;

	// Cache the potential colliders as these don't change during the iterations
	std::unordered_map<ID, VectorOfBalls> mPotentialCollisionBalls;
	std::unordered_map<ID, VectorOfStaticCollidables> mPotentialCollidables;

	// -------------------------------------------------------------
	// Description:
	//     Returns in collisionItem the collision that occur with
	//     static collideables in the next step.
	// Arguments:
	//     ballProp - The ball properties, will be updated if needed
	//     collisionItem - Contains, on output, the collision with static objects in the park
	//     balls - The ball collision candidates
	//     collidables - The collidable collision candidates
	// -------------------------------------------------------------
	void GetNextCollisionStatic(CollisionBallProperties& ballProp, CollisionItem& collisionItem, VectorOfBalls& balls, VectorOfStaticCollidables& collidables);

	// -------------------------------------------------------------
	// Description:
	//     Calculates the collision and their responses with free balls.
	//     Should be called after GetNextCollisionStatic with same collisionItem.
	//     Separated, because it requires a more complicated logic.
	// Arguments:
	//     ballProp - The ball propertis, will be updated if needed
	//     collisionItem - in: output from static collisions, out: the collision with free balls checked as well
	//     balls - The ball collision candidates
	// -------------------------------------------------------------
	void GetNextCollisionFree(CollisionBallProperties& ballProp, CollisionItem& collisionItem, VectorOfBalls& balls, size_t collision_iteration);

	// -------------------------------------------------------------
	// Description:
	//     Using elliptical approximation, calculates the vector from
	//     the CM to the point of impact for ball.
	//     Uses the miniball position if given in the collision.
	//     NOTE: ballLoc has to be fully calculated before entry
	// Arguments:
	//     ballLoc - The location of the ball at time of collision
	//     collision - The properties of the collision
	// Return Value:
	//     The point of impact
	// -------------------------------------------------------------
	Vector3d ApproximateEllipsePointOfImpact(Ball* ball, BallLocation& ballLoc, const CollisionItem& collision);

	// -------------------------------------------------------------
	// Description:
	//     Calculates the actual impact on the ball from the collision.
	//     Only works for static colliders.
	//     Uses https://en.wikipedia.org/wiki/Collision_response
	// Arguments:
	//     ballProp - The pre-calculated properties of the ball
	//     collision - The collision in question, on return the impact is correct
	// -------------------------------------------------------------
	void CalculateCollisionImpactStatic(CollisionBallProperties& ballProp, CollisionItem& collision);

	// -------------------------------------------------------------
	// Description:
	//     Calculates the actual impact on the ball from the collision.
	//     Only works for free ball collisions.
	//     Uses https://en.wikipedia.org/wiki/Collision_response
	// Arguments:
	//     ballProp - The pre-calculated properties of the first ball
	//     otherProp - The pre-calculated properties of the other ball
	//     collision - Information on the collision. On return the impact is for the first ball
	// Return Value:
	//      The collision item for the other ball, with calculated impact
	// -------------------------------------------------------------
	CollisionItem CalculateCollisionImpactFree(CollisionBallProperties& ballProp, CollisionBallProperties& otherProp, CollisionItem& collision);

	// -------------------------------------------------------------
	// Description:
	//     Takes the current impact and adds it to the ball.
	//     Removes any impacts that have a later timeOfImpact
	//     and makes sure ones with a free ball are properly
	//     taken care of.
	// Arguments:
	//     impact - The impact to add
	//     collIter - The current collision iteration
	//     ball - The ball to add the impact to
	// -------------------------------------------------------------
	void AddImpact(CollisionImpact& impact, size_t collIter, Ball* ball);

	// -------------------------------------------------------------
	// Description:
	//     Removes impact from ball with the given collider and time of impact.
	//     Does nothing in rare cases with simultaneous collisions with two balls.
	//     Goes recursively through all possible free-free removals.
	// Arguments:
	//     time - The time of impact
	//     collider - The ID of the collider
	//     ball - The ball to remove the impact from.</param>
	// -------------------------------------------------------------
	void RemoveImpactFromFree(double time, ID collider, Ball* ball);

	// -------------------------------------------------------------
	// Description:
	//     Define this in one place only to avoid mistakes when changing things.
	// Arguments:
	//     ball - The ball colliding
	//     neighbor - The ball it collides with
	// Return Value:
	//     True if no collision should happen
	// -------------------------------------------------------------
	static inline bool CollisionBallNeighborIgnore(Ball* ball, Ball* neighbor);

	// -------------------------------------------------------------
	// Description:
	//     Calculates the collision responses for ball in current iteration.
	//     Updates the mCollisionLocations structure.
	// Arguments:
	//     ball - The ball to calculate responses for
	//     collisionIteration - Counter for the iterations
	// Return Value:
	//     True if a collision was added to the ball
	// -------------------------------------------------------------
	bool CalculateCollisionResponse(Ball* ball, size_t collisionIteration);

	// -------------------------------------------------------------
	// Description:
	//     Calculates the velocity and position of the ball after a
	//     time dt has passed from current time.  Assumes the
	//     acceleration for the current timestep has already been
	//     calculated and takes the current collision responses into account.
	// Arguments:
	//     ball - The ball for which to calculate
	//     t - The time passed since initial position (Generally between 0 and park::dt, but is not checked for validity)
	//     position - The position, should be the initial value at t=0 on input, final value on output
	//     velocity - The velocity, should be the initial value at t=0 on input, final value on output
	// -------------------------------------------------------------
	void CalculateBallPositionVelocity(Ball* ball, double t, Vector3d& position, Vector3d& velocity);

	// -------------------------------------------------------------
	// Description:
	//     Returns the unit vector normal to the orbital plane, if the movemode of the ball is ORBIT.
	//     Returns the 0 vector in case of an error.
	// Arguments:
	//     ball - A pointer to the ball that is orbiting
	//     toVector - On output it is the unit vector pointing to the center of orbit
	//     dist - On output is the distance to the center of orbit
	// Return Value:
	//     The unit orbital vector normal to the orbit
	Vector3d GetOrbitalNormal( Ball* ball, Vector3d& toVector, double& dist );
	Vector3d DefaultOrbitalNormal( Ball* ball );

	// Actual infinitesimal integration
	void Integrate(Vector3d& p, Vector3d& v, const Vector3d& a, double mass, double k, double timefactor, double step);

	// The thrust required to go to the point, letting friction stop you.
	// If missile is true, simply accelerate in the direction of the point.
	Vector3d GotoThrust(const Ball *b,const Vector3d& target,bool missile = false);

	// Calculate required thrust for orbiting balls.
	Vector3d OrbitThrust( const Ball* b, const Vector3d& target );

	// The thrust required to go to a point, ignoring any velocity requirements
	// This is suitable for follow like operations
	// Adjusts for required targetVelocity, weighting the two accelerations by
	// weight
	Vector3d GotoThrustFollow( const Ball* b, const Vector3d& target, const Vector3d& targetVelocity, double weight );

	//Calculate the torque required to rotate towards direction.
	Vector3 GotoTorque( const Ball* b, const Vector3& direction );
	//Calculate the torque required to rotate towards direction.
	//This version puts on the breaks after accelerating
	Vector3 GotoTorqueBreaks( const Ball* b, const Vector3& direction );
	//Calculate the roll of the ship.  This is independent of the torque required for heading
	//Banks the ship into turns, or tries to stay "upright"
	float GotoRoll( const Ball* b, const Vector3& heading_torque );
	float GotoRollBreaks( const Ball* b, const Vector3& heading_torque );
	//Apply the rotation.  The integration is available in fractional steps for interpolation purposes only
	//The integration is approximate and integrationg from 0 to 0.5 and then from 0.5 to 1.0 is not guaranteed
	//to give same results as integrating from 0 to 1.0
	void ApplyTorque( Quaternion& rotation, Vector3& angvel, const Vector3& torque, const float roll, const float tau, const float fraction );

	// -------------------------------------------------------------
	// Description:
	//     Calculate the rotation at time t, taking collision responses into account.
	// Arguments:
	//     ball - The ball for which to calculate
	//     t - The time passed since initial position.  Generally between 0 and park::dt, but is not checked for validity
	//     rotation - The rotation, should be the initial value at t=0 on input, final value on output
	//     omega - The angular velocity, should be the initial value at t=0 on input, final value on output
	// -------------------------------------------------------------
	void CalculateBallRotationVelocity( Ball* ball, double t, Quaternion& rotation, Vector3& omega );


	bool ReadFullStateFromStream(IBlueStreamPtr, int);
	Ball* ReadBallFromStream(IBlueStreamPtr, int);
	void WriteBallToStream(Ball*, IBlueStreamPtr);

	int GetIntersection(Vector3d &origin, Vector3d &ray, Vector3d &sphere, double radius, double *first=0, double *second=0);
	void ShiftFromPlanets(Vector3d& p);

	Ball *BallExists(const ID& id);
	void PopulateAddDelForBubble(PyObject *bubble, PyObject *bubbleAddList, PyObject *bubbleDelList);

public:
	/////////////////////////////////////////
	// Python thunkers

	PyObject* Py__init__ ( PyObject* args );
	PyObject* PyAddBall ( PyObject* args );
	PyObject* PyAddDynamicallyOrientedBall( PyObject* args );
	PyObject* PyAddOldStyleOrientedBall( PyObject* args );
	PyObject* PyAddCapsule( PyObject* args);
	PyObject* PyAddOrientedBox( PyObject* args);
	PyObject* PyFollowBall ( PyObject* args );
	PyObject* PyMissileFollow ( PyObject* args );
	PyObject* PyWarpTo ( PyObject* args );
	PyObject* PyGotoPoint ( PyObject* args );
	PyObject* PyGotoDirection ( PyObject* args );
	PyObject* PyOrbit ( PyObject* args );
	PyObject* PyStop ( PyObject* args );
	PyObject* PyClearAll ( PyObject* args );
	PyObject* PySetBallMass ( PyObject* args );
	PyObject* PySetBallRadius ( PyObject* args );
	PyObject* PySetMaxSpeed ( PyObject* args );
	PyObject* PySetBallPosition ( PyObject* args );
	PyObject* PySetBallVelocity ( PyObject* args );
	PyObject* PySetMaxAngularSpeed( PyObject* args );
	PyObject* PySetBallAngularVelocity( PyObject* args );
	PyObject* PySetBallRotation( PyObject* args );
	PyObject* PySetBallAngularAgility( PyObject* args );
	PyObject* PySetSpeedFraction ( PyObject* args );
	PyObject* PySetNotificationRange ( PyObject* args );
    PyObject* PySetTargetTracking ( PyObject* args );
	PyObject* PySetBallFree ( PyObject* args );
	PyObject* PySetBallMassive ( PyObject* args );
	PyObject* PySetBallGlobal ( PyObject* args );
	PyObject* PySetBallAgility ( PyObject* args );
	PyObject* PySetBallInteractive ( PyObject* args );
	PyObject* PyAddMushroom ( PyObject* args );
	PyObject* PyInitializeBubbles ( PyObject* args );
	PyObject* PyGetAccuracy ( PyObject* args );
	PyObject* PyAddProximitySensor ( PyObject* args );
	PyObject* PyRemoveProximitySensor ( PyObject* args );
	PyObject* PyCheckVisibility ( PyObject* args );
	PyObject* PyCloakBall ( PyObject* args );
	PyObject* PyUncloakBall ( PyObject* args );
	PyObject* PyGetBallIdsInRange ( PyObject* args );
	PyObject* PyGetBallIdsAndDistInRange ( PyObject* args );
	PyObject* PyGetSurfaceDist ( PyObject* args );
	PyObject* PyGetCenterDist ( PyObject* args );
	PyObject* PyScanCone ( PyObject* args );
	PyObject* PyGetBallBox ( PyObject* args );
	PyObject* PyGetStaticCollidableBox ( PyObject* args );
	PyObject* PyEvolve ( PyObject* args );
	PyObject* PyPause ( PyObject* args );
	PyObject* PyStart ( PyObject* args );
	PyObject* PyReadFullStateFromStream ( PyObject* args );
	PyObject* PyWriteFullStateToStream ( PyObject* args );
	PyObject* PyWriteBallsToStream ( PyObject* args );
	PyObject* PyCopyBubbles ( PyObject* args );
	PyObject* PyAdditionsAndDeletions ( PyObject* args );
	PyObject* PySetBoid ( PyObject* args );
	PyObject* PyGetRemoteFollowers ( PyObject* args );
	PyObject* PyGetFollowers ( PyObject* args );
	PyObject* PyGetActiveBoxes ( PyObject* args );
	PyObject* PySetBoxObject ( PyObject* args );
	PyObject* PyGetBoxObject ( PyObject* args );
	PyObject* PySetBallTroll ( PyObject* args );
	PyObject* PySetBallRigid ( PyObject* args );
	PyObject* PySetBallHarmonic ( PyObject* args );
	PyObject* PyGetBoxCenter ( PyObject* args );
	PyObject* PyGetBubbleAtCoordinates ( PyObject* args );
	PyObject* PyGetCurrentEgoPos ( PyObject* args );
	PyObject* PyLaunchMissile ( PyObject* args );
	PyObject* PySetBallFormation ( PyObject* args );
	PyObject* PyFormationFollow ( PyObject* args );
	PyObject* PyLoadFormations ( PyObject* args );
	PyObject* PySetBallNotInParkCallback ( PyObject* args );
	PyObject* PyEntityWarpIn ( PyObject* args );
	PyObject* PyAdjustTimes ( PyObject* args );
	PyObject* PyGetBallBoxKeys(PyObject* args);
	PyObject* PyGetBoxChildren(PyObject* args);
    PyObject* PyGetBoxKey(PyObject* args);    
	PyObject* PyEnableIterativeCollision( PyObject* args );
};

TYPEDEF_BLUECLASS(Ballpark);


#endif

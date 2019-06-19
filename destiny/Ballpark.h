/*
	*************************************************************************

	Ballpark.h

	Author:    Kjartan Pierre Emilsson
	Created:   January 2002
	OS:        Win32
	Project:   Destiny

	Description:

		Includes the headers for


	Dependencies:

		Blue and possibly Trinity

	(c) CCP 2000, 2001, 2002

	*************************************************************************
*/

#ifndef _DYNAMIC_H_
#define _DYNAMIC_H_

#include <trinity/Include/IEveBallpark.h>
#include "include/IDstConstants.h"

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

bool Quadratic(double& v1, double& v2, double a, double b, double c);
double CollideTwoSpheres(const Vector3d& p0, const Vector3d& p1, const Vector3d& q0, const Vector3d& q1, const double collRadius);

class Ballpark :
	public IEveBallpark,
	public IBlueEvents
{

friend class Ball;
friend class ClientBall;
friend class Capsule;
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

	Ball * AddBall(
		Ball *ball
		);

	Ball * AddBall(
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
		float speedFraction
		);

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
	// Moore makes 'srcId' seriously attracted to its mGoto, and shouldn't be able
	// to move more than 'radius' meters away from 'dstId's bounding radius by
	// collisions
	//
	// pre:
	//  'srcId' and 'dstId' exist. 'dstId' is fixed and 'srcId' is free.
	//   range >= 0.
	// post: Ball 'srcId' will now be unable to be bumped outside the mooring range
	//////////////////////////////////////////////////////////////////////////////
	void Moore(
		const ID& srcId,
		const ID& dstId
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
    Vector3d EvolveMoored(Ball* ball);
    void EvolveStop(Ball* ball);    // This one operates directly on the velocity, no need for return value

    bool IsAlignedForWarp(Ball* ball);

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
		);

	/////////////////////////////////////////////////////////////////////////////////////
	// IEveBallpark
	/////////////////////////////////////////////////////////////////////////////////////
	void Delta(
		Vector3* ref,
		Vector3* sref,
		Be::Time time
		);

	void DeltaVel(
		Vector3* vel,
		Be::Time time
		);

	float GetUnitBase(
		){return mSomeWeirdHackToFixSomething;};

	void SetUnitBase(
		float unit
		){mSomeWeirdHackToFixSomething = unit;};

	Vector3d* GetReferencePoint(Vector3d* out, Be::Time time);

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
	// Calculates the avoidance contribution to the dynamical state
	void Gradient(Ball *b);
	void Potential(Ball *me,Ball *other, int recurionsDepth=0);
	//void CalculateBoidPotential(Ball *, ListOfBalls &);

	// Actual infinitesimal integration
	void Integrate(Vector3d& p, Vector3d& v, const Vector3d& a, double mass, double k, double timefactor, double step);
	Vector3d GotoThrust(const Ball *b,const Vector3d& target,bool missile = false);
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
	PyObject* PyAddCapsule( PyObject* args);
	PyObject* PyAddOrientedBox( PyObject* args);
	PyObject* PyFollowBall ( PyObject* args );
	PyObject* PyMissileFollow ( PyObject* args );
	PyObject* PyWarpTo ( PyObject* args );
	PyObject* PyGotoPoint ( PyObject* args );
	PyObject* PyGotoDirection ( PyObject* args );
	PyObject* PyOrbit ( PyObject* args );
	PyObject* PyMoore ( PyObject* args );
	PyObject* PyStop ( PyObject* args );
	PyObject* PyClearAll ( PyObject* args );
	PyObject* PySetBallMass ( PyObject* args );
	PyObject* PySetBallRadius ( PyObject* args );
	PyObject* PySetMaxSpeed ( PyObject* args );
	PyObject* PySetBallPosition ( PyObject* args );
	PyObject* PySetBallVelocity ( PyObject* args );
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
};

TYPEDEF_BLUECLASS(Ballpark);


#endif

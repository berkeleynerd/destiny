#ifndef DST_PARTITION_H
#define DST_PARTITION_H

#include <hash_map>
#include <vector>
#include <hash_set>
#include <iterator>

struct Vector3d;

/*------------------------------------------------------------------------
   A Box is the building elment of a space Partition. Basically all
   of space is partitioned into a hierarchical collection of boxes.
   Each box maintains a list of Balls that intersect it.

   The partition keeps only tracks of hierarchies in which there are some
   balls, so its mostly empty space, as it should be.

   The partitition is built in the following manner:

   Space is covered by boxes and sub-boxes contained in them. The subdivision
   of space has a lower bound, which defines the maximum number of boxes
   in each space dimension for that level. We define this level with the
   2 exponent of the number of boxes and call 'mFineLevel'. The number of boxes
   in each dimension af that level is thus given by 2^mFineLevel. The set of
   boxes are centered divided equally around zero.

   On top of this fine grid, we define a number of coarser grids, that wholly
   contain the grid underneath. The subdivision exponent is defined by the
   variable 'mGridBase'. The number of division between level i and i+1
------------------------------------------------------------------------*/
class Box;
class Ball;
class Key;

typedef __int64 ID;
typedef stdext::hash_set<Ball *> SetOfBalls;
typedef stdext::hash_map<ID, Ball *> DictOfBalls;
typedef std::vector<Ball *> VectorOfBalls;
typedef std::pair<ID, Ball*> DictEntry;
typedef stdext::hash_map<Key,Box *> MapOfBoxes;
typedef stdext::hash_set<Box *> SetOfBoxes;
typedef std::insert_iterator< SetOfBoxes > BoxSetInsertor;
typedef std::vector<Box *> VectorOfBoxes;
typedef std::back_insert_iterator< VectorOfBoxes > BoxVectorInsertor;

class Partition;
class BoxAllocator
{
public:
    BoxAllocator();

    ~BoxAllocator();

    Box *GetNewBox(__int64 i,  __int64 j,  __int64 k, long level, Partition *p);

    void ReleaseBox(Box *box);

private:
    unsigned chunkSize;
    std::vector<void *> freeBoxes;
};

class Key
{
public:
    Key():ix(0),iy(0),iz(0),n(0) {};
    Key(__int64 i,__int64 j,__int64 k,__int64 N):ix(i),iy(j),iz(k),n(N) { Clamp(); };
    bool operator == ( const Key& k) const  { return ix==k.ix && iy==k.iy && iz==k.iz && n==k.n;};
    //less than operator
    bool operator < (const Key& k) const {
        if (ix < k.ix) return true;
        if (ix > k.ix) return false;
        if (iy < k.iy) return true;
        if (iy > k.iy) return false;
        if (iz < k.iz) return true;
        if (iz > k.iz) return false;
        return n<k.n;
    }
    //hash key
    operator size_t () const {
        return stdext::hash_compare<__int64>()((ix<<3) + (iy<<2) + (iz<<1) + n);
    }


    __int64 ix;
    __int64 iy;
    __int64 iz;
    __int64 n;

    void Clamp()
    {
        if(ix<0)
            ix = 0;
        else if(ix>=n)
            ix = n-1;
        if(iy<0)
            iy = 0;
        else if(iy>=n)
            iy = n-1;
        if(iz<0)
            iz = 0;
        else if(iz>=n)
            iz = n-1;
    }
};


class Partition
{
    friend Box;
public:
    Partition();

    //
    // GetBox(i,j,k,l) returns a box for the given grid coordinate at the given level
    // creating one if it didn't exist
    //
    Box* GetBox(__int64 ,  __int64 , __int64 ,  long level);
    //
    // CheckBox returns a box for the given grid coordinate at the given level
    // if and only if it already exists
    //
    Box *CheckBox(__int64 ix, __int64 iy, __int64 iz,  long level);
    //
    // GetBox(r,p) returns a box for the given 3D position and the level corresponding to radius
    // creating one if it didn't exist
    //
    Box* GetBox(double radius, const Vector3d& pos, bool create=true);

    //
    // GetExistingTopBox(p) returns the topmost box for the given 3D position
    // if and only if it already exists
    //
    Box* GetExistingTopBox(const Vector3d& pos);

    //
    // GetOffsetBox() and CheckOffsetBox() are similar to the the GetBox() and CheckBox() functions above
    // except the indices are interpreted as offsets from the box provided.
    //
    Box* GetOffsetBox(Box *box,__int64 i,__int64 j,__int64 k);
    Box* CheckOffsetBox(Box *box,__int64 i,__int64 j,__int64 k);

    // Returns the center of the box corresponding to given coordinate for given level    
    void GetBoxCenter(int level,  Vector3d& pos);

    // returns the total number of boxes in the partition
    int GetSize();

    // Given a box, returns the topmost box of the partition
    Box* GetTopmost(Box *box);

    // Clears the traversal flag for all topmost boxes
    void ClearTraversalFlag();

    // Debug function that logs the current content of all partitions
    void DumpPartition();

    // Returns the coordinates of all active boxes of a given level as Python list
    PyObject *GetActiveBoxes(int level);
    
    // Gets nearby balls that are pertinent to proximity sensors
    void GetProximityCandidates(Ball* ball, VectorOfBalls& uni, bool isMaster);
    
    // Gets nearby balls that are pertinent to collision detection
    void GetCollisionCandidates(Ball* ball, VectorOfBalls& uni, bool isMaster);

    // vector containing partitions for different levels. Zero is the coarsest level
    std::vector<MapOfBoxes> mLevels;
    std::vector<double> mLevelWidth; // size of partition boxes for each level
    std::vector<__int64> mLevelGrid; // number of grid partitions for each level

    int mNumberOfLevels;

    struct NearbyCriteria {
        NearbyCriteria(int casenum=0){ 
            memset(this, 0, sizeof(*this));
            switch(casenum) // initialize with a known 'static' filter case
            {
            case 1: EXCLUDE_MISSILES = EXCLUDE_CLOAKED = 1; break;
            case 3: EXCLUDE_CLOAKED = 1; break;
            }
        };
        unsigned int EXCLUDE_FIXED_BALLS      : 1;
        unsigned int EXCLUDE_INERT_BALLS      : 1;
        unsigned int EXCLUDE_MISSILES         : 1;
        unsigned int EXCLUDE_CLOAKED          : 1;
        unsigned int EXCLUDE_HARMONICS_TEST   : 1;
    };
	// Get balls that are 'close', according to the criteria specified in 'filter'
    void GetNearbyBalls(Ball* ball, VectorOfBalls& uni, bool isMaster, NearbyCriteria filter);

	private:

    void ExplodeChildren(SetOfBoxes &children,BoxVectorInsertor &ins);

    double mGridUnit;
    BoxAllocator mBoxAllocator;
};

#endif
#ifndef DST_SORTEDSETS_H
#define DST_SORTEDSETS_H

#include <set>

class Box;
class Ball;
class StaticCollidable;

class BoxSortComparer
{
public:
	bool operator () (const Box* x, const Box* y) const;
};
typedef std::set<Box*, BoxSortComparer> SortedSetOfBoxes;

class BallSortComparer
{
public:
	bool operator () (const Ball* x, const Ball* y) const;
};
typedef std::set<Ball*, BallSortComparer> SortedSetOfBalls;

class StaticCollidableSortComparer
{
public:
	bool operator () (const StaticCollidable* x, const StaticCollidable* y) const;
};
typedef std::set<StaticCollidable*, StaticCollidableSortComparer> SortedSetOfStaticCollidables;

#endif
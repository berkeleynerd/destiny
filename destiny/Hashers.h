#ifndef DST_HASHERS_H
#define DST_HASHERS_H



class Ball;
class Box;


class BoxPtrHasher
{	
public:
	size_t operator ()(const Box* b) const ;
	bool operator () (const Box* r, const Box* l) const ;
};


class BallPtrHasher
{	
public:
	size_t operator ()(const Ball* b) const ;
	bool operator () (const Ball* r, const Ball* l) const ;
};

#endif
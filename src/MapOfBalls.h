// Copyright © 2014 CCP ehf.

#ifndef DST_MAPOFBALLS
#define DST_MAPOFBALLS

#include "Ball.h"

class MapOfBalls
{
public:
#ifdef DESTINY_EMBEDDED
	std::unordered_map<ID, Ball*> mNative;

	MapOfBalls() = default;

	~MapOfBalls()
	{
		Clear();
	}

	Ball* operator [] (const ID id)
	{
		auto found = mNative.find(id);
		return found == mNative.end() ? nullptr : found->second;
	}

	void Add(const ID id, Ball* ball)
	{
		ball->GetRawRoot()->Lock();
		auto found = mNative.find(id);
		if(found != mNative.end())
		{
			found->second->GetRawRoot()->Unlock();
		}
		mNative[id] = ball;
	}

	Ball* GetBall(const ID id)
	{
		Ball* ball = this->operator[](id);
		if(!ball)
		{
			ball = new OBall;
			Add(id, ball);
			((IBall*)ball)->Unlock();
		}
		return ball;
	}

	bool RemoveBall(const ID id)
	{
		auto found = mNative.find(id);
		if(found == mNative.end())
			return false;
		found->second->GetRawRoot()->Unlock();
		mNative.erase(found);
		return true;
	}

	size_t GetSize()
	{
		return mNative.size();
	}

	void Clear()
	{
		for(auto& entry : mNative)
			entry.second->GetRawRoot()->Unlock();
		mNative.clear();
	}
#else
	PyObject* mDict;

	MapOfBalls()
	{
		mDict = PyDict_New();
	}


	~MapOfBalls()
	{
		Clear();
		Py_DECREF(mDict);
	}

	Ball* operator [] (const ID id)
	{
		PyObject *key = PyLong_FromLongLong(id);
		PyObject* pyBall = PyDict_GetItem(mDict, key);
		Py_DECREF(key);

		/*	
		// this cast is rather evil but as this is a very time 
		// critical part of the system we use our knowledge to do evil			
		return (Ball*)(void*)((BluePythonObject*)pyBall)->mObj;
		*/
		if(!pyBall)
			return NULL;	
		IRoot* top = ((BluePythonObject*)pyBall)->mObj;
		return (top->ClassType() == Ball::ClassType_()) ? (Ball*)(void*)top : (ClientBall*)(void*)top;			
	}

	void Add(const ID id,Ball *ball)
	{
		PyObject* key = PyLong_FromLongLong(id);
		PyObject* pyball = PyOS->WrapBlueObject((IBall*)ball);
		PyDict_SetItem(mDict,key,pyball);
		Py_DECREF(key);
		Py_DECREF(pyball);
	}

	Ball* GetBall(const ID id)
	{
		Ball* ball = this->operator[](id);
		
		if (!ball)
		{
			ball = new OBall;
			Add(id, ball);
		}
		return ball;		
	}

	bool RemoveBall(const ID id)
	{
		PyObject* key = PyLong_FromLongLong(id);
		int ret = PyDict_DelItem(mDict, key);
		Py_DECREF(key);

		if (ret == -1)
		{
			PyErr_Print();
			return false;
		}
		else
		{
			return true;
		}
	}

	size_t GetSize()
	{
		return PyDict_Size(mDict);
	}

	void Clear()
	{
		PyDict_Clear(mDict);
	}
#endif

};

class BallIterator
{
public:
#ifdef DESTINY_EMBEDDED
	BallIterator(MapOfBalls *mapOball) :
		mMapOfBalls(mapOball),
		mIterator(mapOball->mNative.begin())
	{
	}

	MapOfBalls *mMapOfBalls;
	std::unordered_map<ID, Ball*>::iterator mIterator;

	Ball* operator++(int)
	{
		return Next();
	}

	Ball* Next()
	{
		if(mIterator == mMapOfBalls->mNative.end())
			return nullptr;
		return (mIterator++)->second;
	}

	void Begin()
	{
		mIterator = mMapOfBalls->mNative.begin();
	}
#else
	BallIterator(MapOfBalls *mapOball) :
		mMapOfBalls(mapOball),key(0),value(0)
	{
		pos =0;
	};

	MapOfBalls *mMapOfBalls;
	Py_ssize_t pos;
	PyObject* key;	
	PyObject* value;

	Ball* operator++(int dummy)
	{
		return Next();
	}

	Ball* Next()
	{
		if (PyDict_Next(mMapOfBalls->mDict, &pos, &key, &value))
		{			
			// this cast is rather evil but as this is a very time 
			// critical part of the system we use our knowledge to do evil
			// the safe way
			// return BallPtr(value);			
			// Before clientball
			// return (Ball*)(void*)((BluePythonObject*)value)->mObj;
			IRoot* top = ((BluePythonObject*)value)->mObj;
			return (top->ClassType() == Ball::ClassType_()) ? (Ball*)(void*)top : (ClientBall*)(void*)top;
		}
		return NULL;
	}

	void Begin()
	{
		pos = 0;
	}
#endif
};


#endif

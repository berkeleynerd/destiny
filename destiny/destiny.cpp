/*
	*************************************************************************

	destiny.cpp

	Author:    Matthias Gudmundsson
	Created:   Nov. 2001
	OS:        Win32
	Project:   Destiny

	Description:

		destiny DLL


	Dependencies:

		Blue, Trinity

	(c) CCP 2000, 2001, 2002

	*************************************************************************
*/

#include "stdafx.h"


#include "Ball.h"
#include "Ballpark.h"
#ifdef DESTINY_AUTHORING
#include "Galaxy.h"
#endif
#include "Partition.h"
#include "DstConstants.h"

// Define the interfaces we use from Trinity
BLUE_DEFINE_INTERFACE( ITriVectorFunction );
BLUE_DEFINE_INTERFACE( ITriQuaternionFunction );

// interfaces
BLUE_DEFINE_INTERFACE(IBall);
BLUE_DEFINE_INTERFACE(IBallpark);

// classes
BLUE_DEFINE(Ballpark);
BLUE_DEFINE(Ball);
BLUE_DEFINE(ClientBall);
BLUE_DEFINE(MiniBall);
BLUE_DEFINE(MiniBox);
BLUE_DEFINE(MiniCapsule);

#include <Util/PairingHeap.h>

#include "include/Quaternion.h"

#include <limits>
#ifdef max //remove silly max macro, for limits
#undef max
#endif

char _bcfix[8];

const char* g_moduleName = "destiny";

static CcpLogChannel_t s_chPark = CCP_LOG_DEFINE_CHANNEL( "BallPark" );

struct HeapEntry
{
	HeapEntry(PyObject *o) : mObject(o, true) {}
	bool operator < (const HeapEntry &rhs) const {
		double a1 = PyFloat_AS_DOUBLE(PyList_GET_ITEM(mObject.o, 7));
		double a2 = PyFloat_AS_DOUBLE(PyList_GET_ITEM(rhs.mObject.o, 7));
		return a1 < a2;
	}
	operator PyObject * () const { return mObject; }
	operator PyListObject * () const { return (PyListObject*)mObject.o; }
	BluePy mObject;
};


//here is a path finding algorithm, based on Dijkstra's algorithm.
//Each node is represented by a python list, where temporary stuff
//for the algorithm is also stored.
//Each list is like this>
/*
 # Current cache format:
# 0: Jump count  (int)
# 1: origin solarSystemID (int)
# 2: from-solarsystem cache entry (points to another list like this, or None)
# 3: marker (Long, points to the place in the priority queue, temporary)
# 4: list of jump cache entries (list of other lists like this)
# 5: security rating
# 6: avoid flag (boolean)
# 7: weight count  (float, the adjusted distance from origin to here)
*/
// Each edge has the distance 1 (one jump) by default, but this can
// be changed with the security rating of the target system in question.  It would
// be best if each system simply had a penalty multiplyer on it, to be applied to
// the edge length of 1.  Then we could remove all this avoidance logic out of here
// and into python where it belongs.  
// Note that we can provide an optional destination ID, and the algorithm will
// terminate when it has reached that node.
static PyObject* FindShortestPath(PyObject* module, PyObject *args)
{
	PyObject *jumps;
	typedef PairingHeap<HeapEntry> heap_t;
	heap_t q;
	int originSolarSystemID;
	float secLower = -1.0f;
	float secUpper = 1.0f;
	float penalty = 3.0f;
	int avoidSystems = 0;
	int strictSecurity = 0;
	int destinationSolarSystemID = -1;

	if(!PyArg_ParseTuple(args,"Oii|fffii", &jumps, &originSolarSystemID, &avoidSystems, &secLower,
					     &secUpper, &penalty, &strictSecurity, &destinationSolarSystemID))
		return 0;

	// Create the vertex map in STL
	Py_ssize_t pos = 0;
	PyObject *key, *value;

	// Round to two decimal places just to be sure
	secLower = int(secLower*100.0f+0.05f)/100.0f;
	secUpper = int(secUpper*100.0f+0.05f)/100.0f;

	// Filter routes unless the security range would include all systems [-1.0..1.0]
	bool checkSecurity = (secLower != -1.0 || secUpper != 1.0) && secUpper > secLower && secUpper <= 1.0 && secLower >= -1.0;

	const double maxFloat = std::numeric_limits<double>::max();
	const int maxInt = std::numeric_limits<int>::max();

	// Initialize the cache for a new query
	while (PyDict_Next(jumps, &pos, &key, &value))
	{
		//safety.  Ignore non-ints and non lists
		if (!PyInt_Check(key) || !PyList_Check(value))
			continue;
		if (PyList_GET_SIZE(value) < 8)
			continue;

		if(PyInt_AS_LONG(key)==originSolarSystemID)
		{
			// If this is the origin we want to initialize distance to zero
			// and not subject it to any filtering.
			// use the non-macro version to release old values)
			PyList_SetItem(value, 0, PyInt_FromLong(0)); // number of jumps
			PyList_SetItem(value, 7, PyFloat_FromDouble(0.0)); // this is the weighted distance
			Py_INCREF(Py_None);
			PyList_SetItem(value, 2, Py_None); //the prev field.
		}
		else
		{
			// Initialize jump count to infinity for all other systems
			PyList_SetItem(value, 0, PyInt_FromLong(maxInt));
			PyList_SetItem(value, 7, PyFloat_FromDouble(maxFloat));
			// Now check if we want to exclude the system due to some filtering option
			if(avoidSystems)
			{
				// Check if this system is specifically flagged for avoidance
				if(PyObject_IsTrue(PyList_GET_ITEM(value,6)))
				{
					Py_INCREF(Py_None);
					PyList_SetItem(value, 3, Py_None );
					continue;
				}
			}

			if(strictSecurity && checkSecurity)
			{
				double systemRating =  PyFloat_AS_DOUBLE(PyList_GET_ITEM(value,5));

				// Skip system if its security rating falls outside the bounds
				if(systemRating < secLower || systemRating > secUpper)
				{
					Py_INCREF(Py_None);
					PyList_SetItem(value, 3, Py_None );
					continue;
				}
			}
		}

		heap_t::nodeptr_t node = q.insert(value);
		//store the pointer to its place in the priority queue, plus mark as being unprocessed.
		PyList_SetItem(value, 3, PyLong_FromVoidPtr(node));
	}

	while(!q.is_empty())
	{
		// Get the currently lowest point
		HeapEntry currentVertex = q.remove_min();
		
		//remove the marker, it has now been handled
		Py_INCREF(Py_None);
		PyList_SetItem(currentVertex, 3, Py_None);

		// this is the weighted distance to it from the origin
		double currentDist = PyFloat_AS_DOUBLE(PyList_GET_ITEM(currentVertex,7));

		//CCP_LOG_CH( s_chPark,"Found vertex %d out of %d with distance %f", i, length, weigthedDist);

		if(currentDist == maxFloat)
		{
			//This vertex was never reached, ergo isn't connected to the origin.
			//CCP_LOG_CH( s_chPark,"Infinite distance, breaking out");
			break;
		}

		//set the jump count, this comes from the prev. member
		PyObject *prev = PyList_GET_ITEM(currentVertex,2);
		if (prev != Py_None) 
			PyList_SetItem(currentVertex, 0, PyInt_FromLong(PyInt_AS_LONG(PyList_GET_ITEM(prev, 0)) + 1));

		// if this is the destination vertex, we can stop here
		if (destinationSolarSystemID != -1 && PyInt_AS_LONG(PyList_GET_ITEM(currentVertex, 1)) == destinationSolarSystemID)
			break;
		
		// Cycle over all the neighbouring vertices running to that point
		PyObject *edges = PyList_GET_ITEM(currentVertex,4);
		for(int j=0; j < PyList_GET_SIZE(edges); j++)
		{
			PyObject *edge = PyList_GET_ITEM(edges,j);
			
			// Check if that specific vertex has been handled
			// not strictly necessary since if it has been handled, the new distance
			// will be higher than the old distance and nothing will happen.
			PyObject *marker = PyList_GET_ITEM(edge, 3);
			if (marker == Py_None)
				continue;
			
			// Get the distance value of the vertex on other end of edge
			double vertexDist = PyFloat_AS_DOUBLE(PyList_GET_ITEM(edge,7));

			// compute new dist if arrived at through currentVertex
			double newVertexDist = currentDist + 1.0; // This is the default (distance of one jump is 1), but can be overridden

			if(checkSecurity && !strictSecurity)
			{
				double systemRating =  PyFloat_AS_DOUBLE(PyList_GET_ITEM(edge,5));

				// Penalize system if its security rating falls outside the bounds
				// Additionally, apply twice the penalty for routing through nullsec outside the bounds
				if(systemRating <= 0.0 && systemRating <= secLower)
				{
					newVertexDist = currentDist + 2*penalty;
				}
				else if(systemRating > secUpper)
				{
					newVertexDist = currentDist + penalty;
				}
				else if(systemRating < secLower)
				{
					newVertexDist = currentDist + penalty;
				}
				else
				{
					if ( systemRating >= 0.45 )
						systemRating = 1.0;
					else if ( systemRating > 0.0 )
						systemRating = 0.45;

					newVertexDist = currentDist + .1000000000/(secLower-secUpper)*systemRating + .1000000000*(9.*secLower-10.*secUpper)/(secLower-secUpper);
				}
				
			}

			if(vertexDist > newVertexDist)
			{
				//we have found a shorter path to the edge vertex!
				//store new distance, and from where it was got
				PyList_SetItem(edge, 7, PyFloat_FromDouble(newVertexDist));
				Py_INCREF((PyObject*)currentVertex);
				PyList_SetItem(edge, 2, currentVertex);
				
				//update the node's position in the heap
				heap_t::nodeptr_t nodeptr = (heap_t::nodeptr_t)PyLong_AsVoidPtr(marker);
				q.decrease_key(nodeptr);
				
				//CCP_LOG_CH( s_chPark,"Updated to %f", newEdgeWeight);
			}
		}		
	}
	//CCP_LOG_CH( s_chPark,"Pathfinding finished");
	// vertices should now be handled
	Py_INCREF(Py_None);
	return Py_None;
}


PyObject* GetBoxCenter(
	PyObject* module, PyObject* args
	)
{
	Vector3d pos;
	int level;

	if (!PyArg_ParseTuple(args, "iddd",
		&level,
		&pos.x,
		&pos.y,
		&pos.z
		))
		return NULL;

	Partition partition;
	if(level < 0 || level >= partition.mNumberOfLevels)
	{
		PyErr_SetString(PyExc_TypeError, "Illegal level");
		return 0;
	}

	partition.GetBoxCenter(level, pos);

	PyObject *ret = PyTuple_New(3);
	PyTuple_SET_ITEM(ret, 0, PyFloat_FromDouble( pos.x ));
	PyTuple_SET_ITEM(ret, 1, PyFloat_FromDouble( pos.y ));
	PyTuple_SET_ITEM(ret, 2, PyFloat_FromDouble( pos.z ));

	return ret;
}
PyObject* Test(PyObject* , PyObject* );


static PyMethodDef destiny_methods[] = {
	{"FindShortestPath", (PyCFunction)FindShortestPath, METH_VARARGS, "Find shortest path between two systems"},
	{"Test",             (PyCFunction)Test, METH_VARARGS, "Generic test function"},
	{"GetBoxCenter",     (PyCFunction)GetBoxCenter,   METH_VARARGS, "Gives you back a partition centered coordinate"},
    {NULL,      NULL}         /* sentinel */
};

//////////////////////////////////////////////////////////////////////
//
// Class Factory List
//
//////////////////////////////////////////////////////////////////////
#define BLUECLASSFACTORY(_Class) \
	{_Class::ClassType_(), SimpleFactory<O##_Class>::Create},

#define BLUESTATICCLASS(_Class) \
	{_Class::ClassType_(), SingletonFactory<C##_Class>::Create},


static const Be::ClassRegistration classes[] =
{
	BLUECLASSFACTORY(Ball)
	BLUECLASSFACTORY(ClientBall)
	BLUECLASSFACTORY(Ballpark)
	BLUECLASSFACTORY(MiniBall)
	BLUECLASSFACTORY(MiniBox)
	BLUECLASSFACTORY(MiniCapsule)
#ifdef DESTINY_AUTHORING
	BLUECLASSFACTORY(Galaxy)
#endif
	{NULL}
};



//////////////////////////////////////////////////////////////////////
//
// DLL Startup / Shutdown
//
//////////////////////////////////////////////////////////////////////

// reduce CRT link
extern "C" void _setargv(){}
extern "C" void _setenvp(){}

PyObject* gThisModule = NULL;

#ifdef _WIN32
//--------------------------------------------------------------------
BOOL APIENTRY DllMain(HINSTANCE instance, DWORD  reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}
#endif

//--------------------------------------------------------------------
// init_destiny - python dll module entry function
//--------------------------------------------------------------------
extern "C" void
#ifdef _MSC_VER
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
CCP_CONCATENATE( init_destiny, CCP_BUILD_FLAVOR )()
{
    CCP_LOG( "Destiny compiled %s %s starting", __DATE__, __TIME__);
	CCP_LOG( "Size of balls: %d bytes", sizeof(Ball));
    
	BeClasses->RegisterClasses(classes);
    
	// put myself into python as a module
	PyObject* module = Py_InitModule( CCP_STRINGIZE( CCP_CONCATENATE( _destiny, CCP_BUILD_FLAVOR ) ), destiny_methods );
	gThisModule = module;
	PyObject* dict = PyModule_GetDict(module);
    
    
	// constants
	AddDstConstants(dict);
}


/*********************************************************************/
/*                                                                   */
/*                                                                   */
/*                                                                   */
/*                                                                   */
/*********************************************************************/

PyObject* Test(PyObject* module, PyObject* args)
{
	PyObject *coords;
	PyObject *probes;
	char *flags;

	if(!PyArg_ParseTuple(args, "sOO", 
		&flags,
		&coords,
		&probes
		))
	{
		return 0;
	}
	

	Py_INCREF(Py_None);
	return Py_None;
}

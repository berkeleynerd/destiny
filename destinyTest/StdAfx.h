
#include "gtest/gtest.h"

#define BLUE_OVERRIDE_VECTOR_TYPES 1
#define CCP_MATH_USE_OWN_XNA_MATH 1

#include "CcpMath/include/CcpMath.h"
#include "BlueExposure/include/BlueExposure.h"
#include <blue/include/blue.h>
#include <blue/include/IBluePython.h>
#include <blue/include/IBlueOS.h>

#include "destiny/Ball.h"
#include "destiny/Ballpark.h"
#include "destiny/Box.h"
#include "destiny/DstConstants.h"
#include "destiny/Hashers.h"
#include "destiny/MapOfBalls.h"
#include "destiny/MapOfLongLongs.h"
#include "destiny/MiniBall.h"
#include "destiny/Partition.h"
#include "destiny/Random.h"
#include "destiny/resource.h"
#include "destiny/Vector3d.h"
#include "destiny/Triangle.h"
#include "destiny/Plane.h"
#include "destiny/Collision.h"
#include "destiny/BoxShape.h"
#include "destiny/AABB.h"

#include "destiny/include/destinyId.h"
#include "destiny/include/IBall.h"
#include "destiny/include/IDstConstants.h"
#include "destiny/include/Quaternion.h"
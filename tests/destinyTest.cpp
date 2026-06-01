// Copyright © 2015 CCP ehf.

#include "StdAfx.h"

const char* g_moduleName = "DestinyTest";

int main( int argc, char **argv )
{
	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}


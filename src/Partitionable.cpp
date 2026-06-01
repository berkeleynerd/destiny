// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "Partitionable.h"

Partitionable::Partitionable():
	mId(0),
	mMyBox(nullptr),
	mPark(nullptr),
	mHandled(false),
	mNewBubble(-1),
	mOldBubble(-1),
	isInteractive(false),
	mEffectStamp(0),
	isMoribund(false)
{

}
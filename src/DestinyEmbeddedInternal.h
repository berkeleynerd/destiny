// Copyright (c) 2026 CCP Games

#pragma once

#include <cstdint>

#ifdef DESTINY_EMBEDDED
void DestinyEmbeddedRecordOnTick();
void DestinyEmbeddedRecordPythonCallback();
void DestinyEmbeddedRecordStart();
uint64_t DestinyEmbeddedGetOnTickCount();
uint64_t DestinyEmbeddedGetPythonCallbackCount();
uint64_t DestinyEmbeddedGetStartCount();
void DestinyEmbeddedResetRuntimeCounters();

#define DESTINY_PY_SEND_EVENT( ... ) ( DestinyEmbeddedRecordPythonCallback(), true )
#define DESTINY_PY_POST_EVENT( ... ) ( DestinyEmbeddedRecordPythonCallback(), true )
#else
#define DESTINY_PY_SEND_EVENT( ... ) PyOS->SendEvent( __VA_ARGS__ )
#define DESTINY_PY_POST_EVENT( ... ) PyOS->PostEvent( __VA_ARGS__ )
#endif

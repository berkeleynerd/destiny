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

// Collision/proximity SendEvent sites stay silenced in embedded builds;
// their semantics are chartered to the combat/proximity contracts
// (docs/thunker-contract-audit.md). The three warp PostEvent sites forward
// to the embedded session's host callback so the event contract survives
// the Python excision in executable form.
bool DestinyEmbeddedPostEvent(
	void* ballpark,
	const char* timerName,
	const char* eventName,
	const char* format,
	long long ballId,
	long long eventTime );

#define DESTINY_PY_SEND_EVENT( ... ) true
#define DESTINY_PY_POST_EVENT( park, timerName, eventName, format, ... ) \
	DestinyEmbeddedPostEvent( (void*)( park ), timerName, eventName, format, __VA_ARGS__ )
#else
#define DESTINY_PY_SEND_EVENT( ... ) PyOS->SendEvent( __VA_ARGS__ )
#define DESTINY_PY_POST_EVENT( ... ) PyOS->PostEvent( __VA_ARGS__ )
#endif

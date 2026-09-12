//========= Copyright Valve Corporation, All rights reserved. ============//
#pragma once

#define NETWORK_VAR_CHANGE_CALLBACKS( name ) \
	virtual void NetworkStateChanged_##name() {} \
	virtual void NetworkStateChanged_##name( void *pVar ) {}

#define DISABLE_NETWORK_VAR_FOR_DERIVED( name ) \
	NETWORK_VAR_CHANGE_CALLBACKS( name ) \
	virtual bool IsNetworkVarEnabled_##name() { return false; }

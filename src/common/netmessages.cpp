#include "common/netmessages.h"
#include "tier0/callbacks.h"

using ClientTickProcessFn = bool (*)(CLC_ClientTick*);
using ClientTickReadFn = bool (*)(CLC_ClientTick*, bf_read*);
using ClientTickWriteFn = bool (*)(CLC_ClientTick*, bf_write*);
using ClientTickToStringFn = const char* (*)(const CLC_ClientTick*);

static ClientTickProcessFn s_ClientTickProcess;
static ClientTickReadFn s_ClientTickRead;
static ClientTickWriteFn s_ClientTickWrite;
static ClientTickToStringFn s_ClientTickToString;

CLC_ClientTick::CLC_ClientTick()
	: m_nDeltaTick(0), m_nStringTableTick(0), m_flFrameTime(0), m_flFrameTimeStdDeviation(0), m_nServerCPU(0)
{
	m_nGroup = 0;
	m_bReliable = false;
	m_NetChannel = nullptr;
	m_pMessageHandler = nullptr;
}

bool CLC_ClientTick::Process()
{
	return s_ClientTickProcess(this);
}

bool CLC_ClientTick::ReadFromBuffer(bf_read* buffer)
{
	return s_ClientTickRead(this, buffer);
}

bool CLC_ClientTick::WriteToBuffer(bf_write* buffer)
{
	return s_ClientTickWrite(this, buffer);
}

const char* CLC_ClientTick::ToString() const
{
	return s_ClientTickToString(this);
}

ON_DLL_LOAD_CLIENT("engine.dll", ClientTickMessageMethods, [](CModule module)
{
	s_ClientTickProcess = module.Offset(0x75CC0).RCast<ClientTickProcessFn>();
	s_ClientTickRead = module.Offset(0x220AF0).RCast<ClientTickReadFn>();
	s_ClientTickWrite = module.Offset(0x22AA80).RCast<ClientTickWriteFn>();
	s_ClientTickToString = module.Offset(0x229540).RCast<ClientTickToStringFn>();
})

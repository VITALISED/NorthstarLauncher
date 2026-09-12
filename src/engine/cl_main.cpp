#include "cdll_int.h"
#include "common/netmessages.h"
#include "core/tier0.h"
#include "core/tier1.h"
#include "engine/cdll_int.h"
#include "engine/client/clientstate.h"
#include "engine/demo.h"
#include "engine/isplitscreen.h"
#include "engine/r2engine.h"
#include "tier0/hooks.h"
#include "tier1/convar.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

DECLARE_MODULE(EngineClient)


using CLSendMoveFn = void (*)();

CLSendMoveFn CL_SendMove;

IBaseClientDLL** s_ppClientDLL;
CDemoPlayer** s_ppDemoPlayer;
ConVar** s_ppHostTimescale;
ConVar** s_ppCmdRate;
ISplitScreen** s_ppSplitScreenManager;
IDemoRecorder** s_ppDemoRecorder;
double* s_pNetTime;
float* s_pIntervalPerTick;
float* s_pClientFrameTime;
float* s_pClientFrameTimeStdDeviation;
float* s_pServerCPUPercent;
double s_lastMovementCall;
float s_lastFrameTime;

IVEngineClient* g_pEngineClient;
char* g_pLocalPlayerUserID;
char* g_pLocalPlayerOriginToken;
GetBaseLocalClientType GetBaseLocalClient;
GetLocalPlayerIndexType GetLocalPlayerIndex;


void SendClientTick(CClientState* client, CNetChan* channel)
{
	CLC_ClientTick tickMessage;
	tickMessage.m_nDeltaTick = client->m_nDeltaTick;
	tickMessage.m_nStringTableTick = client->m_nStringTableAckTick;
	tickMessage.m_flFrameTime = *s_pClientFrameTime;
	tickMessage.m_flFrameTimeStdDeviation = *s_pClientFrameTimeStdDeviation;
	tickMessage.m_nServerCPU = static_cast<std::uint8_t>(*s_pServerCPUPercent * 100.0f);

	channel->SendNetMsg(tickMessage, false, false);
}

DECLARE_HOOK(CL_Move, engine.dll + 0x734C0, [](auto&, float, bool finalTick)
{
	CClientState* const client = GetBaseLocalClient();
	if (static_cast<int>(client->m_nSignonState) < static_cast<int>(eSignonState::CONNECTED))
	{
		s_lastFrameTime = 0.0f;
		return;
	}

	if (!Host_ShouldRun() || (*s_ppDemoPlayer)->IsPlayingBack())
		return;

	const int commandTick =
		client->m_pCurrentFrameSnapshot ? client->m_pCurrentFrameSnapshot->m_nCommandTick : -1;
	const int pendingCommandCount = client->m_nOutgoingCommandNumber - commandTick + 1;

	float minimumCommandFrameTime;

	// this really fucking pisses me off
	if(g_pVanillaCompatibility->GetVanillaCompatibility())
		minimumCommandFrameTime = 0.005f; // we will speedhack on vanilla if we don't do this
	else
		minimumCommandFrameTime = 0.001f; // need this for listen servers to work properly, smooth to around ~1000 fps

	constexpr int maxNewCommands = 15;
	constexpr float maxFrameTime = 0.1f;

	CNetChan* const channel = client->m_NetChannel;
	const float hostTimeScale = (*s_ppHostTimescale)->GetFloat();
	const bool isTimeScaleDefault = hostTimeScale == 1.0f;
	const float netTime = static_cast<float>(*s_pNetTime);

	bool sendPacket = true;
	const bool packetIsDue = client->m_flNextCmdTime <= netTime;
	if (packetIsDue && (finalTick || pendingCommandCount >= maxNewCommands))
		sendPacket = channel->CanPacket();
	else if (pendingCommandCount < maxNewCommands || isTimeScaleDefault)
		sendPacket = false;

	const bool isActive = client->m_nSignonState == eSignonState::FULL;
	if (isActive)
	{
		const double movementCallTime = g_PlatFloatTime();
		const float elapsedMovementCallTime = static_cast<float>(movementCallTime - s_lastMovementCall);
		const int outgoingCommandNumber = client->m_nOutgoingCommandNumber;
		const bool isPaused = client->IsPaused();
		const int nextCommandNumber = isPaused ? outgoingCommandNumber : outgoingCommandNumber + 1;

		if (!(*s_ppSplitScreenManager)->IsDisconnecting(0))
		{
			IBaseClientDLL* const clientDLL = *s_ppClientDLL;
			float timeScale;
			float frameTime;
			float deltaTime;

			if (isPaused)
			{
				timeScale = 1.0f;
				frameTime = elapsedMovementCallTime;
				deltaTime = frameTime;
			}
			else
			{
				timeScale = hostTimeScale;
				frameTime = client->GetFrameTime() + s_lastFrameTime;
				deltaTime = frameTime / timeScale;
			}

			if (deltaTime > maxFrameTime)
				frameTime = timeScale * maxFrameTime;

			if (isTimeScaleDefault && deltaTime < minimumCommandFrameTime)
			{
				s_lastFrameTime = frameTime;
				return;
			}

			s_lastFrameTime = 0.0f;
			clientDLL->SetInputSampleTime(frameTime);
			clientDLL->CreateMove(nextCommandNumber, frameTime, !isPaused);
			client->m_nOutgoingCommandNumber = nextCommandNumber;
			if ((*s_ppDemoRecorder)->IsRecording())
				(*s_ppDemoRecorder)->RecordUserInput(nextCommandNumber);
		}

		if (sendPacket)
			CL_SendMove();
		else
			channel->SetChoked();

		s_lastMovementCall = movementCallTime;


	}

	if (sendPacket)
	{
		if (isActive)
			SendClientTick(client, channel);

		channel->SendDatagram(nullptr);

		const float commandPacketInterval = 1.0f / (*s_ppCmdRate)->GetFloat();
		const float maxPacketTimeAdjustment = std::max(*s_pIntervalPerTick, commandPacketInterval);
		const float delta = netTime - static_cast<float>(client->m_flNextCmdTime);
		const float packetTimeAdjustment = std::clamp(delta, 0.0f, maxPacketTimeAdjustment);
		client->m_flNextCmdTime =
			static_cast<double>(commandPacketInterval + netTime - packetTimeAdjustment);
	}
})


ON_DLL_LOAD_CLIENT_RELIESON("engine.dll", R2EngineClient, ConCommand, [](CModule module)
{
    g_pEngineClient = Sys_GetFactoryPtr("engine.dll", VENGINE_CLIENT_INTERFACE_VERSION).RCast<IVEngineClient*>();
    g_pLocalPlayerUserID = module.Offset(0x13F8E688).RCast<char*>();
	g_pLocalPlayerOriginToken = module.Offset(0x13979C80).RCast<char*>();
	GetBaseLocalClient = module.Offset(0x78200).RCast<GetBaseLocalClientType>();
	GetLocalPlayerIndex = module.Offset(0x52260).RCast<GetLocalPlayerIndexType>();
	CL_SendMove = module.Offset(0x74F10).RCast<CLSendMoveFn>();

	s_ppClientDLL = module.Offset(0xF849AA8).RCast<IBaseClientDLL**>();
	s_ppDemoPlayer = module.Offset(0xFD15608).RCast<CDemoPlayer**>();
	s_ppHostTimescale = module.Offset(0x1315A2A8).RCast<ConVar**>();
	s_ppCmdRate = module.Offset(0xFDA5AC8).RCast<ConVar**>();
	s_ppSplitScreenManager = module.Offset(0x7A6490).RCast<ISplitScreen**>();
	s_ppDemoRecorder = module.Offset(0xFD14FB8).RCast<IDemoRecorder**>();
	s_pNetTime = module.Offset(0x13FA2DE0).RCast<double*>();
	s_pIntervalPerTick = module.Offset(0x7CB418).RCast<float*>();
	s_pClientFrameTime = module.Offset(0x13158BA4).RCast<float*>();
	s_pClientFrameTimeStdDeviation = module.Offset(0x13158BAC).RCast<float*>();
	s_pServerCPUPercent = module.Offset(0x130024C0).RCast<float*>();

	DISPATCH_MODULE(EngineClient)
})

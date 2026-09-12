#pragma once

class ClientClass;
class IClientUnknown;
struct LerpData;

enum DataUpdateType_t : int
{
	DATA_UPDATE_INVALID = 0,
	DATA_UPDATE_CREATED = 1,
	DATA_UPDATE_DATATABLE_CHANGED = 2,
};

enum ShouldTransmitState_t : int
{
	SHOULDTRANSMIT_START = 0,
	SHOULDTRANSMIT_END = 1,
};

class IClientNetworkable
{
public:
	virtual IClientUnknown* GetIClientUnknown() = 0; // 0
	virtual void Release() = 0; // 1
	virtual ClientClass* GetClientClass() = 0; // 2
	virtual void NotifyShouldTransmit(ShouldTransmitState_t state) = 0; // 3
	virtual void OnPreDataChanged(DataUpdateType_t updateType) = 0; // 4
	virtual void OnDataChangedPreAnim(DataUpdateType_t updateType) = 0; // 5
	virtual void OnDataChanged(DataUpdateType_t updateType) = 0; // 6
	virtual void PreDataUpdate(DataUpdateType_t updateType) = 0; // 7
	virtual void PostDataUpdate(DataUpdateType_t updateType, float oldTime, float newTime) = 0; // 8
	virtual void CalculateLocalPlayer(DataUpdateType_t updateType) = 0; // 9
	virtual void OnFinishAllNetUpdates(DataUpdateType_t updateType) = 0; // 10
	virtual void PostDataOnCreate() = 0; // 11
	virtual void RecalculateOrigin() = 0; // 12
	virtual void UpdateMoveParent() = 0; // 13
	virtual void PreSnapshotTransition() = 0; // 14
	virtual LerpData* GetLerpData() = 0; // 15
	virtual bool IsDormant() const = 0; // 16
	virtual int entindex() const = 0; // 17
	virtual void* GetDataTableBasePtr() = 0; // 18
	virtual void SetDestroyedOnRecreateEntities() = 0; // 19
	virtual void FakeRelease() = 0; // 20
	virtual void FakeRecreate() = 0; // 21
};

static_assert(sizeof(IClientNetworkable) == sizeof(void*));

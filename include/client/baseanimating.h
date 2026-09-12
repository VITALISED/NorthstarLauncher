#pragma once

#include "client/baseentity.h"

class CStudioHdr;
class IBoneSetup;
class CBoneBitList;
class C_BoneAccessor;
class C_ClientRagdoll;
struct BoneList;

class C_BaseAnimating : public C_BaseEntity
{
public:
	virtual unsigned int BuildTransformations(const CStudioHdr* hdr, Vector3* positions, Quaternion* rotations, Vector3* scales, const matrix3x4_t& cameraTransform, const BoneList* boneList, CBoneBitList& boneComputed) = 0; // 205
	virtual void UpdateIKLocks(float currentTime) = 0; // 206
	virtual void CalculateIKLocks(float currentTime) = 0; // 207
private:
	virtual void UnknownEntity208() = 0;
public:
	virtual const matrix3x4_t* GetHandIKOffset() const = 0; // 209
	virtual void DoAnimationEventsHdr(const CStudioHdr* hdr) = 0; // 210
	virtual void FireEvent(const Vector3& origin, const QAngle& angles, int event, const char* options, const float* cycleTime, const int* animationLayerIndex) = 0; // 211
	virtual void AnimEventScriptCallback(const char* name) = 0; // 212
	virtual void StandardBlendingRules(const CStudioHdr* hdr, Vector3* positions, Quaternion* rotations, Vector3* scales, float currentTime, const BoneList* boneList) = 0; // 213
	virtual void AccumulateLayers(IBoneSetup& boneSetup, Vector3* positions, Quaternion* rotations, Vector3* scales, float currentTime) = 0; // 214
	virtual Vector3 GetThirdPersonViewPosition() = 0; // 215
	virtual bool IsClientRagdoll() const = 0; // 216
	virtual C_BaseAnimating* BecomeRagdollOnClient() = 0; // 217
	virtual C_ClientRagdoll* CreateClientRagdoll(bool restoring) = 0; // 218
	virtual void SaveRagdollInfo(int boneCount, const matrix3x4_t& parentTransform, C_BoneAccessor& boneAccessor) = 0; // 219
	virtual bool RetrieveRagdollInfo(Vector3* positions, Quaternion* rotations) = 0; // 220
	virtual void GetRagdollInitBoneArrays(matrix3x4_t* previousBones, matrix3x4_t* nextBones, matrix3x4_t* currentBones, float boneDt) = 0; // 221
	virtual void ClearAnimationSoundsAndEffects(const CStudioHdr* hdr, int sequence, float cycleBegin) = 0; // 222
	virtual void StudioFrameAdvance() = 0; // 223
	virtual void UpdateClientSideAnimation() = 0; // 224
	virtual unsigned int ComputeClientSideAnimationFlags() = 0; // 225
	// Retail byte this+0x1020; matching typed Apex m_bSequenceFinished getter.
	virtual bool IsActivityFinished() = 0; // 226
	virtual bool ForbidThreadedBoneSetup() const = 0; // 227
private:
	virtual void UnknownEntity228() = 0;
public:
	virtual C_BaseAnimating* GetBoneSetupDependency() = 0; // 229
	virtual void ClearPredictedAnimEvents() = 0; // 230
	virtual void ExecPredictedAnimEvent(int event, const char* options) = 0; // 231
	virtual void OnScriptAnimStart(int sequence) = 0; // 232
	virtual bool Weapon_ShouldSmartAmmoLockOn(C_BaseEntity* target, C_WeaponX* weapon, int lockType) = 0; // 233
	virtual void FormatViewModelAttachment(int attachment, matrix3x4_t& matrix) = 0; // 234
	virtual bool IsMenuModel() const = 0; // 235
	// Retail calls renderable SetupBones(nullptr,-1,0x200 or 0x300,curtime); matching typed Apex CalcAttachments.
	virtual bool CalcAttachments() = 0; // 236
private:
	virtual void UnknownEntity237() = 0;
public:
	virtual bool UpdateBlending(int flags, const RenderableInstance_t& instance) = 0; // 238
};

static_assert(sizeof(C_BaseAnimating) == 0x30);

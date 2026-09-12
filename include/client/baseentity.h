#pragma once

#include "client/icliententity.h"

#include "client/iclientmodelrenderable.h"


#include "engine/ICollideable.h"
#include "gametrace.h"

class ISave;
class IRestore;
class C_WeaponX;
class C_BaseCombatWeapon;
class C_TitanSoul;
class CParticleEffect;
struct ScriptClassDesc_t;
struct Quaternion;
struct ClientInterpolationSnapshot;
struct VisibleToLocalPlayerTrace;

struct datamap_t;
class IPhysicsObject;
class C_BaseAnimating;
class C_BaseAnimatingOverlay;
class C_BaseCombatCharacter;
class C_Player;
class C_RopeKeyframe;
class C_ScriptProp;
class C_Beam;
class C_DynamicProp;

class C_BaseEntity : public IClientEntity, public IClientModelRenderable
{
public:
	virtual datamap_t* GetDataDescMap() = 0; // 13
	virtual int YouForgotToImplementOrDeclareClientClass() = 0; // 14
	virtual datamap_t* GetPredDescMap() = 0; // 15
	virtual const datamap_t* GetPredDescMapConst() const = 0; // 16
	virtual ScriptClassDesc_t* GetScriptDesc() = 0; // 17
private:
	virtual void UnknownEntity018() = 0;
	virtual void UnknownEntity019() = 0;
	virtual void UnknownEntity020() = 0;
public:
	virtual int GetTracerAttachment() = 0; // 21
	virtual C_BaseEntity* GetTracerEntity() = 0; // 22
	virtual const char* GetTracerType() const = 0; // 23
	virtual int GetArmorType() const = 0; // 24
private:
	virtual void UnknownEntity025() = 0;
public:
	virtual void Spawn() = 0; // 26
	virtual void SpawnClientEntity() = 0; // 27
	virtual void Precache() = 0; // 28
	virtual void Activate() = 0; // 29
private:
	virtual void UnknownEntity030() = 0;
public:
	virtual bool KeyValue(const char* key, const char* value) = 0; // 31
	virtual bool GetKeyValue(const char* key, char* value, int maxLength) = 0; // 32
	virtual int GetSpawnCounter() const = 0; // 33
	virtual bool Entity_Init(int entityIndex, int serialNumber, bool clientOnly) = 0; // 34
	virtual const C_BaseAnimating* GetBaseAnimatingConst() const = 0; // 35
	virtual C_BaseAnimating* GetBaseAnimating() = 0; // 36
	virtual const C_BaseAnimatingOverlay* GetBaseAnimatingOverlayConst() const = 0; // 37
	virtual C_BaseAnimatingOverlay* GetBaseAnimatingOverlay() = 0; // 38
	virtual C_RopeKeyframe* GetRopeKeyframe() = 0; // 39
	virtual void SetClassname(const char* classname) = 0; // 40
	virtual bool IsFirstPersonProxy() = 0; // 41
	virtual bool IsViewModel() = 0; // 42
	virtual bool SkipsAnimationData() = 0; // 43
	virtual float GetIKHeightOffset() const = 0; // 44
	virtual int Classify() const = 0; // 45
	virtual void ReleaseNoDeleteSelf() = 0; // 46
	virtual void CalcRenderOriginAndAngles(Vector3& origin, QAngle& angles) = 0; // 47
	virtual bool IsTransparentPrimary() = 0; // 48
	virtual bool TestCollision(const Ray_t& ray, unsigned int contentsMask, trace_t& trace) = 0; // 49
	virtual bool TestHitboxes(const Ray_t& ray, unsigned int contentsMask, trace_t& trace) = 0; // 50
private:
	virtual void UnknownEntity051() = 0;
	virtual void UnknownEntity052() = 0;
public:
	virtual C_BaseEntity* GetViewModelOwner() = 0; // 53
	virtual float GetAttackDamageScale() const = 0; // 54
	virtual void MoveFieldsIntoLerpingStruct(float time) = 0; // 55
	virtual void PostDataUpdateThreaded(DataUpdateType_t updateType) = 0; // 56
	virtual void SetDormant(bool dormant) = 0; // 57
	virtual bool ShouldSavePhysics() = 0; // 58
	virtual void OnSave() = 0; // 59
	virtual void OnRestore() = 0; // 60
	virtual int ObjectCaps() = 0; // 61
	virtual int Save(ISave& save) = 0; // 62
	virtual int Restore(IRestore& restore) = 0; // 63
	virtual bool CreateVPhysics() = 0; // 64
	virtual void VPhysicsDestroyObject() = 0; // 65
	virtual void VPhysicsUpdate(IPhysicsObject* physicsObject) = 0; // 66
	virtual void VPhysicsShadowUpdate(IPhysicsObject* physicsObject) = 0; // 67
	virtual int VPhysicsGetObjectList(IPhysicsObject** list, int maxCount) = 0; // 68
	virtual bool SetupBones_MayUseFootIk() = 0; // 69
private:
	virtual void UnknownEntity070() = 0;
public:
	virtual const Vector3& GetPrevAbsOrigin() const = 0; // 71
	virtual void Teleport(const Vector3* position, const QAngle* angles, const Vector3* velocity) = 0; // 72
	virtual void PreNewModel() = 0; // 73
	virtual void PostNewModel() = 0; // 74
	virtual const Vector3& GetBoundingMins() const = 0; // 75
	virtual const Vector3& GetBoundingMaxs() const = 0; // 76
	virtual Vector3 Script_GetBoundingMins() const = 0; // 77
	virtual Vector3 Script_GetBoundingMaxs() const = 0; // 78
	virtual const Vector3& WorldSpaceCenter() const = 0; // 79
	virtual void ComputeWorldSpaceSurroundingBox(Vector3* mins, Vector3* maxs) = 0; // 80
	virtual void GetVectors(Vector3* forward, Vector3* right, Vector3* up) const = 0; // 81
	virtual SolidType_t GetSolid() const = 0; // 82
	virtual int GetSolidFlags() const = 0; // 83
	virtual bool GetAttachmentOrigin(int attachment, Vector3& origin) = 0; // 84
	virtual bool GetAttachmentVelocity(int attachment, Vector3& originVelocity, Quaternion& angleVelocity) = 0; // 85
	virtual bool GetHitboxAttachmentVelocity(int hitbox, Vector3& originVelocity, Quaternion& angleVelocity) = 0; // 86
	virtual bool GetHitboxAttachment(int hitbox, Vector3& origin, QAngle& angles) = 0; // 87
	virtual void InvalidateAttachments() = 0; // 88
	virtual int GetTeamNumber() const = 0; // 89
	virtual void ChangeTeam(int teamNumber) = 0; // 90
private:
	virtual void UnknownEntity091() = 0;
	virtual void UnknownEntity092() = 0;
public:
	virtual bool CastsShadows() = 0; // 93
	virtual void UpdatePartitionListEntry() = 0; // 94
	virtual bool InitializeAsClientEntity(const char* modelName, bool renderWithViewModels) = 0; // 95
	virtual bool Simulate() = 0; // 96
	virtual bool CanStandOn(C_BaseEntity* surface) const = 0; // 97
	virtual void GetAimEntOrigin(IClientEntity* entity, Vector3* origin, QAngle* angles) = 0; // 98
	virtual bool ShouldRenderAlways() const = 0; // 99
	virtual int ComputeTranslucencyType() = 0; // 100
	virtual bool ComputeSortPriority() = 0; // 101
	virtual int GetCollideType() = 0; // 102
	virtual void UpdateVisibility() = 0; // 103
	virtual bool ShouldSuppressForSplitScreenPlayer(int splitScreenSlot) = 0; // 104
	virtual bool IsImmuneToPhaseShiftInvisibility(C_Player* viewPlayer) const = 0; // 105
	virtual bool IsSelfAnimating() = 0; // 106
	virtual void OnNewParticleEffect(const char* name, CParticleEffect* effect) = 0; // 107
	virtual void OnParticleEffectDeleted(CParticleEffect* effect) = 0; // 108
	virtual void ResetIK() = 0; // 109
	virtual int InterpolateFieldsInternal(float currentTime, const ClientInterpolationSnapshot* secondSnapshot, float secondSnapshotTime) = 0; // 110
private:
	virtual void UnknownEntity111() = 0;
public:
	virtual bool DidEntityTeleport() = 0; // 112
	virtual bool IsSubModel() = 0; // 113
	virtual int DrawBrushModel(const CViewSetup* view, int flags) = 0; // 114
	virtual float GetTextureAnimationStartTime() = 0; // 115
	virtual void TextureAnimationWrapped() = 0; // 116
	virtual void SetNextClientThink(float nextThinkTime) = 0; // 117
	virtual void SetHealth(int health) = 0; // 118
	virtual C_TitanSoul* GetTitanSoul() const = 0; // 119
	virtual C_BaseCombatCharacter* TitanSoul_GetTitan() = 0; // 120
	virtual bool IsPredictedProjectile() const = 0; // 121
	virtual void UpdateOnRemove() = 0; // 122
	virtual void SUB_Remove() = 0; // 123
	virtual CBaseHandle GetEntForDecal() = 0; // 124
	virtual C_Player* GetPredictionOwner() = 0; // 125
	virtual void InitPredictable(C_Player* owner, const char* context) = 0; // 126
	virtual void SetPredictable(bool predictable) = 0; // 127
	virtual bool PredictionErrorShouldResetLatchedForAllPredictables() = 0; // 128
	virtual bool PredictionIsPhysicallySimulated() = 0; // 129
	virtual void FindNonWorldTimeEntities() = 0; // 130
	virtual void OnPostRestoreData() = 0; // 131
	virtual bool IsMechanical() const = 0; // 132
	virtual bool HasGibModel() const = 0; // 133
	virtual void DispatchImpactEffects(C_BaseEntity* hitEntity, const Vector3& start, const Vector3& end, const Vector3& normal, short surfaceProp, int staticPropId, int damageType, int impactEffectTable, C_BaseEntity* otherEntity, unsigned int impactEffectFlags) = 0; // 134
	virtual bool ShouldPredict() = 0; // 135
	virtual void Think() = 0; // 136
	virtual bool PreRender() = 0; // 137
	virtual const char* GetClassname() const = 0; // 138
	virtual bool IsCurrentlyTouching() const = 0; // 139
	virtual void StartTouch(C_BaseEntity* other) = 0; // 140
	virtual void Touch(C_BaseEntity* other, const trace_t* trace) = 0; // 141
	virtual void EndTouch(C_BaseEntity* other) = 0; // 142
	virtual unsigned int PhysicsSolidMaskForEntity() const = 0; // 143
	virtual void ResolveFlyCollisionCustom(trace_t& trace, Vector3& velocity) = 0; // 144
	virtual void PhysicsSimulate() = 0; // 145
	virtual bool IsFuncBrush() const = 0; // 146
	virtual bool IsMoving() = 0; // 147
	virtual bool ShouldRegenerateOriginFromCellBits() const = 0; // 148
	virtual bool IsPlayer() = 0; // 149
	virtual bool IsTitan() const = 0; // 150
	virtual bool IsTitanSoul() = 0; // 151
	virtual bool IsOperator() const = 0; // 152
	virtual bool IsHuman() const = 0; // 153
	virtual bool IsZipline() const = 0; // 154
	virtual bool IsBreakableGlass() = 0; // 155
	virtual bool IsHologram() const = 0; // 156
	virtual bool IsPlayerDecoy() = 0; // 157
	virtual bool IsBaseCombatCharacter() = 0; // 158
	virtual bool IsGrenade() = 0; // 159
	virtual C_BaseCombatCharacter* MyCombatCharacterPointer() = 0; // 160
	virtual C_Player* MyPlayerPointer() = 0; // 161
	virtual C_ScriptProp* MyScriptPropPointer() = 0; // 162
	virtual C_Beam* MyBeamPointer() = 0; // 163
	virtual bool IsNPC() = 0; // 164
	virtual bool IsPointCamera() = 0; // 165
	virtual bool IsTurret() = 0; // 166
	virtual bool DisallowsGrapple() = 0; // 167
	virtual C_DynamicProp* MyDynamicPropPointer() = 0; // 168
	virtual bool IsSprite() = 0; // 169
	virtual bool IsProp() = 0; // 170
	virtual bool IsWeaponX() const = 0; // 171
	virtual bool IsCombatWeaponClone() = 0; // 172
	virtual const C_WeaponX* MyWeaponXPointerConst() const = 0; // 173
	virtual C_WeaponX* MyWeaponXPointer() = 0; // 174
	virtual const C_BaseCombatWeapon* MyCombatWeaponPointerConst() const = 0; // 175
	virtual C_BaseCombatWeapon* MyCombatWeaponPointer() = 0; // 176
private:
	virtual void UnknownEntity177() = 0;
public:
	virtual bool IsProjectile() = 0; // 178
	virtual bool IsGrapplingHook() = 0; // 179
	virtual Vector3 EyePosition() = 0; // 180
	virtual Vector3 PositionBetweenEyes() = 0; // 181
	virtual QAngle EyeAngles() = 0; // 182
	virtual QAngle LocalEyeAngles() = 0; // 183
	virtual Vector3 EarPosition() = 0; // 184
	virtual bool ShouldCollideByVelocity(const Vector3& velocity) const = 0; // 185
	virtual bool ShouldCollide(const C_BaseEntity* other, int collisionGroup, int contentsMask) const = 0; // 186
	virtual float GetGravity() const = 0; // 187
	virtual void GetGroundVelocityToApply(Vector3& velocity) = 0; // 188
	virtual bool ShouldInterpolate() = 0; // 189
	virtual void OnMoveParentRendered() = 0; // 190
	virtual bool IsCloaked(bool includeFade) = 0; // 191
	virtual bool DoesInheritParentCloak() = 0; // 192
	virtual bool ShouldPushPhasedEntities() const = 0; // 193
	virtual bool IsPhaseShifted() const = 0; // 194
	virtual const char* GetTitleForUI() const = 0; // 195
	virtual bool OnPredictedEntityRemove(bool isBeingRemoved, C_BaseEntity* predicted) = 0; // 196
private:
	virtual void UnknownEntity197() = 0;
	virtual void UnknownEntity198() = 0;
	virtual void UnknownEntity199() = 0;
public:
	virtual int GetStudioBody() const = 0; // 200
	virtual void OnPositionChanged() = 0; // 201
	virtual void PerformCustomPhysics(Vector3* position, Vector3* velocity, QAngle* angles, QAngle* angularVelocity) = 0; // 202
	virtual bool CollisionExplodesMissiles() const = 0; // 203
	virtual VisibleToLocalPlayerTrace* GetVisibileToPlayerLocalTrace() = 0; // 204
};

static_assert(sizeof(C_BaseEntity) == 0x30);

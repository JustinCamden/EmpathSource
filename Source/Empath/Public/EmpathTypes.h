// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EmpathTypes.generated.h"

class AEmpathHandActor;
class USoundBase;
class UAkAudioEvent;
class UCurveFloat;
class UEmpathSpeakerComponent;
class AEmpathEnemySpawner;
class AEmpathCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeDilationEndDelegate, uint8, RequestID, bool, bAborted);

// Collision definitions
#define ECC_Damage	ECC_GameTraceChannel1
#define ECC_Teleport	ECC_GameTraceChannel2

/**
 Holder for all the custom data types and names used by the engine
 */

UENUM(BlueprintType)
enum class EEmpathTeam :uint8
{
	Neutral,
	Player,
	Enemy
};

struct FEmpathBBKeys
{
	static const FName AttackTarget;
	static const FName bCanSeeTarget;
	static const FName bAlert;
	static const FName GoalLocation;
	static const FName BehaviorMode;
	static const FName bIsPassive;
	static const FName DefendTarget;
	static const FName DefendGuardRadius;
	static const FName DefendPursuitRadius;
	static const FName FleeTarget;
	static const FName FleeTargetRadius;
	static const FName NavRecoveryDestination;
	static const FName NavRecoverySearchInnerRadius;
	static const FName NavRecoverySearchOuterRadius;
};

struct FEmpathCollisionProfiles
{
	static const FName Ragdoll;
	static const FName PawnIgnoreAll;
	static const FName DamageCollision;
	static const FName HandCollision;
	static const FName NoCollision;
	static const FName GripCollision;
	static const FName OverlapAllDynamic;
	static const FName PawnOverlapOnlyTrigger;
	static const FName PlayerRoot;
	static const FName HandCollisionClimb;
	static const FName Projectile;
	static const FName ProjectileSensor;
	static const FName EmpathTrigger;
};

USTRUCT(BlueprintType)
struct FSecondaryAttackTarget
{
	GENERATED_USTRUCT_BODY()

public:
	FSecondaryAttackTarget() :
		TargetActor(nullptr),
		TargetPreference(0.5f),
		TargetingRatio(0.3f),
		TargetRadius(0.0f)
	{}

	/**
	* @param TargetPreference	Determines how likely AI is to target this actor. Player is 0.f. Values >0 will emphasize targeting this actor, <0 will de-emphasize.
	*							Preference is one of several factors in target choice.
	* @param TargetingRatio		What (approx) percentage of AI should attack this target.
	* @param TargetRadius		Radius of the target, used to adjust various attack distances and such. Player is assumed to be radius 0.
	*/
	UPROPERTY(Category = "Empath|AI", EditAnywhere, BlueprintReadWrite)
	AActor* TargetActor;

	UPROPERTY(Category = "Empath|AI", EditAnywhere, BlueprintReadWrite)
	float TargetPreference;

	UPROPERTY(Category = "Empath|AI", EditAnywhere, BlueprintReadWrite)
	float TargetingRatio;

	UPROPERTY(Category = "Empath|AI", EditAnywhere, BlueprintReadWrite)
	float TargetRadius;

	bool IsValid() const;
};

UENUM(BlueprintType)
enum class EEmpathPlayerAwarenessState : uint8
{
	KnownLocation,
	PotentiallyLost,
	Lost,
	Searching,
	PresenceNotKnown,
};

UENUM(BlueprintType)
enum class EEmpathBehaviorMode : uint8
{
	SearchAndDestroy,
	Defend,
	Flee
};

USTRUCT(BlueprintType)
struct FEmpathPerBoneDamageScale
{
	GENERATED_USTRUCT_BODY()

	/**
	* @param BoneNames			Which bones should have this damage scale.
	* @param DamageScale		The value by which damage to the affected bones is multiplied.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Empath|PerBoneDamage")
	TSet<FName> BoneNames;

	UPROPERTY(EditDefaultsOnly, Category = "Empath|PerBoneDamage")
	float DamageScale;

	FEmpathPerBoneDamageScale() :
		DamageScale(1.0f)
	{}
};

USTRUCT(BlueprintType)
struct FEmpathDamageHistoryEvent
{
	GENERATED_USTRUCT_BODY()

public:
	/**
	* @param DamageAmount		The amount of damage received.
	* @param EventTimestamp		The time the damage was received.
	*/
	UPROPERTY(EditDefaultsOnly, Category = "Empath|DamageHistory")
	float DamageAmount;
	UPROPERTY(EditDefaultsOnly, Category = "Empath|DamageHistory")
	float EventTimestamp;

	FEmpathDamageHistoryEvent(float InDamageAmount = 0.0f, float InEventTimestamp = 0.0f)
		: DamageAmount(InDamageAmount),
		EventTimestamp(InEventTimestamp)
	{}
};

UENUM(BlueprintType)
enum class EEmpathCharacterPhysicsState : uint8
{
	/** No physics. */
	Kinematic,

	/** Fully ragdolled. */
	FullRagdoll,

	/** Physical hit reactions */
	HitReact,

	/** Can be hit by the player -- should only on when very close to player */
	PlayerHittable,

	/** Getting up from ragdoll */
	GettingUp,
};

USTRUCT(BlueprintType)
struct FEmpathCharPhysicsStateSettings
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, Category = "Empath|Physics")
	bool bSimulatePhysics;

	UPROPERTY(EditAnywhere, Category = "Empath|Physics")
	bool bEnableGravity;

	UPROPERTY(EditAnywhere, Category = "Empath|Physics")
	FName PhysicalAnimationProfileName;

	UPROPERTY(EditAnywhere, Category = "Empath|Physics")
	FName PhysicalAnimationBodyName;

	UPROPERTY(EditAnywhere, Category = "Empath|Physics")
	FName SimulatePhysicsBodyBelowName;

	UPROPERTY(EditAnywhere, Category = "Empath|Physics")
	FName ConstraintProfileJointName;

	UPROPERTY(EditAnywhere, Category = "Empath|Physics")
	FName ConstraintProfileName;

	FEmpathCharPhysicsStateSettings()
		: bSimulatePhysics(false),
		bEnableGravity(false)
	{}
};

USTRUCT(BlueprintType)
struct FEmpathCharPhysicsStateSettingsEntry
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, Category = "Physics")
	EEmpathCharacterPhysicsState PhysicsState;

	UPROPERTY(EditAnywhere, Category = "Physics")
	FEmpathCharPhysicsStateSettings Settings;
};

struct FEmpathVelocityFrame
{
public:

	FVector Velocity;
	FVector AngularVelocity;
	FVector KAcceleration;
	float SphericalVelocity;
	float RadialVelocity;
	float VerticalVelocity;
	float SphericalAccel;
	float RadialAccel;
	float VerticalAccel;
	float FrameTimeStamp;

	FEmpathVelocityFrame(FVector InVelocity = FVector::ZeroVector,
		FVector InAngularVelocity = FVector::ZeroVector,
		FVector InAcceleration = FVector::ZeroVector,
		float InSphericalVelocity = 0.0f,
		float InRadialVelocity = 0.0f,
		float InVerticalVelocity = 0.0f,
		float InSphericalAccel = 0.0f,
		float InRadialAccel = 0.0f,
		float InVerticalAccel = 0.0f,
		float InEventTimestamp = 0.0f)
		: Velocity(InVelocity),
		AngularVelocity(InAngularVelocity),
		KAcceleration(InAcceleration),
		SphericalVelocity(InSphericalVelocity),
		RadialVelocity(InRadialVelocity),
		VerticalVelocity(InVerticalVelocity),
		SphericalAccel(InSphericalAccel),
		RadialAccel(InRadialAccel),
		VerticalAccel(InVerticalAccel),
		FrameTimeStamp(InEventTimestamp)
	{}
};

namespace EmpathNavAreaFlags
{
	const int16 Navigable = (1 << 1);		// this one is defined by the system
	const int16 Climb = (1 << 2);
	const int16 Jump = (1 << 3);
	const int16 Fly = (1 << 4);
};

UENUM(BlueprintType)
enum class EEmpathNavRecoveryAbility : uint8
{
	/** Can recover if starting off nav mesh. Recovered if nav mesh is below us. Generally won't handle "islands" of nav mesh. */
	OffNavMesh,

	/** Can recover when pathing fails while on valid nav mesh. Recovers when a path exists to the recovery destination. Handles "islands" but assumes destination is off the island (was found requiring no path). Implies ability to recover from off nav mesh. */
	OnNavMeshIsland,
};


USTRUCT(BlueprintType)
struct FEmpathNavRecoverySettings
{
	GENERATED_USTRUCT_BODY();

	/** How many pathing failures until we try to regain navmesh? Ignored if <= 0. However if we failed while off navmesh, we'll immediately start recovery. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 FailureCountUntilRecovery;

	/** How much time after the first pathing failure until we try to regain navmesh on subsequent failures? Ignored if <= 0 or if FailureCountUntilRecovery > 0. However if we failed while off navmesh, we'll immediately start recovery. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FailureTimeUntilRecovery;

	/** How much time since starting recovery to allow until we trigger the failure event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxRecoveryAttemptTime;

	/** The inner radius within which to search for a navmesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SearchInnerRadius;

	/** The outer radius within which to search for a navmesh*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SearchOuterRadius;

	/** How fast the inner search radius should expand*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SearchRadiusGrowthRateInner;

	/** How fast the outer search radius should expand*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SearchRadiusGrowthRateOuter;

	/** Number of frames to wait between recovery attempts. 0=no delay, 1=try every other frame, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (UIMin = 0, ClampMin = 0))
	int32 NavRecoverySkipFrames;

	FEmpathNavRecoverySettings()
		:FailureCountUntilRecovery(0),
		FailureTimeUntilRecovery(4.0f),
		MaxRecoveryAttemptTime(5.0f),
		SearchInnerRadius(100.0f),
		SearchOuterRadius(300.0f),
		SearchRadiusGrowthRateInner(200.0f),
		SearchRadiusGrowthRateOuter(500.0f),
		NavRecoverySkipFrames(1)
	{}
};

UENUM(BlueprintType)
enum class EEmpathHand : uint8
{
	Left,
	Right,
	Both
};

UENUM(BlueprintType)
enum class EEmpathBinaryHand : uint8
{
	Left,
	Right
};

UENUM(BlueprintType)
enum class EEmpathTeleportState : uint8
{
	NotTeleporting,
	TracingTeleportLocation,
	TracingTeleportLocAndRot,
	TeleportingToLocation,
	TeleportingToRotation,
	EndingTeleport
};

USTRUCT(BlueprintType)
struct FEmpathTeleportTraceSettings
{
	GENERATED_USTRUCT_BODY();

	/** Whether we should trace for Teleport Beacons. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTraceForBeacons;

	/** Whether we should trace for Empath Characters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTraceForEmpathChars;

	/** Whether we should trace for World Static Actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTraceForWorldStatic;

	/** Whether to enforce using the min teleport distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSnapToMinDistance;

	/** Whether to check for walls that we can climb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bTraceForWallClimb;

	FEmpathTeleportTraceSettings()
		:bTraceForBeacons(true),
		bTraceForEmpathChars(true),
		bTraceForWorldStatic(true),
		bSnapToMinDistance(true),
		bTraceForWallClimb(true)
	{

	}
};

UENUM(BlueprintType)
enum class EEmpathOrientation : uint8
{
	Hand,
	Head
};

UENUM(BlueprintType)
enum class EEmpathLocomotionMode : uint8
{
	Dash,
	Walk
};

UENUM(BlueprintType)
enum class EEmpathLocomotionControlMode : uint8
{
	DefaultAndAltMovement,
	PressToWalkTapToDash,
	LeftWalkRightTeleport,
	LeftWalkRightTeleportHoldPivot
};

UENUM(BlueprintType)
enum class EEmpathGripType : uint8
{
	NoGrip,
	Grip,
	Move,
	Pickup,
	Climb
};

UENUM(BlueprintType)
enum class EEmpathVelocityCheckType : uint8
{
	BufferedAverage,
	LastFrameOnly
};

USTRUCT(BlueprintType)
struct FEmpathGestureCheck
{
	GENERATED_USTRUCT_BODY()

public:
	/** The velocity of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector CheckVelocity;

	/** The magnitude of the velocity of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float VelocityMagnitude;

	/** The angular velocity of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector AngularVelocity;

	/** The scaled angular velocity of the hand.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector ScaledAngularVelocity;

	/** The spherical velocity of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float SphericalVelocity;

	/** The radial velocity of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float RadialVelocity;

	/** The radial velocity of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float VerticalVelocity;

	/** The spherical distance of the hand from the owning player's center of mass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float SphericalDist;

	/** The radial distance of the hand from the owning player's center of mass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float RadialDist;

	/** The vertical distance of the hand from the owning player's center of mass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float VerticalDist;

	/** The magnitude of the total acceleration of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float AccelMagnitude;

	/** The acceleration of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector CheckAcceleration;

	/** The angular acceleration of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector AngularAcceleration;

	/** The spherical acceleration of hand away the owning player's center of mass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SphericalAccel;

	/** The radial acceleration of hand away the owning player's center of mass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float RadialAccel;

	/** The vertical acceleration of hand away the owning player's center of mass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float VerticalAccel;
	
	/** The motion angle of the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector MotionAngle;

	/** The distance between hands on the last frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DistBetweenHands;

	/** The interior (palm) angle to the other hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float InteriorAngleToOtherHand;
};

UENUM(BlueprintType)
enum class EEmpathActivationState : uint8
{
	Inactive,
	Activating,
	Active,
	Deactivating
};

UENUM(BlueprintType)
enum class EEmpathCastingPose : uint8
{
	NoPose,
	CannonShotStatic,
	CannonShotDynamic
};

UENUM(BlueprintType)
enum class EEmpathCastingPoseType : uint8
{
	CannonShot
};

UENUM(BlueprintType)
enum class EEmpathGestureType : uint8
{
	NoGesture,
	Punching,
	Slashing,
	CannonShotStatic,
	CannonShotDynamic
};

UENUM(BlueprintType)
enum class EEmpathOneHandGestureType : uint8
{
	Punching,
	Slashing
};

USTRUCT(BlueprintType)
struct FEmpathPlayerAttackTarget
{
	GENERATED_USTRUCT_BODY()

public:
	FEmpathPlayerAttackTarget(AActor* InTargetActor = nullptr, float InTargetPreference = 0.0f) 
		:TargetActor(InTargetActor),
		TargetPreference(InTargetPreference)
	{}

	/**
	* @param TargetPreference	Determines how likely the player is to target this actor. Values >0 will emphasize targeting this actor, <0 will de-emphasize.
	*							Preference is one of several factors in target choice.
	*/
	UPROPERTY(Category = "Empath|AI", EditAnywhere, BlueprintReadWrite)
		AActor* TargetActor;

	UPROPERTY(Category = "Empath|AI", EditAnywhere, BlueprintReadWrite)
		float TargetPreference;

	bool IsValid() const;
};

UENUM(BlueprintType)
enum class EEmpathFlammableType : uint8
{
	NotFlammable,
	FlammableStatic,
	FlammableDynamic,
};

UENUM(BlueprintType)
enum class EEmpathSlashGroundedState : uint8
{
	Airborne,
	GroundedHead,
	GroundedTail,
};

USTRUCT(BlueprintType)
struct FEmpathOneHandGestureState
{
	GENERATED_USTRUCT_BODY();
	/*
	* The current activation / deactivation state of the gesture. Be cautious about changing this value.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		EEmpathActivationState ActivationState;

	/*
	*	The current distance that the motion has traveled while satisfying the gesture activation conditions.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float GestureDistance;

	/*
	*	The last time we began entering the gesture.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float LastEntryStartTime;

	/*
	*	The last time we began exiting the gesture.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float LastExitStartTime;
};

USTRUCT(BlueprintType)
struct FEmpathTwoHandGestureState
{
	GENERATED_USTRUCT_BODY();

	/*
	* The current activation / deactivation state of the gesture. Be cautious about changing this value.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		EEmpathActivationState ActivationState;

	/*
	*	The last time we began entering the gesture.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float LastEntryStartTime;

	/*
	*	The last time we began exiting the gesture.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float LastExitStartTime;

	/*
	*	The current distance that left hand has traveled while satisfying the gesture activation conditions.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float GestureDistanceLeft;

	/*
	*	The current distance that left hand has traveled while satisfying the gesture activation conditions.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float GestureDistanceRight;
};

USTRUCT()
struct FEmpathTimeDilationRequest
{
	GENERATED_USTRUCT_BODY()

public:
	/** The remaining duration of the request. */
	float RemainingTime;

	/** The time dilation scale of the request. */
	float TimeScale;

	/** The ID of the request. */
	int32 RequestID;

	/** The delegate called when the request ends.  */
	FOnTimeDilationEndDelegate OnEndDelegate;
};

USTRUCT(BlueprintType)
struct FEmpathOneHandGestureConditionCheckDebug
{
	GENERATED_USTRUCT_BODY()

public:
	/** The gesture condition state at this frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FEmpathGestureCheck GestureConditionState;

	/** The total movement of the hand since we begun debugging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MovementSinceStart;

	/** The time since we began debugging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeSinceStart;
};

USTRUCT(BlueprintType)
struct FEmpathDialogueBlock
{
	GENERATED_USTRUCT_BODY()

public:
	/** The text to display. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
	FString Text;

	/** The AK audio event to play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAkAudioEvent* AkAudioEvent;

	/** The sound to play if there is no inputted AK audio event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundBase* AudioClip;

	/** The minimum period we should wait after the end of each line before clearing it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinDelayAfterEnd;

	/** If nonzero and the next dialogue is another speaker, that will begin after this delay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InterruptedAfterTime;
};

USTRUCT(BlueprintType)
struct FEmpathConversationSpeaker
{
	GENERATED_USTRUCT_BODY()

public:
	/* The name of the person who is speaking here.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SpeakerName;

	/** A reference to a Speaker component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UEmpathSpeakerComponent* SpeakerRef;
};

USTRUCT(BlueprintType)
struct FEmpathConversationBlock
{
	GENERATED_USTRUCT_BODY()

public:
	/** The index of the speaker in the inputted array who should speak this dialogue block. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SpeakerIDX;
	
	/** Dialogue block to play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FEmpathDialogueBlock DialogueBlock;
};

USTRUCT(BlueprintType)
struct FEmpathDamageCauserLog
{
	GENERATED_USTRUCT_BODY()

public:
	/** The causer of the damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* DamageCauser;

	/** The timestamp of when the damage event took place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageTimestamp;
};

UENUM(BlueprintType)
enum class EEmpathGestureConditionCheckType : uint8
{
	AutoSuccess,
	VelocityMagnitude,
	VelocityX,
	VelocityY,
	VelocityZ,
	AngularVelocityX,
	AngularVelocityY,
	AngularVelocityZ,
	ScaledAngularVelocityX,
	ScaledAngularVelocityY,
	ScaledAngularVelocityZ,
	SphericalVelocity,
	RadialVelocity,
	VerticalVelocity,
	SphericalDistance,
	RadialDistance,
	VerticalDistance,
	AccelerationMagnitude,
	AccelerationX,
	AccelerationY,
	AccelerationZ,
	AngularAccelerationX,
	AngularAccelerationY,
	AngularAccelerationZ,
	SphericalAcceleration,
	RadialAcceleration,
	VerticalAcceleration,
	MotionAngleX,
	MotionAngleY,
	MotionAngleZ,
	DistanceBetweenHands,
	InteriorAngleToOtherHand
};

UENUM(BlueprintType)
enum class EEmpathBinaryOperation : uint8
{
	GreaterThan,
	GreaterThanEqual,
	LessThan,
	LessThanEqual,
	Equal
};

USTRUCT(BlueprintType)
struct FEmpathGestureCondition
{
	GENERATED_USTRUCT_BODY()

public:
	/** The value to check. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EEmpathGestureConditionCheckType ConditionCheckType;

	/** Whether we should check with respect to a buffered average or this frame only. Only applies to velocity checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EEmpathVelocityCheckType VelocityCheckType;

	/** The operation type of the check. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EEmpathBinaryOperation CheckOperationType;

	/** The value to check against. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float ThresholdValue;

	/** Performs a binary operation based on the user settable enum. */
	const bool EvaluateBinary(float Value, FString& OutFailureMessage);

	/**	Returns whether the base conditions are met on the provided hand.*/
	const bool AreBaseConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck, FString& OutFailerMessage);

	/** Optional comment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString ConditionComment;

	/** Inverts the conditions to account for differences between the left and right hand. */
	void InvertHand();
};


USTRUCT(BlueprintType)
struct FEmpathGestureCondition04 : public FEmpathGestureCondition
{
	GENERATED_USTRUCT_BODY()

public:
	/** Strong sub-conditions. Each must pass for this condition check to pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FEmpathGestureCondition> StrongSubConditions;

	/** Weak sub-conditions. Only one must pass for this condition check to pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FEmpathGestureCondition> WeakSubConditions;

	/**	Returns the base conditions and sub conditions are met.*/
	const bool AreAllConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck, FString& OutFailerMessage);

	/** Inverts the sub conditions to account for differences between the left and right hand. */
	void InvertSubConditions();
};

USTRUCT(BlueprintType)
struct FEmpathGestureCondition03 : public FEmpathGestureCondition
{
	GENERATED_USTRUCT_BODY()

public:
	/** Strong sub-conditions. Each must pass for this condition check to pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FEmpathGestureCondition04> StrongSubConditions;

	/** Weak sub-conditions. Only one must pass for this condition check to pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FEmpathGestureCondition04> WeakSubConditions;

	/**	Returns the base conditions and sub conditions are met.*/
	const bool AreAllConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck, FString& OutFailerMessage);

	/** Inverts the sub conditions to account for differences between the left and right hand. */
	void InvertSubConditions();
};

USTRUCT(BlueprintType)
struct FEmpathGestureCondition02 : public FEmpathGestureCondition
{
	GENERATED_USTRUCT_BODY()

public:
	/** Strong sub-conditions. Each must pass for this condition check to pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FEmpathGestureCondition03> StrongSubConditions;

	/** Weak sub-conditions. Only one must pass for this condition check to pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FEmpathGestureCondition03> WeakSubConditions;

	/**	Returns the base conditions and sub conditions are met.*/
	const bool AreAllConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck, FString& OutFailerMessage);

	/** Inverts the sub conditions to account for differences between the left and right hand. */
	void InvertSubConditions();
};

USTRUCT(BlueprintType)
struct FEmpathGestureCondition01 : public FEmpathGestureCondition
{
	GENERATED_USTRUCT_BODY()

public:
	/** Strong sub-conditions. Each must pass for this condition check to pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FEmpathGestureCondition02> StrongSubConditions;

	/** Weak sub-conditions. Only one must pass for this condition check to pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FEmpathGestureCondition02> WeakSubConditions;

	/**	Returns the base conditions and sub conditions are met.*/
	const bool AreAllConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck, FString& OutFailerMessage);

	/** Inverts the sub conditions to account for differences between the left and right hand. */
	void InvertSubConditions();
};

USTRUCT(BlueprintType)
struct FEmpathOneHandGestureData
{
	FEmpathOneHandGestureData()
		:LastEntryFailureReason(""),
		LastSustainFailureReason("")
	{}
	GENERATED_USTRUCT_BODY();

	/**	Returns whether the inputted gesture condition checks pass the entry conditions. */
	const bool AreEntryConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck);

	/**	The conditions for entering the gesture. Assumes the right hand is provided. Automatically flipped for the left hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FEmpathGestureCondition01 EntryConditions;

	/** The last failure reason for entering this gesture. */
	UPROPERTY(BlueprintReadOnly)
	FString LastEntryFailureReason;

	/**	The minimum time that conditions must be satisfied before the gesture becomes active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinEntryTime;

	/**	The minimum distance that the motion must travel before the gesture becomes active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinEntryDistance;

	/**	The conditions for sustaining the gesture. Assumes the right hand is provided. Automatically flipped for the left hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FEmpathGestureCondition01 SustainConditions;

	/** The last failure reason for entering this gesture. */
	UPROPERTY(BlueprintReadOnly)
	FString LastSustainFailureReason;

	/**	Returns whether the inputted gesture condition checks pass the sustain conditions */
	const bool AreSustainConditionsMet(const FEmpathGestureCheck& GestureConditionCheck, const FEmpathGestureCheck& FrameConditionCheck);

	/**	The minimum time that at least one sustain check must fail before this gesture deactivates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinExitTime;

	/** The current state for the gesture on this hand. */
	UPROPERTY(BlueprintReadOnly)
	FEmpathOneHandGestureState GestureState;

	/** Optional comment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString GestureComment;

	/**	Inverts the gesture to account for differences between the left and right hand. */
	void InvertHand();
};

USTRUCT(BlueprintType)
struct FEmpathOneHandGestureDataTyped : public FEmpathOneHandGestureData
{
	GENERATED_USTRUCT_BODY();

	/**	The gesture this gesture should trigger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EEmpathOneHandGestureType GestureType;
};


USTRUCT(BlueprintType)
struct FEmpathTwoHandGestureData
{
	GENERATED_USTRUCT_BODY();

	FEmpathTwoHandGestureData()
		:LastRightHandEntryFailureReason(""),
		LastRightHandSustainFailureReason(""),
		LastLeftHandEntryFailureReason(""),
		LastLeftHandSustainFailureReason("")
	{}

	/**	Returns whether the inputted gesture condition checks pass the entry conditions. */
	const bool AreEntryConditionsMet(FEmpathGestureCheck& RightHandGestureCheck, 
		FEmpathGestureCheck& RightHandFrameGestureCheck,
		FEmpathGestureCheck& LeftHandGestureCheck,
		FEmpathGestureCheck& LeftHandFrameGestureCheck);

	/**	The conditions for entering the gesture. Assumes the right hand is provided. Automatically flipped for the left hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FEmpathGestureCondition01 RightHandEntryConditions;

	/**	The conditions for entering the gesture. Calculated by flipping the right hand entry conditions. */
	UPROPERTY(BlueprintReadWrite)
	FEmpathGestureCondition01 LeftHandEntryConditions;

	/** The last failure reason for entering this gesture on the right hand. */
	UPROPERTY(BlueprintReadOnly)
	FString LastRightHandEntryFailureReason;

	/** The last failure reason for entering this gesture on the left hand. */
	UPROPERTY(BlueprintReadOnly)
	FString LastLeftHandEntryFailureReason;

	/**	The minimum time that conditions must be satisfied before the gesture becomes active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinEntryTime;

	/**	The minimum distance that the motion must travel before the gesture becomes active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinEntryDistance;

	/**	The conditions for sustaining the gesture. Assumes the right hand is provided. Automatically flipped for the left hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FEmpathGestureCondition01 RightHandSustainConditions;

	/**	The conditions for sustaining the gesture. Calculated by flipping the right hand sustain conditions. */
	UPROPERTY(BlueprintReadWrite)
	FEmpathGestureCondition01 LeftHandSustainConditions;

	/** The last failure reason for sustaining this gesture on the right hand. */
	UPROPERTY(BlueprintReadOnly)
	FString LastRightHandSustainFailureReason;

	/** The last failure reason for sustaining this gesture on the left hand. */
	UPROPERTY(BlueprintReadOnly)
	FString LastLeftHandSustainFailureReason;

	/**	Returns whether the inputted gesture condition checks pass the sustain conditions */
	const bool AreSustainConditionsMet(FEmpathGestureCheck& RightHandGestureCheck,
		FEmpathGestureCheck& RightHandFrameGestureCheck,
		FEmpathGestureCheck& LeftHandGestureCheck,
		FEmpathGestureCheck& LeftHandFrameGestureCheck);

	/**	The minimum time that at least one sustain check must fail before this gesture deactivates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinExitTime;

	/**	Calculates the left hand conditions by flipping the right hand conditions. */
	void CalculateLeftHandConditions();

	/** The current state of the gesture on both hands. */
	UPROPERTY(BlueprintReadOnly)
	FEmpathTwoHandGestureState GestureState;
};

USTRUCT(BlueprintType)
struct FEmpathPoseData
{
	GENERATED_USTRUCT_BODY();

	/**	The pose state this gesture should trigger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEmpathCastingPoseType PoseType;

	/** The static pose data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FEmpathTwoHandGestureData StaticData;

	/** The dynamic pose data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FEmpathTwoHandGestureData DynamicData;

	/** Optional comment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FString PoseComment;

	/**	Calculates the left hand conditions by flipping the right hand conditions. */
	void CalculateLeftHandConditions();
};

USTRUCT(BlueprintType)
struct FEmpathGestureTransformCache
{
	GENERATED_USTRUCT_BODY();

	/**	The hand's location when beginning to enter this gesture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector LastEntryStartLocation;

	/**	The hand's rotation when beginning to enter this gesture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FRotator LastEntryStartRotation;

	/** The motion angle of the hand when beginning to enter this gesture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector LastEntryStartMotionAngle;

	/**	The hand's velocity when beginning to enter this gesture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector LastEntryStartVelocity;

	/**	The hand's location when beginning to exit this gesture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector LastExitStartLocation;

	/**	The hand's rotation when beginning to exit this gesture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FRotator LastExitStartRotation;

	/** The motion angle of the hand when beginning to exit this gesture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector LastExitStartMotionAngle;

	/**	The hand's velocity when beginning to exit this gesture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector LastExitStartVelocity;
};

USTRUCT(BlueprintType)
struct FEmpathTargetingDirection
{
	GENERATED_USTRUCT_BODY();

	FEmpathTargetingDirection(FVector InSearchDirection = FVector::ZeroVector,float InMaxAngle = 30.0f, float InAngleWeight = 1.0f)
		:SearchDirection(InSearchDirection),
		MaxAngle(InMaxAngle),
		AngleWeight(InAngleWeight)
	{}

	/**	The direction to search in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector SearchDirection;
	
	/**	The maximum angle of the query. Smaller angles are more preferable*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float MaxAngle;
	
	/**	The angle weight of the query, with higher being more preferable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float AngleWeight;

	/**	Optional curve for scoring by angle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UCurveFloat* AngleScoreCurve;
};

struct FEmpathTargetingCandidate
{
	FEmpathTargetingCandidate(AActor* InActor = nullptr,
		FVector InAimLocation = FVector::ZeroVector,
		FVector InDirectionToTarget = FVector::ZeroVector,
		USceneComponent* InAimLocationComponent = nullptr,
		float InDistance = 0.0f,
		float InAngle = 0.0f,
		float InTargetingPreference = 0.0f)
		:Actor(InActor),
		DirectionToTarget(InDirectionToTarget),
		AimLocation(InAimLocation),
		AimLocationComponent(InAimLocationComponent),
		Distance(InDistance),
		Angle(InAngle),
		TargetPreference(InTargetingPreference)
	{}

	/** The actor being targeted */
	AActor* Actor;

	/** The direction to the target. */
	FVector DirectionToTarget;

	/** The aim location. */
	FVector AimLocation;

	/** The aim location component. */
	USceneComponent* AimLocationComponent;

	/** The squared distance to this location. */
	float Distance;

	/** The best angle to this location. */
	float Angle;

	/** The targeting preference of this point, inherited from the attack target. */
	float TargetPreference;

};

struct FEmpathTargetingCandidatePred
{
	FEmpathTargetingCandidatePred(AActor* InActor = nullptr,
		FVector InAimLocation = FVector::ZeroVector,
		FVector InDirectionToTarget = FVector::ZeroVector,
		USceneComponent* InAimLocationComponent = nullptr,
		float InDistance = 0.0f,
		float InAngle = 0.0f,
		float InTargetingPreference = 0.0f,
		bool bInFailedAimPrediction = false)
		:Actor(InActor),
		DirectionToTarget(InDirectionToTarget),
		AimLocation(InAimLocation),
		AimLocationComponent(InAimLocationComponent),
		Distance(InDistance),
		Angle(InAngle),
		TargetPreference(InTargetingPreference),
		bFailedAimPrediction(bInFailedAimPrediction)
	{}

	/** The actor being targeted */
	AActor* Actor;

	/** The direction to the target. */
	FVector DirectionToTarget;

	/** The aim location. */
	FVector AimLocation;

	/** The aim location component. */
	USceneComponent* AimLocationComponent;

	/** The squared distance to this location. */
	float Distance;

	/** The best angle to this location. */
	float Angle;

	/** The targeting preference of this point, inherited from the attack target. */
	float TargetPreference;

	/** Whether we failed aim prediction. */
	bool bFailedAimPrediction;
};

struct FEmpathTargetingCandidateAdv
{
	FEmpathTargetingCandidateAdv(AActor* InActor = nullptr,
		USceneComponent* InAimLocationComp = nullptr,
		float InDistance = 0.0f,
		float InTotalWeightedAngles = 0.0f,
		float InBestAngle = 0.0f,
		float InTargetPreference = 0.0f,
		FVector InBestSearchDirection = FVector::ZeroVector,
		FVector InDirectionToTarget = FVector::ZeroVector)
		:Actor(InActor),
		AimLocationComponent(InAimLocationComp),
		Distance(InDistance),
		TotalWeightedAngles(InTotalWeightedAngles),
		BestAngle(InBestAngle),
		TargetPreference(InTargetPreference),
		BestSearchDirection(InBestSearchDirection),
		DirectionToTarget(InDirectionToTarget)
	{}

	/** The actor that owns this location. */
	AActor* Actor;

	/** The aim location component. */
	USceneComponent* AimLocationComponent;

	/** The squared distance to this location. */
	float Distance;

	/** The total angle score of this location. */
	float TotalWeightedAngles;

	/** The best angle to this location. */
	float BestAngle;

	/** The targeting preference of this point, inherited from the attack target. */
	float TargetPreference;

	/** The best search direction to this location. */
	FVector BestSearchDirection;

	/** The direction to the target. */
	FVector DirectionToTarget;
};

struct FEmpathTargetingCandidateAdvPred
{
	FEmpathTargetingCandidateAdvPred(AActor* InActor = nullptr,
		USceneComponent* InAimLocationComp = nullptr,
		float InDistance = 0.0f,
		float InTotalWeightedAngles = 0.0f,
		float InBestAngle = 0.0f,
		float InTargetPreference = 0.0f,
		FVector InBestSearchDirection = FVector::ZeroVector,
		FVector InDirectionToTarget = FVector::ZeroVector,
		bool bInFailedAimPrediction = false)
		:Actor(InActor),
		AimLocationComponent(InAimLocationComp),
		Distance(InDistance),
		TotalWeightedAngles(InTotalWeightedAngles),
		BestAngle(InBestAngle),
		TargetPreference(InTargetPreference),
		BestSearchDirection(InBestSearchDirection),
		DirectionToTarget(InDirectionToTarget),
		bFailedAimPrediction(bInFailedAimPrediction)
	{}

	/** The actor that owns this location. */
	AActor* Actor;

	/** The aim location component. */
	USceneComponent* AimLocationComponent;

	/** The squared distance to this location. */
	float Distance;

	/** The total angle score of this location. */
	float TotalWeightedAngles;

	/** The best angle to this location. */
	float BestAngle;

	/** The targeting preference of this point, inherited from the attack target. */
	float TargetPreference;

	/** The best search direction to this location. */
	FVector BestSearchDirection;

	/** The direction to the target. */
	FVector DirectionToTarget;

	/** Whether we failed aim prediction. */
	bool bFailedAimPrediction;
};

UENUM(BlueprintType)
enum class EEmpathTeleportTraceResult :uint8
{
	NotValid,
	Character,
	TeleportBeacon,
	WallClimb,
	Ground
};

UENUM(BlueprintType)
enum class EEmpathAudioAreas :uint8
{
	None,
	Alleyway,
	Clocktower,
	Marketplace,
	Rooftop,
	Theatre,
};
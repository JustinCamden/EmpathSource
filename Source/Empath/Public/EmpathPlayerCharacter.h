// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "EmpathTeamAgentInterface.h"
#include "EmpathAimLocationInterface.h"
#include "VRCharacter.h"
#include "EmpathPlayerCharacter.generated.h"

// Stat groups for UE Profiler
DECLARE_STATS_GROUP(TEXT("EmpathPlayerCharacter"), STATGROUP_EMPATH_PlayerCharacter, STATCAT_Advanced);

// Notify delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnPlayerCharacterTeleportDelegate, AActor*, TeleportingActor, FVector, Origin, FVector, Destination, FVector, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnPlayerCharacterDeathDelegate, FHitResult const&, KillingHitInfo, FVector, KillingHitImpulseDir, const AController*, DeathInstigator, const AActor*, DeathCauser, const UDamageType*, DeathDamageType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerCharacterStunnedDelegate, const AController*, StunInstigator, const AActor*, StunCauser, const float, StunDuration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGestureStateChangedDelegate, const EEmpathGestureType, OldGestureState, const EEmpathGestureType, NewGestureState, const EEmpathBinaryHand, Hand);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastingPoseChangedDelegate, const EEmpathCastingPose, OldPose, const EEmpathCastingPose, NewPose);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNewChargedStateDelegate, const bool, bNewChargedState, const EEmpathBinaryHand, Hand);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNewBlockingStateDelegate, const bool, bNewBlockingState, const EEmpathBinaryHand, Hand);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerCharacterStunEndDelegate);

class AEmpathPlayerController;
class AEmpathHandActor;
class AEmpathTeleportBeacon;
class ANavigationData;
class AEmpathCharacter;
class AEmpathTeleportMarker;

/**
*
*/
UCLASS()
class EMPATH_API AEmpathPlayerCharacter : public AVRCharacter, public IEmpathTeamAgentInterface, public IEmpathAimLocationInterface
{
	GENERATED_BODY()
public:
	// ---------------------------------------------------------
	//	Misc 

	// Override for tick
	virtual void Tick(float DeltaTime) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	
	// Override for begin player
	virtual void BeginPlay() override;

	// Constructor to set default variables
	AEmpathPlayerCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Registers our input
	virtual void SetupPlayerInputComponent(class UInputComponent* NewInputComponent) override;

	// Pawn overrides for vr.
	virtual FVector GetPawnViewLocation() const override;
	virtual FRotator GetViewRotation() const override;

	// Override for Take Damage that calls our own custom Process Damage script (Since we can't override the OnAnyDamage event in c++)
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;


	// ---------------------------------------------------------
	//	Setters and getters

	/** Wrapper for GetDistanceTo function that takes into account that this is a VR character */
	UFUNCTION(Category = EmpathPlayerCharacter, BlueprintCallable, BlueprintPure)
	float GetDistanceToVR(AActor* OtherActor) const;

	/*
	* Gets the position of the player character at a specific height,
	* where 1 is their eye height, and 0 is their feet. 
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
		const FVector GetVRScaledHeightLocation(float HeightScale = 0.9f) const;

	/*
	* Gets the height of the player's head in local space.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	const float GetVREyeHeight() const;

	/** Gets this character, the teleport marker actor, the hand actors, and any gripped actors. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	TArray<AActor*> GetControlledActors();

	/** Returns whether this character is currently dead. */
	UFUNCTION(Category = "EmpathPlayerCharacter|Combat", BlueprintCallable, BlueprintPure)
	bool IsDead() const { return bDead; }

	/** Overridable function for whether the character can die.
	* By default, only true if not Invincible and not Dead. */
	UFUNCTION(Category = "EmpathPlayerCharacter|Combat", BlueprintCallable, BlueprintNativeEvent)
	bool CanDie() const;

	/** Whether the character is currently stunned. */
	UFUNCTION(Category = "EmpathPlayerCharacter|Combat", BlueprintCallable)
	bool IsStunned() const { return bStunned; }

	/** Overridable function for whether the character can be stunned.
	* By default, only true if IsStunnable, not Stunned, Dead, and more than
	* StunImmunityTimeAfterStunRecovery has passed since the last time we were stunned. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "EmpathPlayerCharacter|Combat")
	bool CanBeStunned() const;

	/** Returns whether the current character is in the process of teleporting. Returns false if they are pivoting */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathPlayerCharacter|Teleportation")
	bool IsTeleporting() const;

	/** Returns whether the current character can begin a pivot. 
	* By default, only true if PivotEnabled, not Stunned, not Dead, and not already teleporting. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "EmpathPlayerCharacter|Teleportation")
	bool CanPivot() const;

	/** Overridable function for whether the character can dash.
	* By default, only true if DashEnabled, not Stunned, not Dead, not Climbing, and not already teleporting. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "EmpathPlayerCharacter|Teleportation")
	bool CanDash() const;

	/** Overridable function for whether the character can teleport.
	* By default, only true if TeleportEnabled, not Stunned, not Dead, not already teleporting, and not manipulating or climbing with the teleport hand. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "EmpathPlayerCharacter|Teleportation")
	bool CanTeleport() const;

	/** Overridable function for whether the character can walk.
	* By default, only true if WalkEnabled, not Stunned, not Dead, not Climbing, and not already teleporting. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "EmpathPlayerCharacter|Locomotion")
	bool CanWalk() const;

	/** Overridable function for whether the player is moving. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "EmpathPlayerCharacter|Locomotion")
	bool IsMoving() const;

	/** Changes the teleport state */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Teleportation")
	void SetTeleportState(EEmpathTeleportState NewTeleportState);

	/** Overridable function for whether the character can climb.
	* By default, only true if ClimbEnabled, not Stunned, not Dead, and not Teleporting. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "EmpathPlayerCharacter|Climbing")
	bool CanClimb() const;

	// ---------------------------------------------------------
	//	Events and receives

	/** Called when the character dies or their health depletes to 0. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Combat", meta = (DisplayName = "Die"))
	void ReceiveDie(FHitResult const& KillingHitInfo, FVector KillingHitImpulseDir, const AController* DeathInstigator, const AActor* DeathCauser, const UDamageType* DeathDamageType);

	/** Called when the character dies or their health depletes to 0. */
	UPROPERTY(BlueprintAssignable, Category = "EmpathPlayerCharacter|Combat")
	FOnPlayerCharacterDeathDelegate OnDeath;

	/** Called this when this character teleports. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation", meta = (DisplayName = "On Teleport To Location"))
	void ReceiveTeleportToLocation(FVector Origin, FVector Destination, FVector Direction);

	/** Called when a teleport to location attempt times out. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnTeleportToLocationFail();

	/** Called when a teleport to location attempt finishes successfully. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnTeleportToLocationSuccess();

	/** Called when we begin teleporting to a new pivot. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnTeleportToRotation(const float DeltaYaw);

	/** Called when a teleport to rotation attempt finishes successfully. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnTeleportToRotationEnd();

	/** Called when a teleport attempt finishes, regardless of whether it was a success or a failure. */
	void OnTeleportEnd();

	/** Called when a teleport attempt finishes, regardless of whether it was a success or a failure. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Teleportation", meta = (DisplayName = "On Teleport End"))
	void ReceiveTeleportEnd();

	/** Processes and cleared a queued teleport to rotation. */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Teleportation")
	void ProcessQueuedTeleportToRot();

	/** Called whenever our teleport state is updated on Tick. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnTickTeleportStateUpdated(const EEmpathTeleportState TeleportState);

	/** Called whenever our teleport state is changed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnTeleportStateChanged(const EEmpathTeleportState OldTeleportState, const EEmpathTeleportState NewTeleportState);

	/** Called when character becomes stunned. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Combat", meta = (DisplayName = "Be Stunned"))
	void ReceiveStunned(const AController* StunInstigator, const AActor* StunCauser, const float StunDuration);

	/** Called when character becomes stunned. */
	UPROPERTY(BlueprintAssignable, Category = "EmpathPlayerCharacter|Combat")
	FOnPlayerCharacterStunnedDelegate OnStunned;

	/** Called when character stops being stunned. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Combat", meta = (DisplayName = "On Stun End"))
	void ReceiveStunEnd();

	/** Called when character stops being stunned. */
	UPROPERTY(BlueprintAssignable, Category = "EmpathPlayerCharacter|Combat")
	FOnPlayerCharacterStunEndDelegate OnStunEnd;

	/** Called when this character teleports to a location. */
	UPROPERTY(BlueprintAssignable, Category = "EmpathPlayerCharacter|Combat")
	FOnPlayerCharacterTeleportDelegate OnTeleportToLocation;

	/** Called when this character begins any sort of teleport. */
	void OnTeleport();

	/** Called when this character begins any sort of teleport. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Combat", meta = (DisplayName = "On Teleport"))
	void ReceiveTeleport();

	/** Called when the magnitude of the Locomotion input vector reaches the AxisEventThreshold. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Input")
	void OnLocomotionPressed();

	/** Called when the magnitude of the Locomotion input vector drops below the AxisEventThreshold. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Input")
	void OnLocomotionReleased();

	/** Called when the locomotion input vector is pressed and quickly released. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Input")
	void OnLocomotionTapped();

	/** Called when the magnitude of the Teleport input vector reaches the AxisEventThreshold. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Input")
	void OnTeleportPressed();

	/** Called when the magnitude of the Teleport input vector drops below the AxisEventThreshold. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Input")
	void OnTeleportReleased();

	/** Called when the teleport input vector is pressed and quickly released. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Input")
	void OnTeleportTapped();

	/** Called when the teleport input vector is held after being pressed. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Input")
	void OnTeleportHeld();

	/** Called when the Alt Movement key is pressed. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Alt Movement Pressed"))
	void ReceiveAltMovementPressed();

	/** Called when the Alt Movement key is released. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Alt Movement Released"))
	void ReceiveAltMovementReleased();

	/** Called when the Grip Right key is pressed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Grip Right Pressed"))
	void ReceiveGripRightPressed();

	/** Called when the Grip Right key is released. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Grip Right Released"))
	void ReceiveGripRightReleased();

	/** Called when the Grip Left key is pressed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Grip Left Pressed"))
	void ReceiveGripLeftPressed();

	/** Called when the Grip Left  key is released. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Grip Left Released"))
	void ReceiveGripLeftReleased();

	/** Called when the Click Right key is pressed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Click Right Pressed"))
	void ReceiveClickRightPressed();

	/** Called when the Click Right key is released. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Click Right Released"))
	void ReceiveClickRightReleased();

	/** Called when the Click Left key is pressed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Click Left Pressed"))
	void ReceiveClickLeftPressed();

	/** Called when the Click Left key is released. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Click Left Released"))
	void ReceiveClickLeftReleased();

	/** Called when the Charge Right key is pressed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Charge Right Pressed"))
		void ReceiveChargeRightPressed();

	/** Called when the Charge Right key is released. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Charge Right Released"))
		void ReceiveChargeRightReleased();

	/** Called when the Charge Left key is pressed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Charge Left Pressed"))
		void ReceiveChargeLeftPressed();

	/** Called when the Charge Left key is released. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input", meta = (DisplayName = "On Charge Left Released"))
		void ReceiveChargeLeftReleased();


	/** Called when the magnitude of the Dash input vector reaches the AxisEventThreshold. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input")
	void OnWalkBegin();

	/** Called when the magnitude of the Dash input vector drops below the AxisEventThreshold. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Input")
	void OnWalkEnd();

	/** Called after the hands have been registered (assuming valid hands were spawned and attached). */
	UFUNCTION(Category = EmpathPlayerCharacter, BlueprintImplementableEvent)
	void OnHandsRegistered();


	// ---------------------------------------------------------
	//	 Team Agent Interface

	/** Returns the team number of the actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathPlayerCharacter)
	EEmpathTeam GetTeamNum() const;


	// ---------------------------------------------------------
	//	Aim location

	/** Returns the location of the damage collision capsule. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathPlayerCharacter)
	bool GetCustomAimLocationOnActor(FVector LookOrigin, FVector LookDirection, FVector& OutAimlocation, USceneComponent*& OutAimLocationComponent) const;


	// ---------------------------------------------------------
	//	General Teleportation

	/** Whether this character can Dash in principle. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", EditAnywhere, BlueprintReadWrite)
	bool bDashEnabled;

	/** Whether this character can Teleport in principle. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", EditAnywhere, BlueprintReadWrite)
	bool bTeleportEnabled;

	/** Whether this character can Pivot in principle. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", EditAnywhere, BlueprintReadWrite)
	bool bPivotEnabled;

	/** Whether this character can Walk in principle. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Locomotion", EditAnywhere, BlueprintReadWrite)
	bool bWalkEnabled;

	/** The hand we use for teleportation origin. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", EditAnywhere, BlueprintReadWrite)
	EEmpathBinaryHand TeleportHand;

	/** Trace settings for Dashing. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", EditAnywhere, BlueprintReadWrite)
	FEmpathTeleportTraceSettings DashTraceSettings;

	/** Trace settings for Teleporting to a location. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", EditAnywhere, BlueprintReadWrite)
	FEmpathTeleportTraceSettings TeleportTraceSettings;

	/** Trace settings for Teleporting to a location and rotation. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", EditAnywhere, BlueprintReadWrite)
	FEmpathTeleportTraceSettings TeleportLocAndRotTraceSettings;

	/** Implementation of the teleport to location function. */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Teleportation")
	void TeleportToLocation(const FVector Destination);

	/** Implementation of the dash function. */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Teleportation")
	void DashInDirection(const FVector2D Direction);

	/** Implementation of the pivot rotation function. */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Teleportation")
	void TeleportToRotation(const float DeltaYaw, const float RotationSpeed);

	/** Caches the player navmesh for later use by teleportation. */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Teleportation")
	void CacheNavMesh();

	/** The the player's navmesh data. */
	UPROPERTY(BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	ANavigationData* PlayerNavData;

	/** The player's navigation filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	TSubclassOf<UNavigationQueryFilter> PlayerNavFilterClass;

	/** Returns the current teleport state of the character. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathPlayerCharacter|Teleportation")
	EEmpathTeleportState GetTeleportState() const { return TeleportState; }

	/** The target magnitude of teleportation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportMagnitude;

	/** The target magnitude of dashing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float DashMagnitude;

	/** The current teleport velocity. */
	UPROPERTY(BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	FVector TeleportCurrentVelocity;

	/** The target magnitude of teleportation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportRadius;

	/** How fast to lerp the teleport velocity from its current to target velocity. */
	float TeleportVelocityLerpSpeed;

	/** The minimum distance from a teleport beacon for it to be a valid teleport target. */ 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportBeaconMinDistance;

	/*
	* Traces a teleport location from the provided world origin in provided world direction, attempting to reach the provided magnitude. 
	* Returns whether the trace was successful
	* @param bInstantMagnitude Whether we should instantaneously reach the target magnitude (ie for dashing), or smoothly interpolate towards it.
	*/
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Teleportation")
	bool TraceTeleportLocation(FVector Origin, FVector Direction, float Magnitude, FEmpathTeleportTraceSettings& TraceSettings, bool bInterpolateMagnitude);

	/** Checks the trace impact point to see if it is a wall, and if so, whether it can be scaled. */
	bool IsWallClimbLocation(const FVector& ImpactPoint, const FVector& ImpactNormal, FVector& OutScalableLocation) const;

	/*
	* Traces teleport rotation.
	*/
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Teleportation")
	bool UpdateTeleportYaw(FVector Direction, float DeltaYaw);

	/** Object types to collide against when tracing for teleport locations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	TArray<TEnumAsByte<EObjectTypeQuery>> TeleportTraceObjectTypes;

	/** Spline positions from our last teleport trace. Used to draw the spline if we wish to. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	TArray<FVector> TeleportTraceSplinePositions;

	/** Checks to see if our current teleport position is valid. */
	EEmpathTeleportTraceResult ValidateTeleportTrace(FVector& OutFixedLocation, AEmpathTeleportBeacon* OutTeleportBeacon, AEmpathCharacter* OutTeleportCharacter, FHitResult TeleportHit, FVector TeleportOrigin, const FEmpathTeleportTraceSettings& TraceSettings) const;

	/** The teleport position currently targeted by a teleport trace. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	AEmpathTeleportBeacon* TargetedTeleportBeacon;

	/** Called when we target a beacon while tracing teleport. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnTeleportBeaconTargeted(AEmpathTeleportBeacon* NewTeleportBeacon);

	/** Called when we un-target a beacon while tracing teleport. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnTeleportBeaconUnTargeted(AEmpathTeleportBeacon* OldTeleportBeacon);

	/** The Empath Character currently targeted by a teleport trace. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	AEmpathCharacter* TargetedTeleportEmpathChar;

	/** Called when we target an Empath Character while tracing teleport. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnEmpathCharTargetedForTeleport(AEmpathCharacter* NewTargetedChar);

	/** Called when we un-target an Empath Character while tracing teleport. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnEmpathCharUntargetedForTeleport(AEmpathCharacter* OldTargetedChar);

	/** The extent of our query when projecting a point to navigation for teleportation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	FVector TeleportProjectQueryExtent;

	/** The radius of the character when querying a point above a wall for wall climbing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportWallClimbQueryRadius;

	/** The upwards reach of the character when checking for wall climb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportWallClimbReach;

	/** Our last updated teleport location. Zero vector if invalid. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	FVector TeleportTraceLocation;

	/** Our last updated teleport trace impact point. Zero vector if invalid. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	FVector TeleportTraceImpactPoint;

	/** The last normal of the wall climb location we were teleporting. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	FVector TeleportTraceImpactNormal;

	/** Our last updated teleport yaw rotation when tracing for rotation. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	FRotator TeleportYawRotation;

	/** The current teleport state of the character. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	EEmpathTeleportState TeleportState;

	/** Whether a teleport to rotation should occur after the end of this teleport to rotation. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	bool bTeleportToRotationQueued;

	/** The current rotational teleport speed of this actor. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportCurrentRotationSpeed;

	/** Used to project a point to the player's navigation mesh and check if it is on the ground. */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Teleportation")
	bool ProjectPointToPlayerNavigation(const FVector& Point, FVector& OutPoint) const;

	/** Sets whether the currently traced location is valid and calls events as appropriate. */
	void UpdateTeleportTraceState(const FHitResult& TeleportHitResult, const FVector& TeleportOrigin, const FEmpathTeleportTraceSettings& TraceSettings);

	/** Called when our teleport trace state changes. */
	void OnTeleportTraceResultChanged(const EEmpathTeleportTraceResult& NewTraceResult, const EEmpathTeleportTraceResult& OldTraceResult);

	/** Called when our teleport trace state changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerCharacter, meta = (DisplayName = "On Teleport Trace Result Changed"))
	void ReceiveTeleportTraceResultChanged(const EEmpathTeleportTraceResult& NewTraceResult, const EEmpathTeleportTraceResult& OldTraceResult);

	/** Called when a dash trace is successfully executed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnDashSuccess();

	/** Called when a dash trace fails to find a valid location. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnDashFail();

	/** The speed at which we move when teleporting and dashing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportMovementSpeed;

	/** The speed at which we rotate when pivoting left and right. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportRotationSpeed;

	/** The speed at which we rotate when turning 180 degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportRotation180Speed;

	/** The step by which we rotate when pivoting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportPivotStep;

	/** How long we wait after teleport before aborting the teleport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportTimoutTime;

	/** The maximum distance between the teleport goal and the actor position before we stop teleporting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportDistTolerance;

	/** The minimum distance when teleporting to location and rotation before we choose to only teleport to a rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportToLocationMinDist;

	/** The minimum delta between our current and target angle when teleporting to location and rotation before we choose to only teleport to a location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportRotationAndLocationMinDeltaAngle;

	/** The maximum remaining delta rotation before we stop rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportRotTolerance;

	/** The yaw to add to the character when teleporting to a new rotation. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportRemainingDeltaYaw;

	/** Whether the teleport trace is currently shown. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	bool bTeleportTraceVisible;

	/** Makes the teleport trace splines and reticle visible. */
	void ShowTeleportTrace();

	/** Called when we want to make teleport trace splines and reticle visible. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnShowTeleportTrace();

	/** Updates the teleport trace splines and targeting reticle. */
	void UpdateTeleportTrace();

	/** Called when we want to update the teleport trace splines and targeting reticle. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnUpdateTeleportTrace();

	/** Hides the teleport trace splines and targeting reticle. */
	void HideTeleportTrace();

	/** Called when we want to hide the teleport trace splines and targeting reticle. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnHideTeleportTrace();

	/** Updates the collision for this actor at the beginning of teleportation. */
	void UpdateCollisionForTeleportStart();

	/** Updates the collision for this actor at the beginning of teleportation. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnUpdateCollisionForTeleportStart();

	/** Updates the collision for this actor at the end of teleportation. */
	void UpdateCollisionForTeleportEnd();

	/** Updates the collision for this actor at the end of teleportation. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Teleportation")
	void OnUpdateCollisionForTeleportEnd();


	/** Gets the teleport trace origin and direction, as appropriate depending on the value of Teleport Hand. */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Teleportation")
	void GetTeleportTraceOriginAndDirection(FVector& Origin, FVector&Direction);

	/*
	* Updates based on the current teleport state. Should only be called on tick.
	* If tracing teleport, calls trace teleport, and updates the spline and teleport marker./
	* If teleporting, moves the character towards the teleport destination. 
	*/
	void TickUpdateTeleportState();

	/** Updates character walk if enabled. Should only be called on Tick. */
	void TickUpdateWalk();

	/** Our actual teleport goal location. Used to compensate for the offset between the camera and actor location. */
	FVector TeleportActorLocationGoal;

	/** The last time in world seconds that we began teleporting. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Teleportation")
	float TeleportLastStartTime;


	// ---------------------------------------------------------
	//	Health and damage

	/** Whether this character is currently invincible. We most likely want to begin with this on,
	and disable it after they have had a chance to load up.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	bool bInvincible;

	/** Whether this character can take damage from friendly fire. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	bool bCanTakeFriendlyFire;

	/** Our maximum health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	float MaxHealth;

	/** Our Current health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	float CurrentHealth;

	/** Normalized health regen rate, 
	* read as 1 / the time to regen to full health,
	* so 0.333 would be 3 seconds to full regen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	float HealthRegenRate;

	/** How long after the last damage was applied to begin regenerating health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	float HealthRegenDelay;

	/** True if health is regenerating, false otherwise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	bool bHealthRegenActive;

	/** The minimum delay between taking damage from the same damage causer. */
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	float MinTimeBetweenDamageCauserHits;

	/** List of recent damage causers and when the damage event occurred. */
	TArray<FEmpathDamageCauserLog> DamageCauserLogs;

	/** Cleans up damage causer log and returns true the inputted damage causer was found. */
	const bool CleanUpAndQueryDamageCauserLogs(float WorldTimeStamp, AActor* QueriedDamageCauser);

	/** Orders the character to die. Called when the character's health depletes to 0. */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Combat")
	void Die(FHitResult const& KillingHitInfo, FVector KillingHitImpulseDir, const AController* DeathInstigator, const AActor* DeathCauser, const UDamageType* DeathDamageType);

	/** Modifies any damage received from Take Damage calls */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Combat")
	float ModifyAnyDamage(float DamageAmount, const AController* EventInstigator, const AActor* DamageCauser, const UDamageType* DamageType);

	/** Modifies any damage from Point Damage calls (Called after ModifyAnyDamage and per bone damage scaling). */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Combat")
	float ModifyPointDamage(float DamageAmount, const FHitResult& HitInfo, const FVector& ShotDirection, const AController* EventInstigator, const AActor* DamageCauser, const UDamageType* DamageTypeCDO);

	/** Modifies any damage from Radial Damage calls (Called after ModifyAnyDamage). */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Combat")
	float ModifyRadialDamage(float DamageAmount, const FVector& Origin, const TArray<struct FHitResult>& ComponentHits, float InnerRadius, float OuterRadius, const AController* EventInstigator, const AActor* DamageCauser, const UDamageType* DamageTypeCDO);

	/** Processes final damage after all calculations are complete. Includes signaling stun damage and death events. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathPlayerCharacter|Combat")
	void ProcessFinalDamage(const float DamageAmount, FHitResult const& HitInfo, FVector HitImpulseDir, const UDamageType* DamageType, const AController* EventInstigator, const AActor* DamageCauser);

	/** The last time that this character was stunned. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Combat")
	float LastDamageTime;

	/** Updates our health regen as appropriate. Should only be called on tick. */
	void TickUpdateHealthRegen(float DeltaTime);

	// ---------------------------------------------------------
	//	Stun handling

	/** Whether this character is stunnable in principle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	bool bStunnable;

	/** How much the character has to accrue the StunDamageThreshold and become stunned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	float StunTimeThreshold;

	/** Instructs the character to become stunned. If StunDuration <= 0, then stun must be removed by EndStun(). */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Combat")
	void BeStunned(const AController* StunInstigator, const AActor* StunCauser, const float StunDuration = 3.0);

	FTimerHandle StunTimerHandle;

	/** Ends the stun effect on the character. Called automatically at the end of stun duration if it was > 0. */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Combat")
	void EndStun();

	/** How much damage needs to be done in the time defined in StunTimeThreshold for the character to become stunned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	float StunDamageThreshold;

	/** How long a stun lasts by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	float StunDurationDefault;

	/** How long after recovering from a stun that this character should be immune to being stunned again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Combat")
	float StunImmunityTimeAfterStunRecovery;

	/** The last time that this character was stunned. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Combat")
	float LastStunTime;

	/** History of stun damage that has been applied to this character. */
	TArray<FEmpathDamageHistoryEvent, TInlineAllocator<16>> StunDamageHistory;

	/** Checks whether we should become stunned */
	virtual void TakeStunDamage(const float StunDamageAmount, const AController* EventInstigator, const AActor* DamageCauser);

	// ---------------------------------------------------------
	//	Input

	/*
	* The minimum magnitude of a 2D axis before it triggers a Pressed event. 
	* When the magnitude drops below the threshold, a Released event is triggered.
	*/
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadWrite, EditAnywhere)
	float InputAxisEventThreshold;

	/*
	* The magnitude of the dead-zone for the analog stick when walking.
	* Will not walk when below this deadzone;
	*/
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadWrite, EditAnywhere)
	float InputAxisLocomotionWalkThreshold;

	/** The time we wait for the stick to be released in order to trigger a tapped event. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadWrite, EditAnywhere)
	float InputAxisTappedThreshold;

	/** The dead time after releasing the analog stick but did not trigger a dash that we wait before a dash can be triggered. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadWrite, EditAnywhere)
	float InputAxisTappedDeadTime;

	/** Updates and calls input axis event states. Necessary to be called on tick for events to fire and update properly. */
	void TickUpdateInputAxisEvents();

	/*
	* Default locomotion control mode. 
	* DefaultAndAltMovement: The default locomotion mode will be used by default. Holding the Alt Movement key will use the alternate movement mode.
	* PressToWalkTapToDash: The walk locomotion mode will be used by default. Tapping the stick will trigger a dash.
	*/
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", EditAnywhere, BlueprintReadOnly)
	EEmpathLocomotionControlMode LocomotionControlMode;

	/** Default locomotion mode. Alternate mode is available by holding the Alt Movement Mode key. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", EditAnywhere, BlueprintReadOnly)
	EEmpathLocomotionMode DefaultLocomotionMode;

	/** Sets the default locomotion mode. */
	UFUNCTION(Category = "EmpathPlayerCharacter|Input", BlueprintCallable)
	void SetDefaultLocomotionMode(EEmpathLocomotionMode LocomotionMode);

	/** Whether dash locomotion should be based on hand or head orientation. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", EditAnywhere, BlueprintReadWrite)
	EEmpathOrientation DashOrientation;

	/** Whether walk locomotion should be based on hand or head orientation. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", EditAnywhere, BlueprintReadWrite)
	EEmpathOrientation WalkOrientation;

	/** Gets the movement input axis oriented according to the current locomotion mode and the orientation settings. */
	FVector GetOrientedLocomotionAxis(const FVector2D InputAxis) const;

	// ---------------------------------------------------------
	//	Gripping and climbing

	/** Whether gripping is enabled. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Gripping", EditDefaultsOnly, BlueprintReadOnly)
	bool bGripEnabled;

	/** Enabled and disables gripping on this actor. */
	UFUNCTION(Category = "EmpathPlayerCharacter|Gripping", BlueprintCallable)
	void SetGripEnabled(const bool bNewEnabled);

	/** Called when gripping is enabled. */
	void OnGripEnabled();

	/** Called when gripping is enabled. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Gripping", meta = (DisplayName = "On Grip Enabled"))
	void ReceiveGripEnabled();

	/** Called when gripping is disabled. */
	void OnGripDisabled();

	/** Called when gripping is disabled. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Gripping", meta = (DisplayName = "On Grip Disabled"))
	void ReceiveGripDisabled();

	/** Sets whether climbing is currently enabled. */
	UFUNCTION(BlueprintCallable, Category = "EmpathPlayerCharacter|Climbing")
	void SetClimbEnabled(const bool bNewEnabled);

	/** Called when climbing is enabled. */
		void OnClimbEnabled();

	/** Called when climbing is enabled. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Climbing", meta = (DisplayName = "On Climb Enabled"))
		void ReceiveClimbEnabled();

	/** Called when climbing is disabled. */
		void OnClimbDisabled();

	/** Called when climbing is disabled. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Climbing", meta = (DisplayName = "On Climb Disabled"))
		void ReceiveClimbDisabled();

	/** The current dominant climbing hand of this character. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Climbing", BlueprintReadOnly)
	AEmpathHandActor* ClimbHand;

	/** The component currently gripped from climbing. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Climbing", BlueprintReadOnly)
	UPrimitiveComponent* ClimbGrippedComponent;

	/** The relative location offset of our current climb grip point from the climb gripped component. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Climbing", BlueprintReadOnly)
	FVector ClimbGripOffset;

	/** Updates our dominant grip point and hand. */
	UFUNCTION(Category = "EmpathPlayerCharacter|Climbing", BlueprintCallable)
	void SetClimbingGrip(AEmpathHandActor* Hand, UPrimitiveComponent* GrippedComponent, FVector GripOffset);

	/** Resets and clears and our current climbing state. */
	UFUNCTION(Category = "EmpathPlayerCharacter|Climbing", BlueprintCallable)
	void ClearClimbingGrip();

	/** Called when this character starts climbing. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Climbing")
	void OnClimbStart();

	/** Called when this character stops climbing. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathPlayerCharacter|Climbing")
	void OnClimbEnd();

	/** Updates movement from climbing. Should only be called on tick. */
	void TickUpdateClimbing();


	// ---------------------------------------------------------
	//	Charging

	/** Called when the power charged state changes. */
	void OnNewChargedState(const bool bNewChargedState, const EEmpathBinaryHand Hand);

	/** Called when the power charged state changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerCharacter, meta = (DisplayName = "On New Charged State"))
	void ReceiveNewChargedState(const bool bNewChargedState, const EEmpathBinaryHand Hand);

	/** Called when the power charged state changes. */
	UPROPERTY(BlueprintAssignable, Category = EmpathPlayerCharacter)
	FOnNewChargedStateDelegate OnNewChargedStateDelegate;


	// ---------------------------------------------------------
	//	Blocking

	/** Called when the power charged state changes. */
	void OnNewBlockingState(const bool bNewBlockingState, const EEmpathBinaryHand Hand);

	/** Called when the power charged state changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerCharacter, meta = (DisplayName = "On New Charged State"))
	void ReceiveNewBlockingState(const bool bNewBlockingState, const EEmpathBinaryHand Hand);

	/** Called when the power charged state changes. */
	UPROPERTY(BlueprintAssignable, Category = EmpathPlayerCharacter)
	FOnNewBlockingStateDelegate OnNewBlockingStateDelegate;


	// ---------------------------------------------------------
	//	Gestural controls

	/** Called when one of the hands changes gesture state. */
	void OnGestureStateChanged(const EEmpathGestureType OldGestureState,
		const EEmpathGestureType NewGestureState,
		const EEmpathBinaryHand Hand);

	/** Called when one of the hands changes gesture state. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerCharacter, meta = (DisplayName = "On Gesture State Changed"))
	void ReceiveGestureStateChanged(const EEmpathGestureType OldGestureState,
		const EEmpathGestureType NewGestureState,
		const EEmpathBinaryHand Hand);

	/** Called when one of the hands changes gesture state. */
	UPROPERTY(BlueprintAssignable, Category = "EmpathPlayerCharacter|Combat")
		FOnGestureStateChangedDelegate OnGestureStateChangedDelegate;

	/** Returns whether both hands are valid and can currently gesture cast. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathPlayerCharacter)
	const bool CanGestureCast() const;

	/** Returns whether this character can currently grip. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = EmpathPlayerCharacter)
	bool CanGrip() const;

	/** Sets whether gesture casting is enabled on both hands. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	void SetGestureCastingEnabled(const bool bNewEnabled);

	/** Updates the current gesture state of the hands. */
	void TickUpdateGestureState();

	/*
	* Updates the one-handed gesture state for a given hand. 
	* Returns a true if we are currently in, or are in the process of activating a gesture.
	*/
	const bool UpdateOneHandGesture(AEmpathHandActor* Hand);

	/** Whether the player can use cannon shot, in principle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Gestures")
	bool bCannonShotEnabled;

	/** Whether the player can punch, in principle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Gestures")
		bool bPunchEnabled;

	/** Whether the player can slash, in principle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Gestures")
		bool bSlashEnabled;

	/** Whether the player can block, in principle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Gestures")
		bool bBlockEnabled;

	/** The minimum time after completing a punch that we must wait before performing a cannon shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Gestures")
	float CannonShotCooldownAfterPunch;

	/** The minimum time after completing a slash that we must wait before performing a cannon shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Gestures")
	float CannonShotCooldownAfterSlash;

	/** The minimum time after completing a cannon shot that we must wait before performing another cannon shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Gestures")
	float CannonShotCooldownAfterCannonShot;

	/*
	* Overridable function for whether we can currently use cannon shot.
	* By default, returns true if enabled, not dead, not stunned, not teleporting, and not tracing teleport. 
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintNativeEvent, Category = EmpathPlayerCharacter)
	bool CanCannonShot() const;

	/** The gesture data for Cannon Shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathPlayerCharacter|Gestures")
	FEmpathPoseData CannonShotData;

	/** The last time in real seconds the player exited the Cannon Shot Static state without entering the Dynamic state. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathPlayerCharacter)
		float LastCannonShotStaticcExitTimeStamp;

	/** The last time in real seconds the player exited the Cannon Shot Dynamic state. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathPlayerCharacter)
	float LastCannonShotDynamicExitTimeStamp;

	/*
	* Calculates the vector from the damage capsule (with applied height offset) to the midpoint between the player hands' Palms.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathPlayerCharacter)
	const FVector GetDirectionToPalms(float BaseHeightOffset = 0.0f) const;

	/*
	* Calculates the midpoint between the player hands' Palms.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathPlayerCharacter)
	const FVector GetMidpointBetweenPalms() const;

	/** Calculates the vector from the damage capsule (with applied height offset) to the midpoint between the player hands' Palms as well as the midpoint between the controllers. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	bool GetMidpointAndDirectionToPalms(FVector& OutMidpoint, FVector& OutDirection, float BaseHeightOffset = 0.0f);

	/*
	* Calculates the vector from the damage capsule (with applied height offset) to the midpoint between the player hands' K Velocity Components.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathPlayerCharacter)
	const FVector GetDirectionToKVelComps(float BaseHeightOffset = 0.0f) const;

	/*
	* Calculates the midpoint between the player hands' K Velocity Components. 
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathPlayerCharacter)
	const FVector GetMidpointBetweenKVelComps() const;

	/** Calculates the vector from the damage capsule (with applied height offset) to the midpoint between the player hands' K Velocity Components as well as the midpoint between the controllers. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	bool GetMidpointAndDirectionToKVelComps(FVector& OutMidpoint, FVector& OutDirection, float BaseHeightOffset = 0.0f);

	/** Calculates the vector from the damage capsule (with applied height offset) to the midpoint between the motion controllers. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathPlayerCharacter)
	const FVector GetDirectionToMotionControllers(float BaseHeightOffset = 0.0f) const;

	/** Calculates the midpoint between the motion controllers. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathPlayerCharacter)
	const FVector GetMidpointBetweenKMotionControllers() const;

	/** Calculates the vector from the damage capsule (with applied height offset) to the midpoint between the motion controllers, as well as the midpoint between the controllers. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	bool GetMidpointAndDirectionToMotionControllers(FVector& OutMidpoint, FVector& OutDirection, float BaseHeightOffset = 0.0f);

	/** Updates the current gesture casting pose. Returns true of we are successfully entering or have entered a pose. */
	bool AttemptEnterStaticCastingPose();

	/** Sets the character to a new casting pose. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	void SetCastingPose(const EEmpathCastingPose NewPose);

	/** Called when the character's casting pose changes. */
	void OnCastingPoseChanged(const EEmpathCastingPose OldPose, const EEmpathCastingPose NewPose);

	/** Called when the character's casting pose changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerCharacter, meta = (DisplayName = "On Casting Pose Changed"))
	void ReceiveCastingPoseChanged(const EEmpathCastingPose OldPose, const EEmpathCastingPose NewPose);

	/** Called when the character's casting pose changes. */
	UPROPERTY(BlueprintAssignable, Category = "EmpathPlayerCharacter|Combat")
	FOnCastingPoseChangedDelegate OnCastingPoseChangedDelegate;

	/** Resets entry conditions for all poses aside from the gesture to ignore. */
	void ResetPoseEntryState(EEmpathCastingPose PoseToIgnore);

	/** Height offset for the player's height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CenterMassHeightOffset;

	/** Function for getting the center mass of the character. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	const FVector GetCenterMassLocation() const;

	/** Function for quickly setting all powers enabled. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	void SetAllPowersEnabled(const bool bNewEnabled);

	/** Function for quickly setting locomotion types enabled. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	void SetAllLocomotionEnabled(const bool bNewEnabled);

	/** Function for quickly setting all gameplay abilities enabled. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	void SetAllGameplayAbilitiesEnabled(const bool bNewEnabled);

	/** Function for setting whether gravity is enabled. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	void SetGravityEnabled(const bool bNewEnabled);
	
	/** Called when gravity is enabled. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerCharacter)
	void OnGravityEnabled();

	/** Called when gravity is disabled. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerCharacter)
	void OnGravityDisabled();


	// ---------------------------------------------------------
	//	UI Interaction

	/** Sets whether clicking is enabled. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	void SetClickEnabled(const bool bNewEnabled);

	/** Called when clicking is enabled on this player character. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerCharacter)
	void OnClickEnabled();

	/** Called when clicking is disabled on this player character. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathPlayerCharacter)
	void OnClickDisabled();

	/** Returns whether we can currently click. By default, returns true if click enabled. */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = EmpathPlayerCharacter)
	bool CanClick() const;

private:
	/** Whether this character is currently dead. */
	bool bDead;

	/** Whether this character is currently stunned.*/
	bool bStunned;

	/** The player controller for this VR character. */
	UPROPERTY(Category = EmpathPlayerCharacter, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	AEmpathPlayerController* CachedEmpathPlayerCon;


	// ---------------------------------------------------------
	//	Hands

	/** The right hand actor class of this character. */
	UPROPERTY(Category = EmpathPlayerCharacter, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEmpathHandActor> RightHandClass;

	/** The right hand actor class of this character. */
	UPROPERTY(Category = EmpathPlayerCharacter, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEmpathHandActor> LeftHandClass;

	/** Reference to the right hand actor. */
	UPROPERTY(Category = EmpathPlayerCharacter, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	AEmpathHandActor* RightHandActor;

	/** Reference to the left hand actor. */
	UPROPERTY(Category = EmpathPlayerCharacter, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	AEmpathHandActor* LeftHandActor;

	/** Call to clear all gripped objects. */
	UFUNCTION(BlueprintCallable, Category = EmpathPlayerCharacter)
	void ClearHandGrips();


	// ---------------------------------------------------------
	//	Teleportation

	/** The teleport marker class of this character. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEmpathTeleportMarker> TeleportMarkerClass;

	/** Reference to the teleport marker. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	AEmpathTeleportMarker* TeleportMarker;

	/** The capsule component being used for damage collision. */
	UPROPERTY(Category = EmpathPlayerCharacter, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* DamageCapsule;

	/** The current teleport state of the character. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Teleportation", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	EEmpathTeleportTraceResult TeleportTraceResult;

	// Climbing

	/** Whether this character can climb in principle. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Climbing", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bClimbEnabled;

	/** Whether this character is currently climbing. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Climbing", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bClimbing;


	// ---------------------------------------------------------
	//	Input

	/** The current locomotion mode of the character. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	EEmpathLocomotionMode CurrentLocomotionMode;

	// Locomotion
	/** Current value of our Dash input axis. X is Right, Y is Up. -X is Left, -Y is down. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector2D LocomotionInputAxis;
	void LocomotionAxisUpDown(float AxisValue);
	void LocomotionAxisRightLeft(float AxisValue);

	/** Whether the Dash input axis is considered "pressed." */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bLocomotionPressed;

	/** Whether we are currently walking. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bWalking;

	/** The last time in world seconds that we began pressing the locomotion axis */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Input", meta = (AllowPrivateAccess = "true"))
	float LocomotionLastPressedTime;

	/** The last time in world seconds that the locomotion axis was released. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Input", meta = (AllowPrivateAccess = "true"))
	float LocomotionLastReleasedTime;

	/** Whether gravity is currently enabled in principle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EmpathPlayerCharacter, meta = (AllowPrivateAccess = "true"))
	bool bGravityEnabled;


	// Alt movement

	/** Whether the Alt Movement key is pressed. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bAltMovementPressed;

	/** Called when the Alt Movement key is pressed. */
	void OnAltMovementPressed();

	/** Called when the Alt Movement key is released. */
	void OnAltMovementReleased();


	// Teleportation
	/** Current value of our Teleport input axis. X is Right, Y is Up. -X is Left, -Y is down. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector2D TeleportInputAxis;
	void TeleportAxisUpDown(float AxisValue);
	void TeleportAxisRightLeft(float AxisValue);

	/** Whether the Teleport input axis is considered "pressed." */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bTeleportPressed;

	/** Whether the Teleport input axis is considered "held." */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bTeleportHeld;

	/** The last time in world seconds that we began pressing the teleport axis */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Input", meta = (AllowPrivateAccess = "true"))
	float TeleportLastPressedTime;

	/** The last time in world seconds that the teleport axis was released. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathPlayerCharacter|Input", meta = (AllowPrivateAccess = "true"))
	float TeleportLastReleasedTime;



	// Grip

	/** Whether the Grip Right key is pressed. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bGripRightPressed;

	/** Whether the Grip Left key is pressed. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bGripLeftPressed;

	/** Called when the right Grip button is pressed. */
	void OnGripRightPressed();

	/** Called when the right Grip button is released. */
	void OnGripRightReleased();

	/** Called when the left Grip button is pressed. */
	void OnGripLeftPressed();

	/** Called when the left Grip button is released. */
	void OnGripLeftReleased();


	// Clicking
	/** Whether the Click Right key is pressed. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		bool bClickRightPressed;

	/** Whether the Click Left key is pressed. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		bool bClickLeftPressed;

	/** Whether clicking is enabled. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		bool bClickEnabled;

	/** Called when the right Click button is pressed. */
	void OnClickRightPressed();

	/** Called when the right Click button is released. */
	void OnClickRightReleased();

	/** Called when the left Click button is pressed. */
	void OnClickLeftPressed();

	/** Called when the left Click button is released. */
	void OnClickLeftReleased();


	// Charging
	/** Called when the Charge right button is pressed. */
	void OnChargeRightPressed();

	/** Called when the Charge right button is released. */
	void OnChargeRightReleased();

	/** Called when the left Charge button is pressed. */
	void OnChargeLeftPressed();

	/** Called when the left Charge button is released. */
	void OnChargeLeftReleased();


	// Gesture casting

	/** The current casting pose of the character. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathPlayerCharacter, meta = (AllowPrivateAccess = "true"))
		EEmpathCastingPose CastingPose;

	/** Whether the Charge Right key is pressed. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		bool bChargeRightPressed;

	/** Whether the Charge Left key is pressed. */
	UPROPERTY(Category = "EmpathPlayerCharacter|Input", EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		bool bChargeLeftPressed;
};
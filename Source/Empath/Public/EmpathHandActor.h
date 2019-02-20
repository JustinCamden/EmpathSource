// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmpathTypes.h"
#include "EmpathTeamAgentInterface.h"
#include "EmpathHandActor.generated.h"

class USphereComponent;
class AEmpathPlayerCharacter;
class UEmpathKinematicVelocityComponent;

UCLASS()
class EMPATH_API AEmpathHandActor : public AActor, public IEmpathTeamAgentInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEmpathHandActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// ---------------------------------------------------------
	//	Setup

	/** Name of the blocking collision. */
	static FName BlockingCollisionName;

	/** Name of the Kinematic Velocity Component. */
	static FName KinematicVelocityComponentName;

	/** Name of the mesh component. */
	static FName MeshComponentName;

	/** Name of the grip collision component. */
	static FName GripCollisionName;

	/** Name of the palm component. */
	static FName PalmMarkerName;

	/** Call to register this hand with the other hand and the owning player character. */
	UFUNCTION(BlueprintCallable, Category = EmpathHandActor)
	void RegisterHand(AEmpathHandActor* InOtherHand, 
		AEmpathPlayerCharacter* InOwningCharacter,
		USceneComponent* InFollowedComponent,
		EEmpathBinaryHand InOwningHand);

	/** Called after being registered with the owning player character and the other hand. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor)
		void OnHandRegistered();

	/** Gets the hand type (Left or Right of this hand actor. */
	EEmpathBinaryHand GetOwningHand() const { return OwningHand; }

	/** Called when the owning player teleports. */
	void OnOwningPlayerTeleport();

	/** Called when the owning player teleports. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Owning Player Teleport"))
	void ReceiveOwningPlayerTeleport();

	/** Called when the owning player finishes teleporting. */
	void OnOwningPlayerTeleportEnd();

	/** Called when the owning player finishes teleporting. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Owning Player Teleport End"))
	void ReceiveOwningPlayerTeleportEnd();

	// ---------------------------------------------------------
	//	 Team Agent Interface

	/** Returns the team number of the actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathPlayerCharacter)
	EEmpathTeam GetTeamNum() const;
	
	// ---------------------------------------------------------
	//	Follow component

	/** The maximum distance from the component we are following before we are officially lost. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathHandActor)
	float FollowComponentLostThreshold;

	/** Updates whether we have currently lost the follow component, and fires events if necessary. */
	UFUNCTION(BlueprintCallable, Category = EmpathHandActor)
	void UpdateLostFollowComponent(bool bLost);

	/** Called when we first lose the follow component. */
	void OnLostFollowComponent();

	/** Called when we first lose the follow component. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = EmpathHandActor, meta = (DisplayName = "On Lost Follow Component"))
	void ReceiveLostFollowComponent();

	/** Called when we regain the follow component after it being lost. */
	void OnFoundFollowComponent();

	/** Called when we first lose the follow component. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = EmpathHandActor, meta = (DisplayName = "On Found Follow Component"))
	void ReceiveFoundFollowComponent();

	/** Returns whether the follow component is lost. */
	bool IsFollowComponentLost() const { return bLostFollowComponent; }

	/** Called every tick to instruct the hand to sweep its location and rotation to the followed component (usually a motion controller component). */
	UFUNCTION(BlueprintCallable, Category = EmpathHandActor)
	void MoveToFollowComponent();

	/** Called when this hand begins climbing. */
	void OnClimbStart();

	/** Called when this hand begins climbing. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Climb Start"))
	void ReceiveClimbStart();

	/** Called when this hand stops climbing. */
	void OnClimbEnd();

	/** Called when this hand stops climbing. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Climb End"))
	void ReceiveClimbEnd();

	/** Called when climbing is enabled on the owning player character. */
	void OnClimbEnabled();

	/** Called when climbing is disabled on the owning player character. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Climb Enabled"))
	void ReceiveClimbEnabled();

	/** Called when climbing is disabled on the owning player character. */
	void OnClimbDisabled();

	/** Called when climbing is disabled on the owning player character. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Climb Disabled"))
	void ReceiveClimbDisabled();

	// ---------------------------------------------------------
	//	Gripping

	/** Called by the owning character to determine the origin of the teleportation trace. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure, Category = "EmpathHandActor|Teleportation")
	FVector GetTeleportOrigin() const;

	/** Called by the owning character to determine the direction of the teleportation trace. X is forward, Y is right. Input does not need to be normalized */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure, Category = "EmpathHandActor|Teleportation")
	FVector GetTeleportDirection(FVector LocalDirection) const;

	/** Returns the current grip state of this hand actor. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EmpathHandActor|Gripping")
	EEmpathGripType GetGripState() const { return GripState; }

	/** The object currently held by the hand. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathHandActor|Gripping")
	AActor* HeldObject;

	/** Gets the nearest Actor overlapping the grip collision. */
	UFUNCTION(BlueprintCallable, Category = "EmpathHandActor|Gripping")
	void GetBestGripCandidate(AActor*& GripActor, UPrimitiveComponent*& GripComponent, EEmpathGripType& GripResponse);

	/** Called when the grip key is pressed and the Empath Character can grip. */
	void OnGripPressed();

	/** Called when the grip key is pressed and the Empath Character can grip. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathHandActor|Gripping", meta = (DisplayName = "On Grip Pressed"))
	void ReceiveGripPressed();

	/** Called when the grip key is released, or when we are gripping something and grip is disabled. */
	void OnGripReleased();

	/** Called when the grip key is released, or when we are gripping something and grip is disabled. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathHandActor|Gripping", meta = (DisplayName = "On Grip Released"))
		void ReceiveGripReleased();

	/** Call to clear all grip objects. */
	UFUNCTION(BlueprintCallable, Category = "EmpathHandActor|Gripping")
	void ClearGrip();

	/** Checks to see whether this hand can climb a nearby object. */
	bool CheckForClimbGrip();

	/** Called when an actor is gripped begins. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathHandActor|Gripping", meta = (DisplayName = "On Grip Actor Start"))
		void ReceiveGripActorStart(AActor* NewGrippedActor);

	/** Called when a gripped actor is release. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathHandActor|Gripping", meta = (DisplayName = "On Grip Actor Released"))
		void ReceiveGripActorEnd(AActor* OldGrippedActor);

	/** Reference to the currently gripped actor. */
	UPROPERTY(BlueprintReadWrite, Category = "EmpathHandActor|Gripping")
	AActor* GrippedActor;

	/** Called when Gripping is enabled on the owning player character. */
	void OnGripEnabled();

	/** Called when Gripping is disabled on the owning player character. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Grip Enabled"))
	void ReceiveGripEnabled();

	/** Called when Gripping is disabled on the owning player character. */
	void OnGripDisabled();

	/** Called when Gripping is disabled on the owning player character. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Grip Disabled"))
	void ReceiveGripDisabled();

	// ---------------------------------------------------------
	//	Gestural casting

	/** Gets the kinematic velocity component. */
	UEmpathKinematicVelocityComponent* GetKinematicVelocityComponent() const { return KinematicVelocityComponent; }

	/*
*	Overridable function for whether this hand can currently be used for gesture casting.
*	By default, true if bGestureCasting enabled and the player is not dead, stunned, or teleporting.
*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathHandActor)
		bool CanGestureCast() const;

	/*
	*	Overridable function for whether this hand can currently punch.
	*	By default, true if bPunchEnabled and CanGestureCast 
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathHandActor)
	bool CanPunch() const;
	/*
	*	Overridable function for whether this hand can currently slash up.
	*	By default, true if bSlashUpEnabled and CanGestureCast
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathHandActor)
	bool CanSlash() const;

	/*
	*	Overridable function for whether this hand can currently slash down.
	*	By default, true if bSlashDownEnabled and CanGestureCast
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathHandActor)
	bool CanBlock() const;

	/** The check information for Blocking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Gestures")
	FEmpathOneHandGestureData BlockingData;

	/** The transform cache for Blocking. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathHandActor)
	FEmpathGestureTransformCache BlockingTransformCache;

	/** Sets whether gesture casting is enabled on this hand. */
	UFUNCTION(BlueprintCallable, Category = EmpathHandActor)
	void SetGestureCastingEnabled(const bool bNewEnabled);

	/** Called when gesture casting is enabled. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor)
	void OnGestureCastingEnabled();

	/** Called when gesture casting is disabled. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor)
	void OnGestureCastingDisabled();

	/** Sets the active one handed gesture of the hand actor. Also updates the gesture state if appropriate. */
	UFUNCTION(BlueprintCallable, Category = EmpathHandActor)
	void SetActiveOneHandGestureIdx(int32 Idx = -1);

	/** Sets the gesture state of the hand actor. */
	UFUNCTION(BlueprintCallable, Category = EmpathHandActor)
	void SetGestureState(const EEmpathGestureType NewGestureState);

	/** Gets the gesture state of the hand actor. */
	UFUNCTION(BlueprintCallable, Category = EmpathHandActor)
	const EEmpathGestureType GetActiveGestureType() const { return ActiveGestureType; }

	/** Gets the gesture condition state of the hand actor. */
	UFUNCTION(BlueprintCallable, Category = EmpathHandActor)
	const FEmpathGestureCheck GetGestureConditionCheck() const { return GestureConditionCheck; }

	/*
	* Gets the gesture condition state of the hand actor for this frame. 
	* Does not use buffered velocity.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathHandActor)
	const FEmpathGestureCheck GetFrameGestureConditionCheck() const { return FrameConditionCheck; }

	/** Array one handed gestures that can be entered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Gestures")
	TArray<FEmpathOneHandGestureDataTyped> TypedOneHandedGestures;

	/** The minimum time after completing a punch that we must wait before performing another punch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Gestures")
	float PunchCooldownAfterPunch;

	/** The minimum time after completing a slash that we must wait before performing a punch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Gestures")
		float PunchCooldownAfterSlash;

	/** The minimum time after entering but not completing a cannon shot that we must wait before performing a punch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Gestures")
		float PunchCooldownAfterCannonShotStatic;

	/** The minimum time after completing a cannon shot that we must wait before performing a punch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Gestures")
		float PunchCooldownAfterCannonShotDynamic;

	/** The minimum time after completing a punch that we must wait before performing a slash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Gestures")
		float SlashCooldownAfterPunch;

	/** The minimum time after completing a slash that we must wait before performing another slash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Gestures")
		float SlashCooldownAfterSlash;

	/** The minimum time after entering but not completing a cannon shot that we must wait before performing a slash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Gestures")
		float SlashCooldownAfterCannonShotStatic;

	/** The minimum time after completing a cannon shot that we must wait before performing a slash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Gestures")
		float SlashCooldownAfterCannonShotDynamic;

	/** Called after the gesture state changes. */
	void OnGestureStateChanged(const EEmpathGestureType OldGestureState, const EEmpathGestureType NewGestureState);

	/** Called after the gesture state changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Gesture State Changed"))
	void ReceiveGestureStateChanged(const EEmpathGestureType OldGestureState, const EEmpathGestureType NewGestureState);

	/** Called when the other hand changes its gesture state. */
	void OnOtherHandGestureStateChanged(const EEmpathGestureType OtherHandOldGestureState, const EEmpathGestureType OtherHandNewGestureState);

	/** Called when the other hand changes its gesture state. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Other Hand Gesture State Changed"))
	void ReceiveOtherHandGestureStateChanged(const EEmpathGestureType OtherHandOldGestureState, const EEmpathGestureType OtherHandNewGestureState);

	/** Updates current block state. */
	void TickUpdateBlockingState();

	/** Updates current grip state. */
	void TickUpdateGripState();

	/** Sets whether we are currently blocking. */
	void SetIsBlocking(const bool bNewIsBlocking);

	/** Called when we enter blocking state. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor)
	void OnBlockingStart();

	/** Called when we exit blocking state. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor)
	void OnBlockingEnd();

	/*
	* Gets and attempts the current best one handed gesture for this hand. 
	* Assumes that we do not currently have an active gesture.
	* Returns true if we are successfully entering or have entered a gesture.
	*/
	const bool AttemptEnterOneHandGesture();

	/** Resets entry conditions for all one handed gestures aside from the gesture to ignore. */
	void ResetOneHandeGestureEntryState(int32 IdxToIgnore = -1);

	/*
	* Instructs the hand to maintain its current one handed gesture. 
	* Returns true if we are still maintaining the gesture.
	*/
	const bool AttemptSustainOneHandGesture();

	/** Gets the results of all possible one handed conditions checks. */
	void UpdateGestureConditionChecks();

	// ---------------------------------------------------------
	//	Charging

	/** Returns whether this hand is currently charged. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathHandActor)
	const bool IsPowerCharged() const { return bIsPowerCharged; }

	/** Sets whether this hand is currently charged. */
	UFUNCTION(BlueprintCallable, Category = EmpathHandActor)
	void SetIsPowerCharged(const bool bNewIsCharged);

	/** Called when the power charged state starts. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor)
	void OnPowerChargedStart();

	/** Called when the power charged state ends. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor)
	void OnPowerChargedEnd();

	/** Called when the Charge key is pressed and the Empath Character can Charge. */
	void OnChargePressed();

	/** Called when the Charge key is pressed and the Empath Character can Charge. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathHandActor|Chargeping", meta = (DisplayName = "On Charge Pressed"))
		void ReceiveChargePressed();

	/** Called when the Charge key is released, or when we are Charging something and Charge is disabled. */
	void OnChargeReleased();

	/** Called when the Charge key is released, or when we are Charging something and Charge is disabled. */
	UFUNCTION(BlueprintImplementableEvent, Category = "EmpathHandActor|Chargeping", meta = (DisplayName = "On Charge Released"))
		void ReceiveChargeReleased();


	// ---------------------------------------------------------
	//	Damage
	// ---------------------------------------------------------
	//	Health and damage

	/** Whether this hand is currently invincible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Combat")
	bool bHandInvincible;

	/** Whether this hand is can take friendly fire damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EmpathHandActor|Combat")
	bool bCanHandTakeFriendlyFire;

	// Override for Take Damage that calls our own custom Process Damage script (Since we can't override the OnAnyDamage event in c++)
	virtual float TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Modifies any damage received from Take Damage calls */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathHandActor|Combat")
		float ModifyAnyDamage(float DamageAmount, const AController* EventInstigator, const AActor* DamageCauser, const UDamageType* DamageType);

	/** Modifies any damage from Point Damage calls (Called after ModifyAnyDamage and per bone damage scaling). */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathHandActor|Combat")
		float ModifyPointDamage(float DamageAmount, const FHitResult& HitInfo, const FVector& ShotDirection, const AController* EventInstigator, const AActor* DamageCauser, const UDamageType* DamageTypeCDO);

	/** Modifies any damage from Radial Damage calls (Called after ModifyAnyDamage). */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathHandActor|Combat")
		float ModifyRadialDamage(float DamageAmount, const FVector& Origin, const TArray<struct FHitResult>& ComponentHits, float InnerRadius, float OuterRadius, const AController* EventInstigator, const AActor* DamageCauser, const UDamageType* DamageTypeCDO);

	/** Processes final damage after all calculations are complete. Includes signaling stun damage and death events. */
	UFUNCTION(BlueprintNativeEvent, Category = "EmpathHandActor|Combat")
		void ProcessFinalDamage(const float DamageAmount, FHitResult const& HitInfo, FVector HitImpulseDir, const UDamageType* DamageType, const AController* EventInstigator, const AActor* DamageCauser);

	// ---------------------------------------------------------
	//	UI Interaction

	/** Called when the hand clicks. */
		void OnClickPressed();

	/** Called when the hand clicks. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Click Pressed"))
		void ReceiveClickPressed();

	/** Called when the hand un-clicks. */
		void OnClickReleased();

	/** Called when the hand un-clicks. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Click Released"))
		void ReceiveClickReleased();

	/** Called when clicking is enabled on the owning player character. */
		void OnClickEnabled();

	/** Called when clicking is enabled on the owning player character. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Click Enabled"))
		void ReceiveClickEnabled();


	/** Called when clicking is disabled on the owning player character. */
		void OnClickDisabled();

	/** Called when clicking is disabled on the owning player character. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathHandActor, meta = (DisplayName = "On Click Disabled"))
		void ReceiveClickDisabled();

	/** The current one hand gesture condition state of this hand. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathHandActor|Gestures")
		FEmpathGestureCheck GestureConditionCheck;

	/*
	* The current one hand gesture condition state of this hand with.
	* Calculated with respect to this frame only.
	*/
	UPROPERTY(BlueprintReadOnly, Category = "EmpathHandActor|Gestures")
		FEmpathGestureCheck FrameConditionCheck;

	/** Map of the hand locations and rotations when beginning to enter or exit a particular gesture state. */
	TMap<EEmpathGestureType, FEmpathGestureTransformCache> GestureTransformCaches;

	/** Gets the transform cache of the Punch state. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathHandActor)
	FEmpathGestureTransformCache& GetPunchTransformCache() { return GestureTransformCaches[EEmpathGestureType::Punching]; }

	/** Gets the transform cache of the Slash state. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathHandActor)
	FEmpathGestureTransformCache& GetSlashTransformCache() { return GestureTransformCaches[EEmpathGestureType::Slashing]; }

	/** Gets the transform cache of the Cannon Shot Static state. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathHandActor)
	FEmpathGestureTransformCache& GetCannonShotStaticTransformCache() { return GestureTransformCaches[EEmpathGestureType::CannonShotStatic]; }

	/** Gets the transform cache of the Cannon Shot Dynamic state. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathHandActor)
	FEmpathGestureTransformCache& GetCannonShotDynamicTransformCache() { return GestureTransformCaches[EEmpathGestureType::CannonShotDynamic]; }

	/** The last time in real seconds that we exited the Punch state. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		float LastPunchExitTimeStamp;

	/** The last time in real seconds that we exited the Slash state. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		float LastSlashExitTimeStamp;


	// Misc

	/** Gets the palm marker component. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathHandActor)
	USceneComponent* GetPalmMarkerComponent() const { return PalmMarker; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** The hand's currently active gesture. */
	UPROPERTY(BlueprintReadOnly, Category = "EmpathHandActor|Gestures")
	EEmpathGestureType ActiveGestureType;


private:

	// ---------------------------------------------------------
	//	Components

	/** The Sphere component used for collision. */
	UPROPERTY(Category = EmpathHandActor, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USphereComponent* BlockingCollision;

	/** The Sphere component used for grip collision. */
	UPROPERTY(Category = EmpathHandActor, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USphereComponent* GripCollision;

	/** The kinematic velocity component used for movement detection. */
	UPROPERTY(Category = EmpathHandActor, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UEmpathKinematicVelocityComponent* KinematicVelocityComponent;
	
	/** Reference to the other hand actor. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	AEmpathHandActor* OtherHand;

	/** Reference to the owning Empath Player Character hand actor. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	AEmpathPlayerCharacter* OwningPlayerCharacter;

	/** Reference to the scene component we are following. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* FollowedComponent;

	/** The main skeletal mesh associated with this hand. */
	UPROPERTY(Category = EmpathHandActor, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* MeshComponent;

	/** Whether we are currently separated from the component we are following. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bLostFollowComponent;

	/** The initial location offset we apply for this hand (automatically inverted for the left hand). */
	UPROPERTY(Category = EmpathHandActor, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FVector ControllerOffsetLocation;

	/** The initial rotation offset we apply for this hand (automatically inverted for the left hand). */
	UPROPERTY(Category = EmpathHandActor, EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FRotator ControllerOffsetRotation;

	/** Scene Component used to mark the location of the hand's palm. */
	UPROPERTY(Category = EmpathHandActor, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* PalmMarker;

	// ---------------------------------------------------------
	//	State

	/** Whether this hand can be used for gesture casting, in principle. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bGestureCastingEnabled;

	/** Which hand this actor represents. Set when registered with the Empath Player Character. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	EEmpathBinaryHand OwningHand;

	/** The current grip state of this hand. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	EEmpathGripType GripState;

	/** The index of the previously active one handed gesture. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 LastActiveOneHandGestureIdx;

	/** The index of the currently active one handed gesture. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 ActiveOneHandGestureIdx;

	/** Whether we are currently blocking. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bIsBlocking;

	/** Whether the grip Key for this hand is currently pressed. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bGripPressed;

	/** Whether the click Key for this hand is currently pressed. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bClickPressed;

	/** Whether the charge Key for this hand is currently pressed. */
	UPROPERTY(Category = EmpathHandActor, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bChargePressed;

	/** Whether this hand is currently charged. */
	bool bIsPowerCharged;
};

// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathPlayerCharacter.h"
#include "EmpathPlayerController.h"
#include "EmpathDamageType.h"
#include "EmpathFunctionLibrary.h"
#include "EmpathHandActor.h"
#include "EmpathTeleportBeacon.h"
#include "NavigationSystem/Public/NavigationData.h"
#include "Runtime/Engine/Public/EngineUtils.h"
#include "EmpathCharacter.h"
#include "DrawDebugHelpers.h"
#include "EmpathTeleportMarker.h"
#include "Runtime/HeadMountedDisplay/Public/HeadMountedDisplayFunctionLibrary.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "EmpathKinematicVelocityComponent.h"

// Stats for UE Profiler
DECLARE_CYCLE_STAT(TEXT("Empath Player Char Take Damage"), STAT_EMPATH_PlayerTakeDamage, STATGROUP_EMPATH_PlayerCharacter);
DECLARE_CYCLE_STAT(TEXT("Empath Player Char Teleport Trace"), STAT_EMPATH_PlayerTraceTeleport, STATGROUP_EMPATH_PlayerCharacter);
DECLARE_CYCLE_STAT(TEXT("Empath Player Char Gesture Recognition"), STAT_EMPATH_PlayerGestureRecognition, STATGROUP_EMPATH_PlayerCharacter);

// Log categories
DEFINE_LOG_CATEGORY_STATIC(LogTeleportTrace, Log, All);


// Console variable setup so we can enable and disable debugging from the console
static TAutoConsoleVariable<int32> CVarEmpathTeleportDrawDebug(
	TEXT("Empath.TeleportDrawDebug"),
	0,
	TEXT("Whether to enable teleportation debug.\n")
	TEXT("0: Disabled, 1: Enabled"),
	ECVF_Scalability | ECVF_RenderThreadSafe);
static const auto TeleportDrawDebug = IConsoleManager::Get().FindConsoleVariable(TEXT("Empath.TeleportDrawDebug"));


static TAutoConsoleVariable<float> CVarEmpathTeleportDebugLifetime(
	TEXT("Empath.TeleportDebugLifetime"),
	0.04f,
	TEXT("Duration of debug drawing for teleporting, in seconds."),
	ECVF_Scalability | ECVF_RenderThreadSafe);
static const auto TeleportDebugLifetime = IConsoleManager::Get().FindConsoleVariable(TEXT("Empath.TeleportDebugLifetime"));

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#define TELEPORT_LOC(_Loc, _Radius, _Color)				if (TeleportDrawDebug->GetInt()) { DrawDebugSphere(GetWorld(), _Loc, _Radius, 16, _Color, false, -1.0f, 0, 3.0f); }
#define TELEPORT_LINE(_Loc, _Dest, _Color)				if (TeleportDrawDebug->GetInt()) { DrawDebugLine(GetWorld(), _Loc, _Dest, _Color, false,  -1.0f, 0, 3.0f); }
#define TELEPORT_LOC_DURATION(_Loc, _Radius, _Color)	if (TeleportDrawDebug->GetInt()) { DrawDebugSphere(GetWorld(), _Loc, _Radius, 16, _Color, false, TeleportDebugLifetime->GetFloat(), 0, 3.0f); }
#define TELEPORT_LINE_DURATION(_Loc, _Dest, _Color)		if (TeleportDrawDebug->GetInt()) { DrawDebugLine(GetWorld(), _Loc, _Dest, _Color, false, TeleportDebugLifetime->GetFloat(), 0, 3.0f); }
#define TELEPORT_TRACE_DEBUG_TYPE(_Params)				if (TeleportDrawDebug->GetInt()) { _Params.DrawDebugType = EDrawDebugTrace::ForOneFrame; }
#else
#define TELEPORT_LOC(_Loc, _Radius, _Color)				/* nothing */
#define TELEPORT_LINE(_Loc, _Dest, _Color)				/* nothing */
#define TELEPORT_LOC_DURATION(_Loc, _Radius, _Color)	/* nothing */
#define TELEPORT_LINE_DURATION(_Loc, _Dest, _Color)		/* nothing */
#define TELEPORT_TRACE_DEBUG_TYPE()						/* nothing */
#endif

AEmpathPlayerCharacter::AEmpathPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Initialize default variables
	bDead = false;
	bHealthRegenActive = true;
	bInvincible = false;
	bStunnable = false;
	bGravityEnabled = true;
	DashTraceSettings.bTraceForBeacons = false;
	DashTraceSettings.bTraceForEmpathChars = false;
	DashTraceSettings.bSnapToMinDistance = false;
	DashTraceSettings.bTraceForWallClimb = false;
	HealthRegenDelay = 3.0f;
	HealthRegenRate = 1.0f / 3.0f;	// 3 seconds to full regen
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;
	StunDamageThreshold = 50.0f;
	StunTimeThreshold = 0.5f;
	StunDurationDefault = 3.0f;
	StunImmunityTimeAfterStunRecovery = 3.0f;
	CannonShotCooldownAfterPunch = 0.5f;
	CannonShotCooldownAfterSlash = 0.5f;
	CannonShotCooldownAfterCannonShot = 1.5f;
	TeleportMagnitude = 1500.0f;
	DashMagnitude = 800.0f;
	TeleportRadius = 4.0f;
	TeleportVelocityLerpSpeed = 20.0f;
	TeleportBeaconMinDistance = 20.0f;
	InputAxisEventThreshold = 0.6f;
	InputAxisLocomotionWalkThreshold = 0.2f;
	TeleportMovementSpeed = 5000.0f;
	TeleportRotationSpeed = 720.0f;
	TeleportRotation180Speed = 1800.0f;
	TeleportTimoutTime = 0.5f;
	TeleportDistTolerance = 5.0f;
	TeleportRotTolerance = 5.0f;
	TeleportPivotStep = 45.0f;
	InputAxisTappedThreshold = 0.6f;
	MinTimeBetweenDamageCauserHits = 0.5f;
	TeleportRotationAndLocationMinDeltaAngle = 45.0f;
	TeleportToLocationMinDist = 50.0f;
	TeleportWallClimbQueryRadius = 50.0f;
	TeleportWallClimbReach = 100.0f;
	TeleportProjectQueryExtent = FVector(150.0f, 150.0f, 150.0f);
	TeleportTraceObjectTypes.Add((TEnumAsByte<EObjectTypeQuery>)ECC_WorldStatic); // World Statics
	TeleportTraceObjectTypes.Add((TEnumAsByte<EObjectTypeQuery>)ECC_Teleport); // Teleport channel
	DashOrientation = EEmpathOrientation::Head;
	WalkOrientation = EEmpathOrientation::Hand;
	CastingPose = EEmpathCastingPose::NoPose;
	TeleportHand = EEmpathBinaryHand::Right;
	LocomotionControlMode = EEmpathLocomotionControlMode::DefaultAndAltMovement;
	DefaultLocomotionMode = EEmpathLocomotionMode::Dash;

	// Initialize damage capsule
	DamageCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("DamageCapsule"));
	DamageCapsule->SetupAttachment(VRReplicatedCamera);
	DamageCapsule->SetCapsuleHalfHeight(30.0f);
	DamageCapsule->SetCapsuleRadius(8.5f);
	DamageCapsule->SetRelativeLocation(FVector(1.75f, 0.0f, -18.5f));
	DamageCapsule->SetCollisionProfileName(FEmpathCollisionProfiles::DamageCollision);
	DamageCapsule->SetEnableGravity(false);

	// Set default class type
	RightHandClass = AEmpathHandActor::StaticClass();
	LeftHandClass = AEmpathHandActor::StaticClass();
	TeleportMarkerClass = AEmpathTeleportMarker::StaticClass();

	// Initialize default collision
	VRRootReference->SetCollisionProfileName(FEmpathCollisionProfiles::PlayerRoot);
}

void AEmpathPlayerCharacter::SetupPlayerInputComponent(class UInputComponent* NewInputComponent)
{
	Super::SetupPlayerInputComponent(NewInputComponent);

	// Dash
	NewInputComponent->BindAxis("LocomotionAxisUpDown", this, &AEmpathPlayerCharacter::LocomotionAxisUpDown);
	NewInputComponent->BindAxis("LocomotionAxisRightLeft", this, &AEmpathPlayerCharacter::LocomotionAxisRightLeft);

	// Teleport
	NewInputComponent->BindAxis("TeleportAxisUpDown", this, &AEmpathPlayerCharacter::TeleportAxisUpDown);
	NewInputComponent->BindAxis("TeleportAxisRightLeft", this, &AEmpathPlayerCharacter::TeleportAxisRightLeft);

	// Alt movement
	NewInputComponent->BindAction("AltMovementMode", IE_Pressed, this, &AEmpathPlayerCharacter::OnAltMovementPressed);
	NewInputComponent->BindAction("AltMovementMode", IE_Released, this, &AEmpathPlayerCharacter::OnAltMovementReleased);

	// Grip
	NewInputComponent->BindAction("GripRight", IE_Pressed, this, &AEmpathPlayerCharacter::OnGripRightPressed);
	NewInputComponent->BindAction("GripRight", IE_Released, this, &AEmpathPlayerCharacter::OnGripRightReleased);
	NewInputComponent->BindAction("GripLeft", IE_Pressed, this, &AEmpathPlayerCharacter::OnGripLeftPressed);
	NewInputComponent->BindAction("GripLeft", IE_Released, this, &AEmpathPlayerCharacter::OnGripLeftReleased);

	// Click
	NewInputComponent->BindAction("ClickRight", IE_Pressed, this, &AEmpathPlayerCharacter::OnClickRightPressed);
	NewInputComponent->BindAction("ClickRight", IE_Released, this, &AEmpathPlayerCharacter::OnClickRightReleased);
	NewInputComponent->BindAction("ClickLeft", IE_Pressed, this, &AEmpathPlayerCharacter::OnClickLeftPressed);
	NewInputComponent->BindAction("ClickLeft", IE_Released, this, &AEmpathPlayerCharacter::OnClickLeftReleased);

	// Charge
	NewInputComponent->BindAction("ChargeRight", IE_Pressed, this, &AEmpathPlayerCharacter::OnChargeRightPressed);
	NewInputComponent->BindAction("ChargeRight", IE_Released, this, &AEmpathPlayerCharacter::OnChargeRightReleased);
	NewInputComponent->BindAction("ChargeLeft", IE_Pressed, this, &AEmpathPlayerCharacter::OnChargeLeftPressed);
	NewInputComponent->BindAction("ChargeLeft", IE_Released, this, &AEmpathPlayerCharacter::OnChargeLeftReleased);
}

void AEmpathPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Spawn and attach hands if their classes are set
	// Variables
	FTransform SpawnTransform;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	FAttachmentTransformRules SpawnAttachRules(EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		false);

	// Right hand
	if (RightHandClass)
	{
		SpawnTransform = RightMotionController->GetComponentTransform();
		RightHandActor = GetWorld()->SpawnActor<AEmpathHandActor>(RightHandClass, SpawnTransform, SpawnParams);
		RightHandActor->AttachToComponent(RootComponent, SpawnAttachRules);
	}

	// Left hand
	if (LeftHandClass)
	{
		SpawnTransform = RightMotionController->GetComponentTransform();
		LeftHandActor = GetWorld()->SpawnActor<AEmpathHandActor>(LeftHandClass, SpawnTransform, SpawnParams);
		LeftHandActor->AttachToComponent(RootComponent, SpawnAttachRules);
	}

	// Register hands
	if (RightHandActor)
	{
		RightHandActor->RegisterHand(LeftHandActor, this, RightMotionController, EEmpathBinaryHand::Right);
	}
	if (LeftHandActor)
	{
		LeftHandActor->RegisterHand(RightHandActor, this, LeftMotionController, EEmpathBinaryHand::Left);
	}
	OnHandsRegistered();

	// Spawn, attach, and hide the teleport marker
	if (TeleportMarkerClass)
	{
		SpawnTransform = GetTransform();
		TeleportMarker = GetWorld()->SpawnActor<AEmpathTeleportMarker>(TeleportMarkerClass, SpawnTransform, SpawnParams);
		TeleportMarker->OwningCharacter = this;
		TeleportMarker->AttachToComponent(RootComponent, SpawnAttachRules);
	}

	// Cache the player navmeshes
	CacheNavMesh();

	// Ensure we start on the default locomotion mode
	CurrentLocomotionMode = DefaultLocomotionMode;

	// Calculate Left hand pose conditions
	CannonShotData.CalculateLeftHandConditions();

	// Set tracking variables for headset
	EHMDTrackingOrigin::Type TrackingOrigin = EHMDTrackingOrigin::Type::Floor;
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(TrackingOrigin);

	// Handle gravity as appropriate
	if (VRRootReference)
	{
		VRRootReference->SetEnableGravity(bGravityEnabled);
	}
	if (VRMovementReference)
	{
		VRMovementReference->GravityScale = (bGravityEnabled ? 1.0f : 0.0f);
	}
}

void AEmpathPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	TickUpdateHealthRegen(DeltaTime);
	TickUpdateInputAxisEvents();
	TickUpdateTeleportState();
	TickUpdateWalk();
	TickUpdateClimbing();
	TickUpdateGestureState();
}

void AEmpathPlayerCharacter::PossessedBy(AController* NewController)
{
	if (NewController)
	{
		CachedEmpathPlayerCon = Cast<AEmpathPlayerController>(NewController);
	}
	Super::PossessedBy(NewController);
	return;
}

void AEmpathPlayerCharacter::UnPossessed()
{
	CachedEmpathPlayerCon = nullptr;
	Super::UnPossessed();
}

float AEmpathPlayerCharacter::GetDistanceToVR(AActor* OtherActor) const
{
	return (GetVRLocation() - OtherActor->GetActorLocation()).Size();
}

EEmpathTeam AEmpathPlayerCharacter::GetTeamNum_Implementation() const
{
	// We should always return player for VR characters
	return EEmpathTeam::Player;
}


bool AEmpathPlayerCharacter::GetCustomAimLocationOnActor_Implementation(FVector LookOrigin, FVector LookDirection, FVector& OutAimlocation, USceneComponent*& OutAimLocationComponent) const
{
	// Use the damage capsule for the aim location of the player
	if (DamageCapsule)
	{
		OutAimlocation = DamageCapsule->GetComponentLocation();
		OutAimLocationComponent = DamageCapsule;
		return true;
	}

	// If the damage capsule has been destroyed, use the default fallback
	else
	{
		return UEmpathFunctionLibrary::GetAimLocationOnActor(this, LookOrigin, LookDirection, OutAimlocation, OutAimLocationComponent);
	}
}

const FVector AEmpathPlayerCharacter::GetVRScaledHeightLocation(float HeightScale) const
{
	// Get the scaled height
	FVector Height = VRReplicatedCamera->GetRelativeTransform().GetLocation();
	Height.Z *= HeightScale;
	Height = GetTransform().TransformPosition(Height);

	// Get the VR location with the adjusted height
	FVector VRLoc = GetVRLocation();
	VRLoc.Z = Height.Z;
	return VRLoc;
}

FVector AEmpathPlayerCharacter::GetPawnViewLocation() const
{
	if (VRReplicatedCamera)
	{
		return VRReplicatedCamera->GetComponentLocation();
	}
	return Super::GetPawnViewLocation();
}

FRotator AEmpathPlayerCharacter::GetViewRotation() const
{
	if (VRReplicatedCamera)
	{
		return VRReplicatedCamera->GetComponentRotation();
	}
	return Super::GetViewRotation();
}

void AEmpathPlayerCharacter::TeleportToLocation(FVector Destination)
{
	// Do nothing if we are already teleporting
	if (IsTeleporting())
	{
		return;
	}

	// Cache origin
	FVector Origin = GetVRLocation();

	// Update teleport state
	SetTeleportState(EEmpathTeleportState::TeleportingToLocation);
	TeleportActorLocationGoal = GetTeleportLocation(TeleportTraceLocation);
	TeleportLastStartTime = GetWorld()->GetTimeSeconds();
	VRRootReference->SetEnableGravity(false);
	VRMovementReference->SetMovementMode(MOVE_None);

	// Hide the teleport trace
	HideTeleportTrace();

	// Temporarily disable collision
	TArray<AActor*> ControlledActors = GetControlledActors();
	UpdateCollisionForTeleportStart();
	for (AActor* CurrentActor : ControlledActors)
	{
		CurrentActor->SetActorEnableCollision(false);
	}
	
	// Calculate direction vector
	FVector FixedDest = Destination;
	FixedDest.Z += GetVREyeHeight();
	FVector Direction = (FixedDest - Origin).GetSafeNormal();

	// Fire notifies
	OnTeleport();
	ReceiveTeleportToLocation(Origin, Destination, Direction);
	OnTeleportToLocation.Broadcast(this, Origin, Destination, Direction);
 
	return;
}

bool AEmpathPlayerCharacter::CanDie_Implementation() const
{
	return (!bInvincible && !bDead);
}

const bool AEmpathPlayerCharacter::CleanUpAndQueryDamageCauserLogs(float WorldTimeStamp, AActor* QueriedDamageCauser)
{
	// Initialize return value
	bool bFoundQueriedDamageCauser = false;

	// Loop through every entry in the array
	for (int32 Idx = 0; Idx < DamageCauserLogs.Num(); Idx++)
	{
		// Check if the entry has expired
		if (WorldTimeStamp - DamageCauserLogs[Idx].DamageTimestamp > MinTimeBetweenDamageCauserHits)
		{
			// If so, remove it
			DamageCauserLogs.RemoveAt(Idx, 1, false);
		}

		// Else, check if it contains the queried actor
		else if (QueriedDamageCauser && DamageCauserLogs[Idx].DamageCauser == QueriedDamageCauser)
		{
			bFoundQueriedDamageCauser = true;
		}
	}

	// Shrink down the array after looping through
	DamageCauserLogs.Shrink();

	return bFoundQueriedDamageCauser;
}

void AEmpathPlayerCharacter::Die(FHitResult const& KillingHitInfo, FVector KillingHitImpulseDir, const AController* DeathInstigator, const AActor* DeathCauser, const UDamageType* DeathDamageType)
{
	if (CanDie())
	{
		// Update variables
		bDead = true;

		// End stunned state flow
		if (bStunned)
		{
			EndStun();
		}
		else
		{
			GetWorldTimerManager().ClearTimer(StunTimerHandle);
		}

		// Signal Player controller
		if (CachedEmpathPlayerCon)
		{
			CachedEmpathPlayerCon->ReceiveCharacterDeath(KillingHitInfo, KillingHitImpulseDir, DeathInstigator, DeathCauser, DeathDamageType);
		}

		// Signal notifies
		ReceiveDie(KillingHitInfo, KillingHitImpulseDir, DeathInstigator, DeathCauser, DeathDamageType);
		OnDeath.Broadcast(KillingHitInfo, KillingHitImpulseDir, DeathInstigator, DeathCauser, DeathDamageType);
	}
}

float AEmpathPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Scope these functions for the UE4 profiler
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_PlayerTakeDamage);

	// If we're invincible, dead, or this is no damage, do nothing
	if (bInvincible || bDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	// Check if we can take damage from this again
	if (MinTimeBetweenDamageCauserHits > 0.0f  && CleanUpAndQueryDamageCauserLogs(GetWorld()->GetTimeSeconds(), DamageCauser))
	{
		return 0.0f;
	}

	// Cache the initial damage amount
	float AdjustedDamage = DamageAmount;

	// Grab the damage type
	UDamageType const* const DamageTypeCDO = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	UEmpathDamageType const* EmpathDamageTypeCDO = Cast<UEmpathDamageType>(DamageTypeCDO);

	// Friendly fire damage adjustment
	if (EventInstigator || DamageCauser)
	{
		EEmpathTeam const InstigatorTeam = UEmpathFunctionLibrary::GetActorTeam((EventInstigator? EventInstigator : DamageCauser));
		EEmpathTeam const MyTeam = GetTeamNum();
		if (InstigatorTeam == MyTeam)
		{
			// If this damage came from our team and we can't take friendly fire, do nothing
			if (!bCanTakeFriendlyFire)
			{
				return 0.0f;
			}

			// Otherwise, scale the damage by the friendly fire damage multiplier
			else if (EmpathDamageTypeCDO)
			{
				AdjustedDamage *= EmpathDamageTypeCDO->FriendlyFireDamageMultiplier;

				// If our damage was negated, return
				if (AdjustedDamage <= 0.0f)
				{
					return 0.0f;
				}
			}
		}
	}


	// Setup variables for hit direction and info
	FVector HitImpulseDir;
	FHitResult HitInfo;

	// Initialize best hit info
	DamageEvent.GetBestHitInfo(this, (EventInstigator ? EventInstigator->GetPawn() : DamageCauser), HitInfo, HitImpulseDir);

	// Process point damage
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		// Point damage event, pass off to helper function
		FPointDamageEvent* const PointDamageEvent = (FPointDamageEvent*)&DamageEvent;

		// Update hit info
		HitInfo = PointDamageEvent->HitInfo;
		HitImpulseDir = PointDamageEvent->ShotDirection;

		// Allow modification of any damage amount in children or blueprint classes
		AdjustedDamage = ModifyAnyDamage(AdjustedDamage, EventInstigator, DamageCauser, DamageTypeCDO);

		// Allow modification of point damage specifically in children and blueprint classes
		AdjustedDamage = ModifyPointDamage(AdjustedDamage, PointDamageEvent->HitInfo, PointDamageEvent->ShotDirection, EventInstigator, DamageCauser, DamageTypeCDO);
	}

	// Process radial damage
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		// Radial damage event, pass off to helper function
		FRadialDamageEvent* const RadialDamageEvent = (FRadialDamageEvent*)&DamageEvent;

		// Allow modification of any damage amount in children or blueprint classes
		AdjustedDamage = ModifyAnyDamage(AdjustedDamage, EventInstigator, DamageCauser, DamageTypeCDO);

		// Allow modification of specifically radial damage amount in children or blueprint classes
		AdjustedDamage = ModifyRadialDamage(AdjustedDamage, RadialDamageEvent->Origin, RadialDamageEvent->ComponentHits, RadialDamageEvent->Params.InnerRadius, RadialDamageEvent->Params.OuterRadius, EventInstigator, DamageCauser, DamageTypeCDO);
	}
	else
	{
		// Allow modification of any damage amount in children or blueprint classes
		AdjustedDamage = ModifyAnyDamage(AdjustedDamage, EventInstigator, DamageCauser, DamageTypeCDO);
	}

	// Check again if our damage is <= 0
	if (AdjustedDamage <= 0.0f)
	{
		return 0.0f;
	}

	// Fire parent damage command. Do not modify damage amount after this
	const float ActualDamage = Super::TakeDamage(AdjustedDamage, DamageEvent, EventInstigator, DamageCauser);

	// Respond to the damage
	if (ActualDamage > 0.0f)
	{
		// Log this damage causer's damage time
		if (MinTimeBetweenDamageCauserHits > 0.0f)
		{
			FEmpathDamageCauserLog NewDamageCauserLogEntry;
			NewDamageCauserLogEntry.DamageCauser = DamageCauser;
			NewDamageCauserLogEntry.DamageTimestamp = GetWorld()->GetTimeSeconds();
			DamageCauserLogs.Add(NewDamageCauserLogEntry);
		}

		// Process damage to update health and death state
		LastDamageTime = GetWorld()->GetTimeSeconds();
		ProcessFinalDamage(ActualDamage, HitInfo, HitImpulseDir, DamageTypeCDO, EventInstigator, DamageCauser);
		return ActualDamage;
	}
	return 0.0f;
}

float AEmpathPlayerCharacter::ModifyAnyDamage_Implementation(float DamageAmount, const AController* EventInstigator, const AActor* DamageCauser, const class UDamageType* DamageType)
{
	return DamageAmount;
}

float AEmpathPlayerCharacter::ModifyPointDamage_Implementation(float DamageAmount, const FHitResult& HitInfo, const FVector& ShotDirection, const AController* EventInstigator, const AActor* DamageCauser, const class UDamageType* DamageTypeCDO)
{
	return DamageAmount;
}

float AEmpathPlayerCharacter::ModifyRadialDamage_Implementation(float DamageAmount, const FVector& Origin, const TArray<struct FHitResult>& ComponentHits, float InnerRadius, float OuterRadius, const AController* EventInstigator, const AActor* DamageCauser, const class UDamageType* DamageTypeCDO)
{
	return DamageAmount;
}

void AEmpathPlayerCharacter::ProcessFinalDamage_Implementation(const float DamageAmount, FHitResult const& HitInfo, FVector HitImpulseDir, const UDamageType* DamageType, const AController* EventInstigator, const AActor* DamageCauser)
{
	// Decrement health and check for death
	CurrentHealth -= DamageAmount;
	if (CurrentHealth <= 0.0f && CanDie())
	{
		Die(HitInfo, HitImpulseDir, EventInstigator, DamageCauser, DamageType);
		return;
	}

	// If we didn't die, and we can be stunned, log stun damage
	if (CanBeStunned())
	{
		UWorld* const World = GetWorld();
		const UEmpathDamageType* EmpathDamageTyeCDO = Cast<UEmpathDamageType>(DamageType);
		if (EmpathDamageTyeCDO && World)
		{
			// Log the stun damage and check for being stunned
			if (EmpathDamageTyeCDO->StunDamageMultiplier > 0.0f)
			{
				TakeStunDamage(EmpathDamageTyeCDO->StunDamageMultiplier * DamageAmount, EventInstigator, DamageCauser);
			}
		}
		else
		{
			// Default implementation for if we weren't passed an Empath damage type
			TakeStunDamage(DamageAmount, EventInstigator, DamageCauser);
		}
	}

	return;
}

bool AEmpathPlayerCharacter::CanBeStunned_Implementation() const
{
	return (bStunnable
		&& !bDead
		&& !bStunned
		&& GetWorld()->TimeSince(LastStunTime) > StunImmunityTimeAfterStunRecovery);
}

void AEmpathPlayerCharacter::TakeStunDamage(const float StunDamageAmount, const AController* EventInstigator, const AActor* DamageCauser)
{
	// Log stun event
	StunDamageHistory.Add(FEmpathDamageHistoryEvent(StunDamageAmount, -GetWorld()->GetTimeSeconds()));

	// Clean old stun events. They are stored oldest->newest, so we can just iterate to find 
	// the transition point. This plus the next loop will still constitute at most one pass 
	// through the array.

	int32 NumToRemove = 0;
	for (int32 Idx = 0; Idx < StunDamageHistory.Num(); ++Idx)
	{
		FEmpathDamageHistoryEvent& DHE = StunDamageHistory[Idx];
		if (GetWorld()->TimeSince(DHE.EventTimestamp) > StunTimeThreshold)
		{
			NumToRemove++;
		}
		else
		{
			break;
		}
	}

	if (NumToRemove > 0)
	{
		// Remove expired events
		StunDamageHistory.RemoveAt(0, NumToRemove);
	}


	// Remaining history array is now guaranteed to be inside the time threshold.
	// Just add up and stun if necessary. This way we don't have to process on Tick.
	float AccumulatedDamage = 0.f;
	for (FEmpathDamageHistoryEvent& DHE : StunDamageHistory)
	{
		AccumulatedDamage += DHE.DamageAmount;
		if (AccumulatedDamage > StunDamageThreshold)
		{
			BeStunned(EventInstigator, DamageCauser, StunDurationDefault);
			break;
		}
	}
}

void AEmpathPlayerCharacter::BeStunned(const AController* StunInstigator, const AActor* StunCauser, const float StunDuration)
{
	if (CanBeStunned())
	{
		bStunned = true;
		LastStunTime = GetWorld()->GetTimeSeconds();
		ReceiveStunned(StunInstigator, StunCauser, StunDuration);
		if (CachedEmpathPlayerCon)
		{
			CachedEmpathPlayerCon->ReceiveCharacterStunEnd();
		}
		OnStunned.Broadcast(StunInstigator, StunCauser, StunDuration);
	}
}

void AEmpathPlayerCharacter::ReceiveStunned_Implementation(const AController* StunInstigator, const AActor* StunCauser, const float StunDuration)
{
	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	if (StunDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(StunTimerHandle, this, &AEmpathPlayerCharacter::EndStun, StunDuration, false);
	}
}

void AEmpathPlayerCharacter::EndStun()
{
	if (bStunned)
	{
		// Update variables
		bStunned = false;
		GetWorldTimerManager().ClearTimer(StunTimerHandle);
		StunDamageHistory.Empty();

		// Broadcast events and notifies
		ReceiveStunEnd();
		if (CachedEmpathPlayerCon)
		{
			CachedEmpathPlayerCon->ReceiveCharacterStunEnd();
		}
		OnStunEnd.Broadcast();
	}
}

bool AEmpathPlayerCharacter::TraceTeleportLocation(FVector Origin, FVector Direction, float Magnitude, FEmpathTeleportTraceSettings& TraceSettings, bool bInterpolateMagnitude)
{
	// Declare scope cycle for profiler
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_PlayerTraceTeleport);

	// Update the velocity
	if (bInterpolateMagnitude)
	{
		TeleportCurrentVelocity = FMath::VInterpTo(TeleportCurrentVelocity,
			Direction.GetSafeNormal() * Magnitude,
			UEmpathFunctionLibrary::GetUndilatedDeltaTime(this),
			TeleportVelocityLerpSpeed);
	}
	else
	{
		TeleportCurrentVelocity = Direction.GetSafeNormal() * Magnitude;
	}

	// Trace for any teleport locations we may already be inside of,
	// and remove them from our actual teleport trace
	TArray<AEmpathTeleportBeacon*> OverlappingTeleportTargets;
	TArray<FHitResult> BeaconTraceHits;
	const FVector BeaconTraceOrigin = Origin;
	const FVector BeaconTraceEnd = Origin + (Direction.GetSafeNormal() * TeleportBeaconMinDistance);
	FCollisionQueryParams BeaconTraceParams(FName(TEXT("BeaconTrace")), false, this);
	if (GetWorld()->LineTraceMultiByChannel(BeaconTraceHits, BeaconTraceOrigin, BeaconTraceEnd, ECC_Teleport, BeaconTraceParams))
	{
		// Add each overlapping beacon to the trace ignore list
		for (FHitResult BeaconTraceResult : BeaconTraceHits)
		{
			AEmpathTeleportBeacon* OverlappingBeacon = Cast<AEmpathTeleportBeacon>(BeaconTraceResult.Actor.Get());
			if (OverlappingBeacon)
			{
				OverlappingTeleportTargets.Add(OverlappingBeacon);
			}
		}
	}
	
	// Initialize and setup trace parameters
	FPredictProjectilePathParams TeleportTraceParams = FPredictProjectilePathParams(TeleportRadius, Origin, TeleportCurrentVelocity, 3.0f);
	TeleportTraceParams.bTraceWithCollision = true;
	TeleportTraceParams.ObjectTypes.Append(TeleportTraceObjectTypes);
	TeleportTraceParams.ActorsToIgnore.Append(GetControlledActors());
	TeleportTraceParams.ActorsToIgnore.Append(OverlappingTeleportTargets);
	TeleportTraceParams.ActorsToIgnore.Add(this);
	TELEPORT_TRACE_DEBUG_TYPE(TeleportTraceParams)

	// Do the trace and update variables
	FPredictProjectilePathResult TraceResult;
	bool TraceHit = UGameplayStatics::PredictProjectilePath(this, TeleportTraceParams, TraceResult);
	TeleportTraceSplinePositions.Empty(TraceResult.PathData.Num());
	for (const FPredictProjectilePathPointData& PathPoint : TraceResult.PathData)
	{
		TeleportTraceSplinePositions.Add(PathPoint.Location);
	}
	
	// Check if the hit location is valid
	UpdateTeleportTraceState(TraceResult.HitResult, Origin, TraceSettings);
	if (TeleportTraceResult != EEmpathTeleportTraceResult::NotValid)
	{
		TELEPORT_LOC(TraceResult.HitResult.ImpactPoint, 20.0f, FColor::Yellow)
		UE_LOG(LogTeleportTrace, VeryVerbose, TEXT("%s: Teleport trace succeeded."), *GetNameSafe(this));
		TELEPORT_LOC(TeleportTraceLocation, 25.0f, FColor::Green)
		TELEPORT_LINE(Origin, TeleportTraceLocation, FColor::Green)
		return true;
	}
	else
	{
		UE_LOG(LogTeleportTrace, VeryVerbose, TEXT("%s: Teleport trace failed."), *GetNameSafe(this));
		if (TraceHit)
		{
			TELEPORT_LOC_DURATION(TraceResult.HitResult.ImpactPoint, 25.0f, FColor::Red)
			TELEPORT_LINE_DURATION(Origin, TraceResult.HitResult.ImpactPoint, FColor::Red)
		}
		return false;
	}
}

bool AEmpathPlayerCharacter::IsWallClimbLocation(const FVector& ImpactPoint, const FVector& ImpactNormal, FVector& OutScalableLocation) const
{
	// Ensure that the surface is perpendicular to the XY plane
	if (FMath::Abs(ImpactNormal.Z) < 0.1f)
	{
		// Perform a line trace to find information about the wall's top surface
		// Initialize trace variables
		FVector WallTraceEnd = ImpactPoint + (ImpactNormal * -TeleportWallClimbQueryRadius);
		FVector WallTraceStart = FVector(0.0f, 0.0f, 2.0f * TeleportWallClimbReach);
		WallTraceStart += WallTraceEnd;
		FHitResult WallTraceHit;
		FCollisionQueryParams WallTraceParams(FName(TEXT("WallClimbTrace")), false, this);

		// Perform the actual trace
		GetWorld()->LineTraceSingleByChannel(WallTraceHit, WallTraceStart, WallTraceEnd, ECC_WorldStatic, WallTraceParams);

		// Check if there was a blocking hit and the location is within reach
		if (WallTraceHit.bBlockingHit && WallTraceHit.ImpactPoint.Z - WallTraceEnd.Z <= TeleportWallClimbReach)
		{
			// Project the point to navigation
			if (ProjectPointToPlayerNavigation(WallTraceHit.ImpactPoint, OutScalableLocation))
			{
				return true;
			}
		}
	}

	// If the above conditions are not met, return false
	OutScalableLocation = FVector::ZeroVector;
	return false;
}

bool AEmpathPlayerCharacter::UpdateTeleportYaw(FVector Direction, float DeltaYaw)
{
	FRotator TeleportRot = FRotationMatrix::MakeFromX(Direction.GetSafeNormal2D()).Rotator();
	if (DeltaYaw != 0.0f)
	{
		//DeltaYaw = FMath::DivideAndRoundNearest(DeltaYaw, TeleportPivotStep) *TeleportPivotStep;
		FRotator DeltaRot;
		DeltaRot.Yaw = DeltaYaw;
		TeleportRot = FRotator(FQuat(DeltaRot) * FQuat(TeleportRot));
	}

	TeleportYawRotation = TeleportRot;
	return true;
}

EEmpathTeleportTraceResult AEmpathPlayerCharacter::ValidateTeleportTrace(FVector& OutFixedLocation, AEmpathTeleportBeacon* OutTeleportBeacon, AEmpathCharacter* OutTeleportCharacter, FHitResult TeleportHit, FVector TeleportOrigin, const FEmpathTeleportTraceSettings& TraceSettings) const
{
	// Return false if there was an initial overlap or no blocking hit
	if (!TeleportHit.bBlockingHit || TeleportHit.bStartPenetrating)
	{
		UE_LOG(LogTeleportTrace, VeryVerbose, TEXT("%s: Teleport trace began overlapping [%s] "), *GetNameSafe(this), *GetNameSafe(TeleportHit.Actor.Get()));
		OutFixedLocation = FVector::ZeroVector;
		OutTeleportBeacon = nullptr;
		OutTeleportCharacter = nullptr;
		return EEmpathTeleportTraceResult::NotValid;
	}

	// Cache the actor hit
	AActor* HitActor = TeleportHit.Actor.Get();

	// Check if the object is a teleport beacon
	if (TraceSettings.bTraceForBeacons)
	{
		AEmpathTeleportBeacon* HitTeleportBeacon = Cast<AEmpathTeleportBeacon>(HitActor);
		if (HitTeleportBeacon)
		{
			// If we can get a valid position from the teleport beacon, go with that
			if (HitTeleportBeacon->GetBestTeleportLocation(TeleportHit,
				TeleportOrigin,
				OutFixedLocation,
				PlayerNavData,
				PlayerNavFilterClass))
			{
				FVector FixedLocationHolder = OutFixedLocation;
				if (ProjectPointToPlayerNavigation(FixedLocationHolder, OutFixedLocation))
				{
					OutTeleportBeacon = HitTeleportBeacon;
					OutTeleportCharacter = nullptr;
					return EEmpathTeleportTraceResult::TeleportBeacon;
				}
			}
		}
		UE_LOG(LogTeleportTrace, VeryVerbose, TEXT("%s: Teleport trace hit Teleport Beacon [%s], but found no valid teleport locations."), *GetNameSafe(this), *GetNameSafe(HitActor));
	}

	// Next, check if the hit target is an Empath Character
	if (TraceSettings.bTraceForEmpathChars)
	{
		AEmpathCharacter* HitEmpathChar = Cast<AEmpathCharacter>(HitActor);
		if (HitEmpathChar)
		{
			// Check if this actor has a valid teleportation that is on the ground
			if (HitEmpathChar->GetBestTeleportLocation(TeleportHit,
				TeleportOrigin,
				OutFixedLocation,
				PlayerNavData,
				PlayerNavFilterClass))
			{
				FVector FixedLocationHolder = OutFixedLocation;
				if (ProjectPointToPlayerNavigation(FixedLocationHolder, OutFixedLocation))
				{
					OutTeleportBeacon = nullptr;
					OutTeleportCharacter =  HitEmpathChar;
					return EEmpathTeleportTraceResult::Character;
				}
			}
			UE_LOG(LogTeleportTrace, VeryVerbose, TEXT("%s: Teleport trace hit Empath Character [%s], but found no valid teleport locations."), *GetNameSafe(this), *GetNameSafe(HitActor));
		}
	}
	
	// Finally, check for world static objects
	if (TraceSettings.bTraceForWorldStatic)
	{
		// If wall climb teleportation is enabled, check if this is a suitable wall climb location
		if (TraceSettings.bTraceForWallClimb && IsWallClimbLocation(TeleportHit.ImpactPoint, TeleportHit.ImpactNormal, OutFixedLocation))
		{
			OutTeleportBeacon = nullptr;
			OutTeleportCharacter = nullptr;
			return EEmpathTeleportTraceResult::WallClimb;
		}

		// If snapping to min distance is enabled, check if we are within the snap min distance
		if (TeleportTraceSettings.bSnapToMinDistance)
		{
			FVector FeetLoc = GetVRScaledHeightLocation(0.0f);
			if (UEmpathFunctionLibrary::DistanceLessThan(FeetLoc, TeleportHit.Location, TeleportToLocationMinDist))
			{
				// If so, check the foot location
				if (ProjectPointToPlayerNavigation(FeetLoc, OutFixedLocation))
				{
					OutTeleportBeacon = nullptr;
					OutTeleportCharacter = nullptr;
					return EEmpathTeleportTraceResult::Ground;
				}
			}
		}

		// If not, perform the check as normal
		if (ProjectPointToPlayerNavigation(TeleportHit.Location, OutFixedLocation))
		{
			OutTeleportBeacon = nullptr;
			OutTeleportCharacter = nullptr;
			return EEmpathTeleportTraceResult::Ground;
		}
	}


	UE_LOG(LogTeleportTrace, VeryVerbose, TEXT("%s: Teleport trace found no valid locations on hit actor [%s]"), *GetNameSafe(this), *GetNameSafe(HitActor));
	OutFixedLocation = FVector::ZeroVector;
	OutTeleportBeacon = nullptr;
	OutTeleportCharacter = nullptr;
	return EEmpathTeleportTraceResult::NotValid;
}

void AEmpathPlayerCharacter::CacheNavMesh()
{
	UWorld* World = GetWorld();
	if (World)
	{
		for (ANavigationData* CurrNavData : TActorRange<ANavigationData>(World))
		{
			if (GetNameSafe(CurrNavData) == "RecastNavMesh-Player")
			{
				PlayerNavData = CurrNavData;
				break;
			}
		}
	}
}

bool AEmpathPlayerCharacter::ProjectPointToPlayerNavigation(const FVector& Point, FVector& OutPoint) const
{
	if (UEmpathFunctionLibrary::EmpathProjectPointToNavigation(this,
		OutPoint,
		Point,
		PlayerNavData,
		PlayerNavFilterClass,
		TeleportProjectQueryExtent))
	{
		// Ensure the projected point is on the ground
		FHitResult GroundTraceHit;
		const FVector GroundTraceOrigin = OutPoint;
		const FVector GroundTraceEnd = GroundTraceOrigin + FVector(0.0f, 0.0f, -200.0f);
		FCollisionQueryParams GroundTraceParams(FName(TEXT("GroundTrace")), false, this);
		if (GetWorld()->LineTraceSingleByChannel(GroundTraceHit, GroundTraceOrigin, GroundTraceEnd, ECC_WorldStatic, GroundTraceParams))
		{
			OutPoint = GroundTraceHit.ImpactPoint;
			return true;
		}
	}

	OutPoint = FVector::ZeroVector;
	return false;
}

void AEmpathPlayerCharacter::UpdateTeleportTraceState(const FHitResult& TeleportHitResult, const FVector& TeleportOrigin, const FEmpathTeleportTraceSettings& TraceSettings)
{
	// Get the new teleport state
	AEmpathCharacter* HitEmpathCharacter = nullptr;
	AEmpathTeleportBeacon* HitTeleportBeacon = nullptr;
	EEmpathTeleportTraceResult OldTraceResult = TeleportTraceResult;
	TeleportTraceResult = ValidateTeleportTrace(TeleportTraceLocation, HitTeleportBeacon, HitEmpathCharacter, TeleportHitResult, TeleportOrigin, TraceSettings);
	TeleportTraceImpactPoint = TeleportHitResult.ImpactPoint;
	TeleportTraceImpactNormal = TeleportHitResult.ImpactNormal;

	// Call events and notifies depending on the state
	switch (TeleportTraceResult)
	{
	case EEmpathTeleportTraceResult::Character:
	{
		switch (OldTraceResult)
		{
		case EEmpathTeleportTraceResult::Character:
		{
			// Update targeted empath character
			if (HitEmpathCharacter != TargetedTeleportEmpathChar)
			{
				if (TargetedTeleportEmpathChar)
				{
					TargetedTeleportEmpathChar->OnUnTargetedForTeleport();
				}
				OnEmpathCharUntargetedForTeleport(TargetedTeleportEmpathChar);
				TargetedTeleportEmpathChar = HitEmpathCharacter;
				OnEmpathCharTargetedForTeleport(TargetedTeleportEmpathChar);
				if (TargetedTeleportEmpathChar)
				{
					TargetedTeleportEmpathChar->OnTargetedForTeleport();
				}
			}
			break;
		}
		case EEmpathTeleportTraceResult::TeleportBeacon:
		{
			// Clear old teleport beacon
			if (TargetedTeleportBeacon)
			{
				TargetedTeleportBeacon->OnUnTargetedForTeleport();
			}
			OnTeleportBeaconUnTargeted(TargetedTeleportBeacon);
			TargetedTeleportBeacon = nullptr;

			// Update targeted empath character
			TargetedTeleportEmpathChar = HitEmpathCharacter;
			OnEmpathCharTargetedForTeleport(TargetedTeleportEmpathChar);
			if (TargetedTeleportEmpathChar)
			{
				TargetedTeleportEmpathChar->OnTargetedForTeleport();
			}
			break;
		}
		default:
		{
			// Update targeted empath character
			TargetedTeleportEmpathChar = HitEmpathCharacter;
			OnEmpathCharTargetedForTeleport(TargetedTeleportEmpathChar);
			if (TargetedTeleportEmpathChar)
			{
				TargetedTeleportEmpathChar->OnTargetedForTeleport();
			}
			break;
		}
		}
		break;
	}
	case EEmpathTeleportTraceResult::TeleportBeacon:
	{
		switch (OldTraceResult)
		{
		case EEmpathTeleportTraceResult::TeleportBeacon:
		{
			// Update beacon
			if (HitTeleportBeacon != TargetedTeleportBeacon)
			{
				if (TargetedTeleportBeacon)
				{
					TargetedTeleportBeacon->OnUnTargetedForTeleport();
				}
				OnTeleportBeaconUnTargeted(TargetedTeleportBeacon);
				TargetedTeleportBeacon = HitTeleportBeacon;
				OnTeleportBeaconTargeted(TargetedTeleportBeacon);
				if (TargetedTeleportBeacon)
				{
					TargetedTeleportBeacon->OnTargetedForTeleport();
				}
			}
			break;
		}
		case EEmpathTeleportTraceResult::Character:
		{
			// Clear old teleport character
			if (TargetedTeleportEmpathChar)
			{
				TargetedTeleportEmpathChar->OnUnTargetedForTeleport();
			}
			OnEmpathCharUntargetedForTeleport(TargetedTeleportEmpathChar);
			TargetedTeleportEmpathChar = nullptr;

			// Update beacon
			TargetedTeleportBeacon = HitTeleportBeacon;
			OnTeleportBeaconTargeted(HitTeleportBeacon);
			if (TargetedTeleportBeacon)
			{
				TargetedTeleportBeacon->OnTargetedForTeleport();
			}
			break;
		}
		default:
		{
			// Update beacon
			TargetedTeleportBeacon = HitTeleportBeacon;
			OnTeleportBeaconTargeted(HitTeleportBeacon);
			if (TargetedTeleportBeacon)
			{
				TargetedTeleportBeacon->OnTargetedForTeleport();
			}
			break;
		}
		}
		break;
	}
	default:
	{
		// Clear old states
		switch (OldTraceResult)
		{
		case EEmpathTeleportTraceResult::Character:
		{
			if (TargetedTeleportEmpathChar)
			{
				TargetedTeleportEmpathChar->OnUnTargetedForTeleport();
			}
			OnEmpathCharUntargetedForTeleport(TargetedTeleportEmpathChar);
			TargetedTeleportEmpathChar = nullptr;
			break;
		}
		case EEmpathTeleportTraceResult::TeleportBeacon:
		{
			if (TargetedTeleportBeacon)
			{
				TargetedTeleportBeacon->OnUnTargetedForTeleport();
			}
			OnTeleportBeaconUnTargeted(TargetedTeleportBeacon);
			TargetedTeleportBeacon = nullptr;
			break;
		}
		default:
		{
			break;
		}
		}
		break;
	}
	}

	// Call notifies if appropriate
	if (TeleportTraceResult != OldTraceResult)
	{
		OnTeleportTraceResultChanged(TeleportTraceResult, OldTraceResult);
	}

	return;
}

void AEmpathPlayerCharacter::OnTeleportTraceResultChanged(const EEmpathTeleportTraceResult& NewTraceResult, const EEmpathTeleportTraceResult& OldTraceResult)
{
	ReceiveTeleportTraceResultChanged(NewTraceResult, OldTraceResult);
	if (bTeleportTraceVisible && TeleportMarker)
	{
		TeleportMarker->OnTeleportTraceResultChanged(NewTraceResult, OldTraceResult);
	}
	return;
}

void AEmpathPlayerCharacter::ClearHandGrips()
{
	if (RightHandActor)
	{
		RightHandActor->ClearGrip();
	}
	if (LeftHandActor)
	{
		LeftHandActor->ClearGrip();
	}
	return;
}

void AEmpathPlayerCharacter::LocomotionAxisUpDown(float AxisValue)
{
	LocomotionInputAxis.X = AxisValue;
}

void AEmpathPlayerCharacter::LocomotionAxisRightLeft(float AxisValue)
{
	LocomotionInputAxis.Y = AxisValue;
}

void AEmpathPlayerCharacter::TeleportAxisUpDown(float AxisValue)
{
	TeleportInputAxis.X = AxisValue;
}

void AEmpathPlayerCharacter::TeleportAxisRightLeft(float AxisValue)
{
	TeleportInputAxis.Y = AxisValue;
}

void AEmpathPlayerCharacter::DashInDirection(const FVector2D Direction)
{

	// Get the best XY direction
	// X and Y are swapped because in Unreal, Y is forward
	FVector2D DashDirectionLocal = FVector2D::ZeroVector;

	// Dashing left or right
	if (FMath::Abs(Direction.Y) > FMath::Abs(Direction.X))
	{
		// Right
		if (Direction.Y > 0.0f)
		{
			DashDirectionLocal.Y = 1.0f;
		}

		// Left
		else
		{
			DashDirectionLocal.Y = -1.0f;
		}
	}

	// Dashing forward or backward
	else
	{
		// Forward
		if (Direction.X > 0.0f)
		{
			DashDirectionLocal.X = 1.0f;
		}
		else
		{
			DashDirectionLocal.X = -1.0f;
		}
	}

	// Update direction vector based on our camera's facing direction
	FVector OrientedDashDirection = GetOrientedLocomotionAxis(DashDirectionLocal);

	// Perform the trace
	bool bIsTeleportCurrLocValid = TraceTeleportLocation(GetVRLocation(), OrientedDashDirection, DashMagnitude, DashTraceSettings, false);

	// Quality of life feature. If the first dash trace is invalid, we do a single, shorter trace, 
	// to see if we can still move in the desired direction. This will cost us some performance,
	// but will make life easier on the player if they're in a high space with ledges that they would
	// otherwise dash off of
	if (!bIsTeleportCurrLocValid)
	{
		bIsTeleportCurrLocValid = TraceTeleportLocation(GetVRLocation(), OrientedDashDirection, DashMagnitude / 2.0f, DashTraceSettings, false);
	}

	if (bIsTeleportCurrLocValid)
	{
		OnDashSuccess();
		TeleportToLocation(TeleportTraceLocation);
	}
	else
	{
		OnDashFail();
	}

}

void AEmpathPlayerCharacter::OnLocomotionPressed_Implementation()
{
	if (LocomotionControlMode == EEmpathLocomotionControlMode::DefaultAndAltMovement 
		&& CurrentLocomotionMode == EEmpathLocomotionMode::Dash 
		&& CanDash())
	{
		DashInDirection(LocomotionInputAxis);
	}
	
}

void AEmpathPlayerCharacter::OnLocomotionReleased_Implementation()
{

}

void AEmpathPlayerCharacter::OnLocomotionTapped_Implementation()
{
	if (LocomotionControlMode == EEmpathLocomotionControlMode::PressToWalkTapToDash && CanDash())
	{
		DashInDirection(LocomotionInputAxis);
	}
}

void AEmpathPlayerCharacter::OnTeleportTapped_Implementation()
{
	if ((LocomotionControlMode == EEmpathLocomotionControlMode::LeftWalkRightTeleport || LocomotionControlMode == EEmpathLocomotionControlMode::LeftWalkRightTeleportHoldPivot) && CanDash())
	{
		DashInDirection(TeleportInputAxis);
	}
}

void AEmpathPlayerCharacter::OnTeleportPressed_Implementation()
{
	// Attempt to teleport or pivot if in the appropriate movement mode
	if (LocomotionControlMode != EEmpathLocomotionControlMode::LeftWalkRightTeleport
		&& LocomotionControlMode != EEmpathLocomotionControlMode::LeftWalkRightTeleportHoldPivot)
	{
		// Get the best direction and then decide the action
		// X and Y are reversed because in Unreal, X is forward and Y is right

		// Pivoting left or right
		if (FMath::Abs(TeleportInputAxis.Y) > FMath::Abs(TeleportInputAxis.X))
		{
			if (CanPivot())
			{
				// Right
				if (TeleportInputAxis.Y > 0.0f)
				{
					TeleportToRotation(TeleportPivotStep, TeleportRotationSpeed);
				}

				// Left
				else
				{
					TeleportToRotation(TeleportPivotStep * -1.0f, TeleportRotationSpeed);
				}
			}
		}

		// Either teleporting or turning 180 degrees
		else
		{
			// Teleporting
			if (TeleportInputAxis.X > 0.0f)
			{
				if (CanTeleport())
				{
					// Trace the teleport once to update locations
					FVector TeleportOrigin = FVector::ZeroVector;
					FVector TeleportLocalDirection = FVector(1.0f, 0.0f, 0.0f);
					GetTeleportTraceOriginAndDirection(TeleportOrigin, TeleportLocalDirection);
					TraceTeleportLocation(TeleportOrigin, TeleportLocalDirection, TeleportMagnitude, TeleportTraceSettings, false);

					// Begin drawing the trace
					SetTeleportState(EEmpathTeleportState::TracingTeleportLocation);
					ShowTeleportTrace();
				}
			}

			// Pivoting 180 degrees
			else if (CanPivot())
			{
				TeleportToRotation(180.0f, TeleportRotation180Speed);
			}
		}
	}
}

void AEmpathPlayerCharacter::OnTeleportHeld_Implementation()
{
	// Attempt to teleport if in the appropriate movement mode
	if (LocomotionControlMode == EEmpathLocomotionControlMode::LeftWalkRightTeleport && CanTeleport())
	{
		// Trace the teleport once to update locations
		FVector TeleportOrigin = FVector::ZeroVector;
		FVector TeleportLocalDirection = FVector(1.0f, 0.0f, 0.0f);
		GetTeleportTraceOriginAndDirection(TeleportOrigin, TeleportLocalDirection);
		TraceTeleportLocation(TeleportOrigin, TeleportLocalDirection, TeleportMagnitude, TeleportLocAndRotTraceSettings, false);


		// Begin drawing the trace
		SetTeleportState(EEmpathTeleportState::TracingTeleportLocAndRot);
		ShowTeleportTrace();
	}
	
	else if (LocomotionControlMode == EEmpathLocomotionControlMode::LeftWalkRightTeleportHoldPivot)
	{
		// Get the best direction and then decide the action
		// X and Y are reversed because in Unreal, X is forward and Y is right

		// Pivoting left or right
		if (FMath::Abs(TeleportInputAxis.Y) > FMath::Abs(TeleportInputAxis.X))
		{
			if (CanPivot())
			{
				// Right
				if (TeleportInputAxis.Y > 0.0f)
				{
					TeleportToRotation(TeleportPivotStep, TeleportRotationSpeed);
				}

				// Left
				else
				{
					TeleportToRotation(TeleportPivotStep * -1.0f, TeleportRotationSpeed);
				}
			}
		}

		// Either teleporting or turning 180 degrees
		else
		{
			// Teleporting
			if (TeleportInputAxis.X > 0.0f)
			{
				if (CanTeleport())
				{
					// Trace the teleport once to update locations
					FVector TeleportOrigin = FVector::ZeroVector;
					FVector TeleportLocalDirection = FVector(1.0f, 0.0f, 0.0f);
					GetTeleportTraceOriginAndDirection(TeleportOrigin, TeleportLocalDirection);
					TraceTeleportLocation(TeleportOrigin, TeleportLocalDirection, TeleportMagnitude, TeleportTraceSettings, false);

					// Begin drawing the trace
					SetTeleportState(EEmpathTeleportState::TracingTeleportLocation);
					ShowTeleportTrace();
				}
			}

			// Pivoting 180 degrees
			else if (CanPivot())
			{
				TeleportToRotation(180.0f, TeleportRotation180Speed);
			}
		}
	}
}

void AEmpathPlayerCharacter::OnTeleportReleased_Implementation()
{
	if (TeleportState == EEmpathTeleportState::TracingTeleportLocation)
	{
		if (TeleportTraceResult != EEmpathTeleportTraceResult::NotValid && UEmpathFunctionLibrary::DistanceGreaterThan(GetVRScaledHeightLocation(0.0f), TeleportTraceLocation, TeleportToLocationMinDist))
		{
			TeleportToLocation(TeleportTraceLocation);
		}
		else
		{
			SetTeleportState(EEmpathTeleportState::NotTeleporting);
			HideTeleportTrace();
		}

	}
	else if (TeleportState == EEmpathTeleportState::TracingTeleportLocAndRot)
	{
		if (TeleportTraceResult != EEmpathTeleportTraceResult::NotValid && UEmpathFunctionLibrary::DistanceGreaterThan(GetVRScaledHeightLocation(0.0f), TeleportTraceLocation, TeleportToLocationMinDist))

		{
			bTeleportToRotationQueued = true;
			TeleportToLocation(TeleportTraceLocation);
		}
		else
		{
			SetTeleportState(EEmpathTeleportState::NotTeleporting);
			HideTeleportTrace();
			bTeleportToRotationQueued = true;
			ProcessQueuedTeleportToRot();
		}
	}
}

void AEmpathPlayerCharacter::OnAltMovementPressed()
{
	bAltMovementPressed = true;
	if (DefaultLocomotionMode == EEmpathLocomotionMode::Dash)
	{
		CurrentLocomotionMode = EEmpathLocomotionMode::Walk;
	}
	else
	{
		CurrentLocomotionMode = EEmpathLocomotionMode::Dash;
	}
	ReceiveAltMovementPressed();
}

void AEmpathPlayerCharacter::ReceiveAltMovementPressed_Implementation()
{

}

void AEmpathPlayerCharacter::OnAltMovementReleased()
{
	bAltMovementPressed = false;
	if (DefaultLocomotionMode == EEmpathLocomotionMode::Dash)
	{
		CurrentLocomotionMode = EEmpathLocomotionMode::Dash;
	}
	else
	{
		CurrentLocomotionMode = EEmpathLocomotionMode::Walk;
	}
	ReceiveAltMovementReleased();
}

void AEmpathPlayerCharacter::ReceiveAltMovementReleased_Implementation()
{

}

void AEmpathPlayerCharacter::TickUpdateInputAxisEvents()
{
	// Locomotion
	// Pressed begin
	if (LocomotionInputAxis.Size() >= InputAxisEventThreshold)
	{
		if (!bLocomotionPressed)
		{
			bLocomotionPressed = true;
			LocomotionLastPressedTime = GetWorld()->GetRealTimeSeconds();
			OnLocomotionPressed();
		}
	}

	// Released begin
	else
	{
		if (bLocomotionPressed)
		{
			bLocomotionPressed = false;

			// Check for tapped
			if (UEmpathFunctionLibrary::GetRealTimeSince(this, LocomotionLastPressedTime) <= InputAxisTappedThreshold
				&& UEmpathFunctionLibrary::GetRealTimeSince(this, LocomotionLastReleasedTime) >= InputAxisTappedDeadTime)
			{
				OnLocomotionTapped();
			}
			else
			{
				LocomotionLastReleasedTime = GetWorld()->GetRealTimeSeconds();
			}
			OnLocomotionReleased();
		}
	}

	// Teleport
	// Pressed begin
	if (TeleportInputAxis.Size() >= InputAxisEventThreshold)
	{
		if (!bTeleportPressed)
		{
			bTeleportPressed = true;
			TeleportLastPressedTime = GetWorld()->GetRealTimeSeconds();
			OnTeleportPressed();
		}
		else if (!bTeleportHeld && UEmpathFunctionLibrary::GetRealTimeSince(this, TeleportLastPressedTime) > InputAxisTappedThreshold)
		{
			bTeleportHeld = true;
			OnTeleportHeld();
		}
	}
	// Released begin
	else
	{
		if (bTeleportPressed)
		{
			bTeleportPressed = false;

			// Check for dash
			if (UEmpathFunctionLibrary::GetRealTimeSince(this, TeleportLastPressedTime) <= InputAxisTappedThreshold
				&& UEmpathFunctionLibrary::GetRealTimeSince(this, TeleportLastReleasedTime) >= InputAxisTappedDeadTime)
			{
				OnTeleportTapped();
			}
			else
			{
				TeleportLastReleasedTime = GetWorld()->GetRealTimeSeconds();
			}
			bTeleportHeld = false;
			OnTeleportReleased();
		}
	}
}

bool AEmpathPlayerCharacter::CanPivot_Implementation() const
{
	return (bPivotEnabled && !bDead && !bStunned && !IsTeleporting());
}

bool AEmpathPlayerCharacter::CanDash_Implementation() const
{
	return (bDashEnabled && !bDead && !bStunned && !bClimbing && !IsTeleporting());
}

bool AEmpathPlayerCharacter::CanTeleport_Implementation() const
{
	if (bTeleportEnabled && !bDead && !bStunned && !IsTeleporting())
	{
		// Check if our teleport hand is gripping an object
		EEmpathGripType TeleportHandGripState = EEmpathGripType::NoGrip;
		if (TeleportHand == EEmpathBinaryHand::Left)
		{
			if (LeftHandActor)
			{
				TeleportHandGripState = LeftHandActor->GetGripState();
			}
		}
		else
		{
			if (RightHandActor)
			{
				TeleportHandGripState = RightHandActor->GetGripState();
			}
		}
		// Only return true if we are either holding a pickup or not holding on object in the teleport hand
		if (TeleportHandGripState == EEmpathGripType::NoGrip || TeleportHandGripState == EEmpathGripType::Pickup)
		{
			return true;
		}
	}
	return false;
}

bool AEmpathPlayerCharacter::CanWalk_Implementation() const
{
	return (bWalkEnabled && !bDead && !bStunned && !bClimbing && !IsTeleporting());
}

void AEmpathPlayerCharacter::TickUpdateHealthRegen(float DeltaTime)
{
	// Regen health if appropriate
	if (bHealthRegenActive && GetWorld()->TimeSince(LastDamageTime) > HealthRegenDelay)
	{
		CurrentHealth = FMath::Min((CurrentHealth + (HealthRegenRate * MaxHealth * DeltaTime)), MaxHealth);
	}
}

void AEmpathPlayerCharacter::TickUpdateTeleportState()
{
	// Update our teleport state as appropriate
	switch (TeleportState)
	{
	case EEmpathTeleportState::NotTeleporting:
	{
		break;
	}

	case EEmpathTeleportState::TracingTeleportLocation:
	{
		if (CanTeleport())
		{
			// Get the correct location and direction depending on the teleport hand
			FVector TeleportLocalOrigin;
			FVector TeleportLocalDirection = FVector(1.0f, 0.0f, 0.0f);
			GetTeleportTraceOriginAndDirection(TeleportLocalOrigin, TeleportLocalDirection);

			// Perform the actual trace
			TraceTeleportLocation(TeleportLocalOrigin, TeleportLocalDirection, TeleportMagnitude, TeleportTraceSettings, true);
			UpdateTeleportYaw(TeleportLocalDirection, 0.0f);
			UpdateTeleportTrace();
			OnTickTeleportStateUpdated(TeleportState);
		}
		else
		{
			SetTeleportState(EEmpathTeleportState::NotTeleporting);
		}


		break;
	}
	case EEmpathTeleportState::TracingTeleportLocAndRot:
	{
		if (CanTeleport())
		{
			// Get the correct location and direction depending on the teleport hand
			FVector TeleportLocationOrigin;
			FVector TeleportLocalDirection = FVector(1.0f, 0.0f, 0.0f);
			GetTeleportTraceOriginAndDirection(TeleportLocationOrigin, TeleportLocalDirection);

			// Perform the actual trace
			TraceTeleportLocation(TeleportLocationOrigin, TeleportLocalDirection, TeleportMagnitude, TeleportLocAndRotTraceSettings, true);

			// Calculate delta yaw
			float DeltaYaw = FMath::RadiansToDegrees(FMath::Atan2(TeleportInputAxis.Y, TeleportInputAxis.X));

			UpdateTeleportYaw(TeleportLocalDirection, DeltaYaw);
			UpdateTeleportTrace();
			OnTickTeleportStateUpdated(TeleportState);
		}
		else
		{
			SetTeleportState(EEmpathTeleportState::NotTeleporting);
		}
		break;
	}

	case EEmpathTeleportState::TeleportingToLocation:
	{
		// Check if we have timed out
		if (GetWorld()->TimeSince(TeleportLastStartTime) > TeleportTimoutTime)
		{
			bTeleportToRotationQueued = false;
			OnTeleportEnd();
			OnTeleportToLocationFail();
			OnTickTeleportStateUpdated(TeleportState);
			break;
		}

		// Cache the distance between the current and goal locations
		FVector CurrLoc = GetActorLocation();
		float CurrDist = (TeleportActorLocationGoal - CurrLoc).Size();

		// Lerp towards the target location
		float DeltaSeconds = UEmpathFunctionLibrary::GetUndilatedDeltaTime(this);
		SetActorLocation(FMath::VInterpConstantTo(CurrLoc, TeleportActorLocationGoal, DeltaSeconds, TeleportMovementSpeed));

		// Check if we have arrived at or overshot the target location
		float NewDist = (TeleportActorLocationGoal - GetActorLocation()).Size();
		if (NewDist <= TeleportDistTolerance || NewDist > CurrDist)
		{
			SetActorLocation(TeleportActorLocationGoal);
			OnTeleportEnd();
			OnTeleportToLocationSuccess();
		}
		OnTickTeleportStateUpdated(TeleportState);
		break;
	}

	case EEmpathTeleportState::TeleportingToRotation:
	{
		// Cache and setup variables
		float OldDeltaYaw = TeleportRemainingDeltaYaw;

		// Get the current delta yaw to apply on this frame by interping the remaining delta towards 0 and finding the difference
		float DeltaSeconds = UEmpathFunctionLibrary::GetUndilatedDeltaTime(this);
		TeleportRemainingDeltaYaw = FMath::FInterpConstantTo(TeleportRemainingDeltaYaw, 0.0f, DeltaSeconds, TeleportCurrentRotationSpeed);
		float DeltaYawToApply = OldDeltaYaw - TeleportRemainingDeltaYaw;

		// Ensure we did not overshoot the target
		if (FMath::Abs(OldDeltaYaw) < FMath::Abs(TeleportRemainingDeltaYaw))
		{
			TeleportRemainingDeltaYaw = 0.0f;
			DeltaYawToApply = OldDeltaYaw;
		}

		// Apply the delta yaw to our rotation
		FRotator DeltaRotation = FRotator(0.0f, DeltaYawToApply, 0.0f);
		AddActorWorldRotationVR(DeltaRotation, true);

		// End the rotation as appropriate
		if (FMath::IsNearlyZero(TeleportRemainingDeltaYaw))
		{
			SetTeleportState(EEmpathTeleportState::EndingTeleport);
			OnTeleportEnd();
			OnTeleportToRotationEnd();
		}
		OnTickTeleportStateUpdated(TeleportState);
		break;
	}

	case EEmpathTeleportState::EndingTeleport:
	{
		OnTickTeleportStateUpdated(TeleportState);
		break;
	}

	default:
	{
		break;
	}
	}
}

bool AEmpathPlayerCharacter::IsTeleporting() const
{
	return (TeleportState != EEmpathTeleportState::NotTeleporting
		&& TeleportState != EEmpathTeleportState::TracingTeleportLocation
		&& TeleportState != EEmpathTeleportState::TracingTeleportLocAndRot);
}

void AEmpathPlayerCharacter::SetTeleportState(EEmpathTeleportState NewTeleportState)
{
	if (NewTeleportState != TeleportState)
	{
		EEmpathTeleportState OldTeleportState = TeleportState;
		TeleportState = NewTeleportState;

		// Hide the teleport trace if appropriate
		if (OldTeleportState == EEmpathTeleportState::TracingTeleportLocation
			|| OldTeleportState == EEmpathTeleportState::TracingTeleportLocAndRot)
		{
			HideTeleportTrace();
		}


		OnTeleportStateChanged(OldTeleportState, TeleportState);
	}
}

void AEmpathPlayerCharacter::OnTeleportEnd()
{
	VRRootReference->SetEnableGravity(bGravityEnabled);
	SetTeleportState(EEmpathTeleportState::EndingTeleport);
	VRMovementReference->SetMovementMode(MOVE_Walking);
	UpdateCollisionForTeleportEnd();
	
	TArray<AActor*> ControlledActors(GetControlledActors());
	for (AActor* CurrentActor : ControlledActors)
	{
		CurrentActor->SetActorEnableCollision(true);
	}
	if (LeftHandActor)
	{
		LeftHandActor->OnOwningPlayerTeleportEnd();
	}
	if (RightHandActor)
	{
		RightHandActor->OnOwningPlayerTeleportEnd();
	}
	ReceiveTeleportEnd();
}

void AEmpathPlayerCharacter::ReceiveTeleportEnd_Implementation()
{
	// By default, just end the teleport state
	SetTeleportState(EEmpathTeleportState::NotTeleporting);

	ProcessQueuedTeleportToRot();

}

void AEmpathPlayerCharacter::GetTeleportTraceOriginAndDirection(FVector& Origin, FVector& Direction)
{
	// Right hand
	if (TeleportHand == EEmpathBinaryHand::Right)
	{
		if (RightHandActor)
		{
			Origin = RightHandActor->GetTeleportOrigin();
			Direction = RightHandActor->GetTeleportDirection(FVector(1.0f, 0.0f, 0.0f));
		}

		else
		{
			Origin = RightMotionController->GetComponentLocation();
			Direction = RightMotionController->GetForwardVector();
		}
	}

	// Left hand
	else
	{
		if (LeftHandActor)
		{
			Origin = LeftHandActor->GetTeleportOrigin();
			Direction = LeftHandActor->GetTeleportDirection(Direction);
		}

		else
		{
			Origin = LeftMotionController->GetComponentLocation();
			Direction = LeftMotionController->GetForwardVector();
		}
	}
}

void AEmpathPlayerCharacter::TeleportToRotation(float DeltaYaw, float RotationSpeed)
{
	// Do nothing if we are already teleporting
	if (IsTeleporting())
	{
		return;
	}

	// Update delta yaw and teleport state
	TeleportRemainingDeltaYaw = DeltaYaw;
	SetTeleportState(EEmpathTeleportState::TeleportingToRotation);
	TeleportCurrentRotationSpeed = RotationSpeed;

	// Ensure we don't get stuck on 0 rotation
	if (TeleportCurrentRotationSpeed < 1.0f)
	{
		TeleportCurrentRotationSpeed = TeleportRotationSpeed;
	}

	// Fire notifies
	OnTeleport();
	OnTeleportToRotation(DeltaYaw);

}

TArray<AActor*> AEmpathPlayerCharacter::GetControlledActors()
{
	TArray<AActor*> ControlledActors;
	if (RightHandActor)
	{
		ControlledActors.Add(RightHandActor);
		if (RightHandActor->HeldObject)
		{
			ControlledActors.Add(RightHandActor->HeldObject);
		}
	}
	if (LeftHandActor)
	{
		ControlledActors.Add(LeftHandActor);
		if (LeftHandActor->HeldObject)
		{
			ControlledActors.Add(LeftHandActor->HeldObject);
		}
	}
	return ControlledActors;
}

void AEmpathPlayerCharacter::ShowTeleportTrace()
{
	if (!bTeleportTraceVisible)
	{
		bTeleportTraceVisible = true;
		OnShowTeleportTrace();
		if (TeleportMarker)
		{
			TeleportMarker->ShowTeleportMarker(TeleportTraceResult);
		}
	}
}

void AEmpathPlayerCharacter::UpdateTeleportTrace()
{
	if (TeleportMarker)
	{
		TeleportMarker->UpdateMarkerLocationAndYaw(TeleportTraceLocation, TeleportTraceImpactPoint, TeleportTraceImpactNormal, TeleportYawRotation, TeleportTraceResult, TargetedTeleportEmpathChar, TargetedTeleportBeacon);
	}
	OnUpdateTeleportTrace();
}

void AEmpathPlayerCharacter::HideTeleportTrace()
{
	if (bTeleportTraceVisible)
	{
		bTeleportTraceVisible = false;
		OnHideTeleportTrace();
		if (TeleportMarker)
		{
			TeleportMarker->HideTeleportMarker();
		}

	}
}

void AEmpathPlayerCharacter::UpdateCollisionForTeleportStart()
{
	// Update root reference collision
	VRRootReference->SetCollisionProfileName(FEmpathCollisionProfiles::PawnOverlapOnlyTrigger);

	// Disable collision for capsule
	DamageCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OnUpdateCollisionForTeleportStart();
}

void AEmpathPlayerCharacter::UpdateCollisionForTeleportEnd()
{
	// Update root reference collision
	VRRootReference->SetCollisionProfileName(FEmpathCollisionProfiles::PlayerRoot);

	// Enable collision for damage capsule
	DamageCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	OnUpdateCollisionForTeleportEnd();

}

void AEmpathPlayerCharacter::SetDefaultLocomotionMode(EEmpathLocomotionMode LocomotionMode)
{
	if (bAltMovementPressed)
	{
		CurrentLocomotionMode = DefaultLocomotionMode;
	}
	else
	{
		CurrentLocomotionMode = LocomotionMode;
	}

	DefaultLocomotionMode = LocomotionMode;
}

void AEmpathPlayerCharacter::TickUpdateWalk()
{
	// Check for valid input
	float MoveScale = LocomotionInputAxis.Size();
	UVRCharacterMovementComponent* CMC = Cast<UVRCharacterMovementComponent>(GetMovementComponent());
	if (CMC && CanWalk() && MoveScale > InputAxisLocomotionWalkThreshold
		&& (LocomotionControlMode != EEmpathLocomotionControlMode::DefaultAndAltMovement
		|| (CurrentLocomotionMode == EEmpathLocomotionMode::Walk && LocomotionControlMode == EEmpathLocomotionControlMode::DefaultAndAltMovement)))
	{
		// Call notify if appropriate
		if (!bWalking)
		{
			bWalking = true;
			OnWalkBegin();
		}

		// Convert the remaining magnitude of the analog stick into a scale from 0 to 1
		float MaxScale = 1.0f - InputAxisLocomotionWalkThreshold;
		MoveScale -= InputAxisLocomotionWalkThreshold;
		MoveScale /= InputAxisLocomotionWalkThreshold;
		FVector2D ScaledInput = LocomotionInputAxis.GetSafeNormal() * MoveScale;

		// Add the input to our movement component
		CMC->AddInputVector(GetOrientedLocomotionAxis(ScaledInput));
	}

	else if (bWalking)
	{
		bWalking = false;
		OnWalkEnd();
	}
}

FVector AEmpathPlayerCharacter::GetOrientedLocomotionAxis(const FVector2D InputAxis) const
{
	// Convert to a 3 dimensional vector
	FVector ThreeDInputAxis = FVector(InputAxis.X, InputAxis.Y, 0.0f);

	// Find the correct orientation
	EEmpathOrientation OrientationToUse;
	if (CurrentLocomotionMode == EEmpathLocomotionMode::Dash)
	{
		OrientationToUse = DashOrientation;
	}
	else
	{
		OrientationToUse = WalkOrientation;
	}

	// Get the appropriate transform according to the vector
	FTransform DirectionTransform;
	if (OrientationToUse == EEmpathOrientation::Head)
	{
		DirectionTransform = VRReplicatedCamera->GetComponentTransform();
	}
	else
	{
		// Switch depending on witch hand is walk
		// It will always be whichever one is not teleport
		if (TeleportHand == EEmpathBinaryHand::Right)
		{
			if (LeftHandActor)
			{
				DirectionTransform = LeftHandActor->GetKinematicVelocityComponent()->GetComponentTransform();
			}
			else 
			{
				DirectionTransform = LeftMotionController->GetComponentTransform();
			}
		}
		else
		{
			if (RightHandActor)
			{
				DirectionTransform = RightHandActor->GetKinematicVelocityComponent()->GetComponentTransform();
			}
			else
			{
				DirectionTransform = RightMotionController->GetComponentTransform();
			}
		}
	}

	ThreeDInputAxis = DirectionTransform.TransformVectorNoScale(ThreeDInputAxis);
	ThreeDInputAxis.Z = 0.0f;
	return ThreeDInputAxis;
}

void AEmpathPlayerCharacter::OnGripRightPressed()
{
	if (!bGripRightPressed)
	{
		bGripRightPressed = true;
		if (RightHandActor)
		{
			RightHandActor->OnGripPressed();
		}
		ReceiveGripRightPressed();
	}
	return;
}

void AEmpathPlayerCharacter::OnGripRightReleased()
{
	if (bGripRightPressed)
	{
		bGripRightPressed = false;
		ReceiveGripRightReleased();
		if (RightHandActor)
		{
			RightHandActor->OnGripReleased();
		}
	}
	return;
}

void AEmpathPlayerCharacter::OnGripLeftPressed()
{
	if (!bGripLeftPressed)
	{
		bGripLeftPressed = true;
		if (LeftHandActor)
		{
			LeftHandActor->OnGripPressed();
		}
		ReceiveGripLeftPressed();
	}
	return;
}

void AEmpathPlayerCharacter::OnGripLeftReleased()
{
	if (bGripLeftPressed)
	{
		bGripLeftPressed = false;
		if (LeftHandActor)
		{
			LeftHandActor->OnGripReleased();
		}
		ReceiveGripLeftReleased();
	}
	return;
}

void AEmpathPlayerCharacter::OnClickRightPressed()
{
	if (!bClickRightPressed)
	{
		bClickRightPressed = true;
		ReceiveClickRightPressed();
		if (RightHandActor)
		{
			RightHandActor->OnClickPressed();
		}
	}
	return;
}

void AEmpathPlayerCharacter::OnClickRightReleased()
{
	if (bClickRightPressed)
	{
		bClickRightPressed = false;
		ReceiveClickRightReleased();
		if (RightHandActor)
		{
			RightHandActor->OnClickReleased();
		}
	}
	return;
}



void AEmpathPlayerCharacter::OnClickLeftPressed()
{
	if (!bClickLeftPressed)
	{
		bClickLeftPressed = true;
		ReceiveClickLeftPressed();
		if (LeftHandActor)
		{
			LeftHandActor->OnClickPressed();
		}
	}
	return;
}

void AEmpathPlayerCharacter::OnClickLeftReleased()
{
	if (bClickLeftPressed)
	{
		bClickLeftPressed = false;
		ReceiveClickLeftReleased();
		if (LeftHandActor)
		{
			LeftHandActor->OnClickReleased();
		}
	}
}

void AEmpathPlayerCharacter::OnChargeRightPressed()
{
	if (!bChargeRightPressed)
	{
		bChargeRightPressed = true;
		ReceiveChargeRightPressed();
		if (RightHandActor)
		{
			RightHandActor->OnChargePressed();
		}
	}
	return;
}

void AEmpathPlayerCharacter::OnChargeRightReleased()
{
	if (bChargeRightPressed)
	{
		bChargeRightPressed = false;
		ReceiveChargeRightReleased();
		if (RightHandActor)
		{
			RightHandActor->OnChargeReleased();
		}
	}
	return;
}


void AEmpathPlayerCharacter::OnChargeLeftPressed()
{
	if (!bChargeLeftPressed)
	{
		bChargeLeftPressed = true;
		ReceiveChargeLeftPressed();
		if (LeftHandActor)
		{
			LeftHandActor->OnChargePressed();
		}
	}
	return;
}

void AEmpathPlayerCharacter::OnChargeLeftReleased()
{
	if (bChargeLeftPressed)
	{
		bChargeLeftPressed = false;
		ReceiveChargeLeftReleased();
		if (LeftHandActor)
		{
			LeftHandActor->OnChargeReleased();
		}
	}
	return;
}

void AEmpathPlayerCharacter::ProcessQueuedTeleportToRot()
{
	// Start a teleport to rotation if one was queued and the delta angle is large enough to warrant it
	if (bTeleportToRotationQueued)
	{
		bTeleportToRotationQueued = false;
		float DeltaRot = FMath::FindDeltaAngleDegrees(GetVRRotation().Yaw, TeleportYawRotation.Yaw);
		if (FMath::Abs(DeltaRot) >= TeleportRotationAndLocationMinDeltaAngle)
		{
			TeleportToRotation(DeltaRot, TeleportRotationSpeed);
		}
	}
}

void AEmpathPlayerCharacter::SetGripEnabled(const bool bNewEnabled)
{
	if (bNewEnabled != bGripEnabled)
	{
		bGripEnabled = bNewEnabled;

		// Release any grips if we can no longer grip
		if (bGripEnabled)
		{
			OnGripEnabled();
		}
		else
		{
			OnGripDisabled();
		}
	}
	return;
}

void AEmpathPlayerCharacter::OnGripEnabled()
{
	ReceiveGripEnabled();
	if (RightHandActor)
	{
		RightHandActor->OnGripEnabled();
	}
	if (LeftHandActor)
	{
		LeftHandActor->OnGripEnabled();
	}
	return;
}

void AEmpathPlayerCharacter::OnGripDisabled()
{
	ReceiveGripDisabled();
	if (RightHandActor)
	{
		RightHandActor->ReceiveGripDisabled();
	}
	if (LeftHandActor)
	{
		LeftHandActor->ReceiveGripDisabled();
	}
	return;
}

void AEmpathPlayerCharacter::SetClimbEnabled(const bool bNewEnabled)
{
	if (bNewEnabled != bClimbEnabled)
	{
		bClimbEnabled = bNewEnabled;
		if (bNewEnabled)
		{
			OnClimbEnabled();
		}
		else
		{
			OnClimbDisabled();
		}
	}
}

void AEmpathPlayerCharacter::OnClimbEnabled()
{
	ReceiveClimbEnabled();
	if (RightHandActor)
	{
		RightHandActor->ReceiveClimbEnabled();
	}
	if (LeftHandActor)
	{
		LeftHandActor->ReceiveClimbEnabled();
	}
	return;
}

void AEmpathPlayerCharacter::OnClimbDisabled()
{
	ReceiveClimbDisabled();
	if (RightHandActor)
	{
		RightHandActor->OnClickDisabled();
	}
	if (LeftHandActor)
	{
		LeftHandActor->OnClimbDisabled();
	}
	return;
}

bool AEmpathPlayerCharacter::CanClimb_Implementation() const
{
	return (bClimbEnabled);
}


void AEmpathPlayerCharacter::SetClimbingGrip(AEmpathHandActor* Hand, UPrimitiveComponent* GrippedComponent, FVector GripOffset)
{
	if (Hand && GrippedComponent && CanClimb() && VRMovementReference)
	{
		ClimbHand = Hand;
		ClimbGrippedComponent = GrippedComponent;
		ClimbGripOffset = GripOffset;

		if (!bClimbing)
		{
			VRMovementReference->SetClimbingMode(true);
			bClimbing = true;
			OnClimbStart();
		}
	}
}

void AEmpathPlayerCharacter::ClearClimbingGrip()
{
	ClimbHand = nullptr;
	ClimbGrippedComponent = nullptr;
	ClimbGripOffset = FVector::ZeroVector;
	bClimbing = false;
	if (VRMovementReference)
	{
		VRMovementReference->SetClimbingMode(false);
	}
	OnClimbEnd();
}

void AEmpathPlayerCharacter::TickUpdateClimbing()
{
	if (bClimbing)
	{
		if (ClimbHand)
		{
			// Apply movement as the offset of where our hand was in relation to the gripped component, versus where it is now
			if (ClimbGrippedComponent && VRMovementReference)
			{
				FVector Offset = ClimbHand->GetActorLocation() - ClimbGrippedComponent->GetComponentTransform().TransformPosition(ClimbGripOffset);
				Offset *= -1.0f;
				VRMovementReference->AddCustomReplicatedMovement(Offset);

			}
			else
			{
				ClimbHand->OnGripReleased();
				ClearClimbingGrip();
			}
		}
		else
		{
			ClearClimbingGrip();
		}
	}
}

void AEmpathPlayerCharacter::OnNewChargedState(const bool bNewCharged, const EEmpathBinaryHand Hand)
{
	ReceiveNewChargedState(bNewCharged, Hand);
	OnNewChargedStateDelegate.Broadcast(bNewCharged, Hand);
}

void AEmpathPlayerCharacter::OnNewBlockingState(const bool bNewBlockingState, const EEmpathBinaryHand Hand)
{
	ReceiveNewBlockingState(bNewBlockingState, Hand);
	OnNewBlockingStateDelegate.Broadcast(bNewBlockingState, Hand);
}

void AEmpathPlayerCharacter::OnGestureStateChanged(const EEmpathGestureType OldGestureState,
	const EEmpathGestureType NewGestureState,
	const EEmpathBinaryHand Hand)
{
	ReceiveGestureStateChanged(OldGestureState, NewGestureState, Hand);
	OnGestureStateChangedDelegate.Broadcast(OldGestureState, NewGestureState, Hand);
}

bool AEmpathPlayerCharacter::CanGrip_Implementation() const
{
	return (bGripEnabled && !bDead && !bStunned && !IsTeleporting());
}

void AEmpathPlayerCharacter::SetGestureCastingEnabled(const bool bNewEnabled)
{
	if (RightHandActor)
	{
		RightHandActor->SetGestureCastingEnabled(bNewEnabled);
	}
	if (LeftHandActor)
	{
		LeftHandActor->SetGestureCastingEnabled(bNewEnabled);
	}
}

void AEmpathPlayerCharacter::TickUpdateGestureState()
{
	// Scope process for the UE4 profiler
	SCOPE_CYCLE_COUNTER(STAT_EMPATH_PlayerGestureRecognition);

	// Update condition checks
	if (RightHandActor)
	{
		RightHandActor->UpdateGestureConditionChecks();
	}
	if (LeftHandActor)
	{
		LeftHandActor->UpdateGestureConditionChecks();
	}

	// Update state depending on the current casting pose
	switch (CastingPose)
	{
	case EEmpathCastingPose::NoPose:
	{
		// Check if we are in or activating a one-handed gesture
		// If not, check if we are able to enter another casting pose
		bool bRightHandGestureActive = UpdateOneHandGesture(RightHandActor);
		bool bLeftHandGestureActive = UpdateOneHandGesture(LeftHandActor);
		if (!bRightHandGestureActive && !bLeftHandGestureActive)
		{
			AttemptEnterStaticCastingPose();
		}

		return;
		break;
	}
	case EEmpathCastingPose::CannonShotStatic:
	{
		// If the pose is valid
		if (CanGestureCast() && LeftHandActor->IsPowerCharged() && RightHandActor->IsPowerCharged() && CanCannonShot())
		{
			// Attempt to enter dynamic state
			if (CannonShotData.DynamicData.AreEntryConditionsMet(RightHandActor->GestureConditionCheck,
				RightHandActor->FrameConditionCheck, 
				LeftHandActor->GestureConditionCheck,
				LeftHandActor->FrameConditionCheck))
			{
				// Start activating the dynamic gesture if we have not already
				if (CannonShotData.DynamicData.GestureState.ActivationState != EEmpathActivationState::Activating)
				{
					// Reset the casting pose
					//ResetPoseEntryState(EEmpathCastingPose::CannonShotDynamic);

					// Update the entry state
					CannonShotData.DynamicData.GestureState.ActivationState = EEmpathActivationState::Activating;

					// Cache entry variables
					CannonShotData.DynamicData.GestureState.LastEntryStartTime = GetWorld()->GetRealTimeSeconds();
					RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastEntryStartLocation = RightHandActor->GetKinematicVelocityComponent()->GetComponentLocation();
					RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastEntryStartRotation = RightHandActor->GetKinematicVelocityComponent()->GetComponentRotation();
					RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastEntryStartMotionAngle = RightHandActor->GestureConditionCheck.MotionAngle;
					RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastEntryStartVelocity = RightHandActor->GetKinematicVelocityComponent()->GetKinematicVelocity();
					LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastEntryStartLocation = LeftHandActor->GetKinematicVelocityComponent()->GetComponentLocation();
					LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastEntryStartRotation = LeftHandActor->GetKinematicVelocityComponent()->GetComponentRotation();
					LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastEntryStartMotionAngle = LeftHandActor->GestureConditionCheck.MotionAngle;
					LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastEntryStartVelocity = LeftHandActor->GetKinematicVelocityComponent()->GetKinematicVelocity();
				}

				// Increment the distance traveled by each hand
				CannonShotData.DynamicData.GestureState.GestureDistanceRight += RightHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();
				CannonShotData.DynamicData.GestureState.GestureDistanceLeft += LeftHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();

				// Check if we have moved the minimum distance and for the minimum time
				if (UEmpathFunctionLibrary::GetRealTimeSince(this, CannonShotData.DynamicData.GestureState.LastEntryStartTime) >= CannonShotData.DynamicData.MinEntryTime
					&& CannonShotData.DynamicData.GestureState.GestureDistanceRight >= CannonShotData.DynamicData.MinEntryDistance
					&& CannonShotData.DynamicData.GestureState.GestureDistanceLeft >= CannonShotData.DynamicData.MinEntryDistance)
				{
					// Set the casting pose if necessary
					CannonShotData.DynamicData.GestureState.ActivationState = EEmpathActivationState::Active;
					SetCastingPose(EEmpathCastingPose::CannonShotDynamic);
				}

				return;
				break;
			}

			// If we do not meet the dynamic conditions, reset the activation state
			else
			{
				ResetPoseEntryState(EEmpathCastingPose::CannonShotStatic);
				
				// Next, check if we still meet the static conditions
				if (CannonShotData.StaticData.AreSustainConditionsMet(RightHandActor->GestureConditionCheck,
					RightHandActor->FrameConditionCheck,
					LeftHandActor->GestureConditionCheck,
					LeftHandActor->FrameConditionCheck))
				{
					// If so, maintain and update the static state and update the distance traveled
					CannonShotData.StaticData.GestureState.ActivationState = EEmpathActivationState::Active;
					CannonShotData.StaticData.GestureState.GestureDistanceRight += RightHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();
					CannonShotData.StaticData.GestureState.GestureDistanceLeft += LeftHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();
					
					return;
					break;
				}
			}
		}

		// Otherwise, begin deactivating the static state
		// Start deactivating the static gesture if we have not already
		if (CannonShotData.StaticData.GestureState.ActivationState != EEmpathActivationState::Deactivating)
		{
			// Update the exit state
			CannonShotData.StaticData.GestureState.ActivationState = EEmpathActivationState::Deactivating;
			CannonShotData.StaticData.GestureState.LastExitStartTime = GetWorld()->GetRealTimeSeconds();
		}

		// Deactivate if enough time has passed since we began deactivating
		if (UEmpathFunctionLibrary::GetRealTimeSince(this, CannonShotData.StaticData.GestureState.LastExitStartTime) >= CannonShotData.StaticData.MinExitTime)
		{
			CannonShotData.StaticData.GestureState.ActivationState = EEmpathActivationState::Inactive;
			SetCastingPose(EEmpathCastingPose::NoPose);
		}

		// Otherwise, add to the distance traveled
		else
		{
			if (RightHandActor && RightHandActor->GetKinematicVelocityComponent())
			{
				CannonShotData.StaticData.GestureState.GestureDistanceRight += RightHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();
			}
			if (LeftHandActor && LeftHandActor->GetKinematicVelocityComponent())
			{
				CannonShotData.StaticData.GestureState.GestureDistanceLeft += LeftHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();
			}
		}

		return;
		break;
	}
	case EEmpathCastingPose::CannonShotDynamic:
	{
		// Try to maintain the cannon shot
		if (CanGestureCast() && RightHandActor->IsPowerCharged() && LeftHandActor->IsPowerCharged() && CanCannonShot() && CannonShotData.DynamicData.AreSustainConditionsMet(RightHandActor->GestureConditionCheck, 
			RightHandActor->FrameConditionCheck,
			LeftHandActor->GestureConditionCheck,
			LeftHandActor->FrameConditionCheck))
		{
			// Stop any current deactivation
			CannonShotData.DynamicData.GestureState.ActivationState = EEmpathActivationState::Active;

			// Update gesture distances
			CannonShotData.DynamicData.GestureState.GestureDistanceLeft += LeftHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();
			CannonShotData.DynamicData.GestureState.GestureDistanceRight += RightHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();

			// Update last positions and rotations
			RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastExitStartLocation = RightHandActor->GetKinematicVelocityComponent()->GetComponentLocation();
			RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastExitStartRotation = RightHandActor->GetKinematicVelocityComponent()->GetComponentRotation();
			RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastExitStartMotionAngle = RightHandActor->GestureConditionCheck.MotionAngle;
			RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastExitStartVelocity = RightHandActor->GetKinematicVelocityComponent()->GetKinematicVelocity();
			LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastExitStartLocation = LeftHandActor->GetKinematicVelocityComponent()->GetComponentLocation();
			LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastExitStartRotation = LeftHandActor->GetKinematicVelocityComponent()->GetComponentRotation();
			LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastExitStartMotionAngle = LeftHandActor->GestureConditionCheck.MotionAngle;
			LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotDynamic].LastExitStartVelocity = LeftHandActor->GetKinematicVelocityComponent()->GetKinematicVelocity();
			
			return;
			break;
		}

		// Otherwise, try to deactivate the gesture
		else
		{
			// Begin deactivating if we have not already
			if (CannonShotData.DynamicData.GestureState.ActivationState != EEmpathActivationState::Deactivating)
			{
				// Update gesture state
				CannonShotData.DynamicData.GestureState.ActivationState = EEmpathActivationState::Deactivating;
				CannonShotData.DynamicData.GestureState.LastExitStartTime = GetWorld()->GetRealTimeSeconds();
			}

			// If enough time has passed, deactivate the gesture
			if (UEmpathFunctionLibrary::GetRealTimeSince(this, CannonShotData.DynamicData.GestureState.LastExitStartTime) >= CannonShotData.DynamicData.MinExitTime)
			{
				CannonShotData.DynamicData.GestureState.ActivationState = EEmpathActivationState::Inactive;
				SetCastingPose(EEmpathCastingPose::NoPose);
			}

			// Otherwise, add to the distance traveled
			else
			{
				if (RightHandActor && RightHandActor->GetKinematicVelocityComponent())
				{
					CannonShotData.StaticData.GestureState.GestureDistanceRight += RightHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();
				}
				if (LeftHandActor && LeftHandActor->GetKinematicVelocityComponent())
				{
					CannonShotData.StaticData.GestureState.GestureDistanceLeft += LeftHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();
				}
			}
		}

		return;
		break;
	}
	default:
	{
		break;
	}
	}

	return;
}

bool AEmpathPlayerCharacter::GetMidpointAndDirectionToPalms(FVector& OutMidpoint, FVector& OutDirection, float BaseHeightOffset /*= 0.0f*/)
{
	// Hands
	if (RightHandActor && LeftHandActor)
	{
		const USceneComponent* RightPalmComp = RightHandActor->GetPalmMarkerComponent();
		const USceneComponent* LeftPalmComp = LeftHandActor->GetPalmMarkerComponent();
		if (RightPalmComp && LeftPalmComp)
		{
			// Get the vector by getting the midpoint between the two hands, 
			// and the direction to the midpoint from the damage collision capsule
			OutMidpoint = (RightPalmComp->GetComponentLocation() + LeftPalmComp->GetComponentLocation()) / 2.0f;
			FVector BasePoint = DamageCapsule->GetComponentLocation();

			// Offset the base point by the inputted offset to allow for different types of angles
			BasePoint.Z += BaseHeightOffset;
			OutDirection = (OutMidpoint - BasePoint).GetSafeNormal();
			return true;
		}
	}

	OutMidpoint = FVector::ZeroVector;
	OutDirection = FVector::ZeroVector;

	return false;
}

bool AEmpathPlayerCharacter::GetMidpointAndDirectionToKVelComps(FVector& OutMidpoint, FVector& OutDirection, float BaseHeightOffset /*= 0.0f*/)
{
	// Hands
	if (RightHandActor && LeftHandActor)
	{
		UEmpathKinematicVelocityComponent* RightKVelComp = RightHandActor->GetKinematicVelocityComponent();
		UEmpathKinematicVelocityComponent* LeftKVelComp = LeftHandActor->GetKinematicVelocityComponent();
		if (RightKVelComp && LeftKVelComp)
		{
			// Get the vector by getting the midpoint between the two hands, 
			// and the direction to the midpoint from the damage collision capsule
			OutMidpoint = (RightKVelComp->GetComponentLocation() + LeftKVelComp->GetComponentLocation()) / 2.0f;
			FVector BasePoint = DamageCapsule->GetComponentLocation();

			// Offset the base point by the inputted offset to allow for different types of angles
			BasePoint.Z += BaseHeightOffset;
			OutDirection = (OutMidpoint - BasePoint).GetSafeNormal();

			return true;
		}
	}

	OutMidpoint = FVector::ZeroVector;
	OutDirection = FVector::ZeroVector;

	return false;
}

bool AEmpathPlayerCharacter::GetMidpointAndDirectionToMotionControllers(FVector& OutMidpoint, FVector& OutDirection, float BaseHeightOffset)
{
	if (RightMotionController && LeftMotionController)
	{
		// Get the vector by getting the midpoint between the two hands, 
		// and the direction to the midpoint from the damage collision capsule
		OutMidpoint = (RightMotionController->GetComponentLocation() + LeftMotionController->GetComponentLocation()) / 2.0f;
		FVector BasePoint = DamageCapsule->GetComponentLocation();

		// Offset the base point by the inputted offset to allow for different types of angles
		BasePoint.Z += BaseHeightOffset;

		OutDirection = (OutMidpoint - BasePoint).GetSafeNormal();
		return true;
	}

	OutMidpoint = FVector::ZeroVector;
	OutDirection = FVector::ZeroVector;
	return false;

}

bool AEmpathPlayerCharacter::AttemptEnterStaticCastingPose()
{
	// Check if we are in the correct position for entering a special casting pose, 
	// and if so, update our pose.
	// Assumes we are currently in no casting pose.
	if (CanGestureCast() && RightHandActor->IsPowerCharged() && LeftHandActor->IsPowerCharged())
	{	
		// Currently cannon shot is the only pose. If we add more, we'll make this extensible, like with one handed gestures.
		if (CanCannonShot()
			&& CannonShotData.StaticData.AreEntryConditionsMet(RightHandActor->GestureConditionCheck,
				RightHandActor->FrameConditionCheck,
				LeftHandActor->GestureConditionCheck, 
				LeftHandActor->FrameConditionCheck))
		{
			// If we are not currently entering the static condition of cannon shot, begin entering it
			if (CannonShotData.StaticData.GestureState.ActivationState != EEmpathActivationState::Activating)
			{
				// Reset any other pose we may be entering
				ResetPoseEntryState(EEmpathCastingPose::CannonShotStatic);

				// Cache entry variables
				CannonShotData.StaticData.GestureState.LastEntryStartTime = GetWorld()->GetRealTimeSeconds();
				RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastEntryStartLocation = RightHandActor->GetKinematicVelocityComponent()->GetComponentLocation();
				RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastEntryStartRotation = RightHandActor->GetKinematicVelocityComponent()->GetComponentRotation();
				RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastEntryStartMotionAngle = RightHandActor->GestureConditionCheck.MotionAngle;
				RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastEntryStartVelocity = RightHandActor->GetKinematicVelocityComponent()->GetKinematicVelocity();
				LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastEntryStartLocation = LeftHandActor->GetKinematicVelocityComponent()->GetComponentLocation();
				LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastEntryStartRotation = LeftHandActor->GetKinematicVelocityComponent()->GetComponentRotation();
				LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastEntryStartMotionAngle = LeftHandActor->GestureConditionCheck.MotionAngle;
				LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastEntryStartVelocity = LeftHandActor->GetKinematicVelocityComponent()->GetKinematicVelocity();

				// Update the state
				CannonShotData.StaticData.GestureState.ActivationState = EEmpathActivationState::Activating;
			}

			// Update distance traveled
			CannonShotData.StaticData.GestureState.GestureDistanceRight += RightHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();
			CannonShotData.StaticData.GestureState.GestureDistanceLeft += LeftHandActor->GetKinematicVelocityComponent()->GetDeltaLocation().Size();

			// Enter the static state if enough time has passed and we have trave;ed far enough
			if (UEmpathFunctionLibrary::GetRealTimeSince(this, CannonShotData.StaticData.GestureState.LastEntryStartTime) >= CannonShotData.StaticData.MinEntryTime
				&& CannonShotData.StaticData.GestureState.GestureDistanceRight >= CannonShotData.StaticData.MinEntryDistance
				&& CannonShotData.StaticData.GestureState.GestureDistanceLeft >= CannonShotData.StaticData.MinEntryDistance)
			{
				// Update state variables
				RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastExitStartLocation = RightHandActor->GetKinematicVelocityComponent()->GetComponentLocation();
				RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastExitStartRotation = RightHandActor->GetKinematicVelocityComponent()->GetComponentRotation();
				RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastExitStartMotionAngle = RightHandActor->GestureConditionCheck.MotionAngle;
				RightHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastExitStartVelocity = RightHandActor->GetKinematicVelocityComponent()->GetKinematicVelocity();
				LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastExitStartLocation = LeftHandActor->GetKinematicVelocityComponent()->GetComponentLocation();
				LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastExitStartRotation = LeftHandActor->GetKinematicVelocityComponent()->GetComponentRotation();
				LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastExitStartMotionAngle = LeftHandActor->GestureConditionCheck.MotionAngle;
				LeftHandActor->GestureTransformCaches[EEmpathGestureType::CannonShotStatic].LastExitStartVelocity = LeftHandActor->GetKinematicVelocityComponent()->GetKinematicVelocity();
				CannonShotData.StaticData.GestureState.ActivationState = EEmpathActivationState::Active;
				
				// Update casting pose
				SetCastingPose(EEmpathCastingPose::CannonShotStatic);
			}

			return true;
		}
	}

	// Reset entry state if we are in no pose
	ResetPoseEntryState(EEmpathCastingPose::NoPose);

	return false;
}

void AEmpathPlayerCharacter::OnTeleport()
{
	if (LeftHandActor)
	{
		LeftHandActor->OnOwningPlayerTeleport();
	}
	if (RightHandActor)
	{
		RightHandActor->OnOwningPlayerTeleport();
	}
}

const bool AEmpathPlayerCharacter::UpdateOneHandGesture(AEmpathHandActor* Hand)
{
	if (Hand)
	{

		// If we are in no gesture
		if (Hand->GetActiveGestureType() == EEmpathGestureType::NoGesture)
		{
			// Try and enter a new one-handed gesture
			return Hand->AttemptEnterOneHandGesture();
		}

		// If we are in a gesture
		else
		{
			// Try and sustain that gesture
			return Hand->AttemptSustainOneHandGesture();
		}
	}
	return false;
}

bool AEmpathPlayerCharacter::CanCannonShot_Implementation() const 
{
	if (bCannonShotEnabled && RightHandActor && LeftHandActor)
	{
		float RealTimeSecs = GetWorld()->GetRealTimeSeconds();
		return (RealTimeSecs - LastCannonShotDynamicExitTimeStamp >= CannonShotCooldownAfterCannonShot
			&& RealTimeSecs - RightHandActor->LastPunchExitTimeStamp >= CannonShotCooldownAfterPunch
			&& RealTimeSecs - LeftHandActor->LastPunchExitTimeStamp >= CannonShotCooldownAfterPunch
			&& RealTimeSecs - RightHandActor->LastSlashExitTimeStamp >= CannonShotCooldownAfterSlash
			&& RealTimeSecs - LeftHandActor->LastSlashExitTimeStamp >= CannonShotCooldownAfterSlash);

	}
	return false;
}


bool AEmpathPlayerCharacter::IsMoving_Implementation() const
{
	return (VRMovementReference->Velocity.SizeSquared() > 25.0f || VRMovementReference->IsFalling());
}

void AEmpathPlayerCharacter::SetCastingPose(const EEmpathCastingPose NewPose)
{
	if (NewPose != CastingPose)
	{ 
		if (CanGestureCast())
		{
			// Update variables
			EEmpathCastingPose OldPose = CastingPose;
			CastingPose = NewPose;
			ResetPoseEntryState(CastingPose);

			RightHandActor->SetGestureState(UEmpathFunctionLibrary::FromCastingPoseToGestureType(CastingPose));
			LeftHandActor->SetGestureState(UEmpathFunctionLibrary::FromCastingPoseToGestureType(CastingPose));

			// Call notifies and events
			OnCastingPoseChanged(OldPose, CastingPose);
		}

		else if (CastingPose != EEmpathCastingPose::NoPose)
		{
			// Update variables
			EEmpathCastingPose OldPose = CastingPose;
			CastingPose = EEmpathCastingPose::NoPose;
			ResetPoseEntryState(CastingPose);

			RightHandActor->SetGestureState(EEmpathGestureType::NoGesture);
			LeftHandActor->SetGestureState(EEmpathGestureType::NoGesture);

			// Call notifies and events
			OnCastingPoseChanged(OldPose, CastingPose);
		}

	}
	return;
}

void AEmpathPlayerCharacter::OnCastingPoseChanged(const EEmpathCastingPose OldPose, const EEmpathCastingPose NewPose)
{
	// Update exit time stamp for cannon shot
	switch (OldPose)
	{
	case EEmpathCastingPose::CannonShotStatic:
	{
		if (NewPose != EEmpathCastingPose::CannonShotDynamic)
		{
			LastCannonShotStaticcExitTimeStamp = GetWorld()->GetRealTimeSeconds();
		}
		break;
	}
	case EEmpathCastingPose::CannonShotDynamic:
	{
		LastCannonShotDynamicExitTimeStamp = GetWorld()->GetRealTimeSeconds();
		break;
	}
	default:
	{
		break;
	}
	}

	ReceiveCastingPoseChanged(OldPose, CastingPose);
	OnCastingPoseChangedDelegate.Broadcast(OldPose, CastingPose);
	return;
}

const FVector AEmpathPlayerCharacter::GetDirectionToKVelComps(const float BaseHeightOffset) const
{
	// Hands
	if (RightHandActor && LeftHandActor)
	{
		UEmpathKinematicVelocityComponent* RightKVelComp = RightHandActor->GetKinematicVelocityComponent();
		UEmpathKinematicVelocityComponent* LeftKVelComp = LeftHandActor->GetKinematicVelocityComponent();
		if (RightKVelComp && LeftKVelComp)
		{
			// Get the vector by getting the midpoint between the two hands, 
			// and the direction to the midpoint from the damage collision capsule
			FVector MidPoint = (RightKVelComp->GetComponentLocation() + LeftKVelComp->GetComponentLocation()) / 2.0f;
			FVector BasePoint = DamageCapsule->GetComponentLocation();

			// Offset the base point by the inputted offset to allow for different types of angles
			BasePoint.Z += BaseHeightOffset;

			return (MidPoint - BasePoint).GetSafeNormal();
		}
	}

	return FVector::ZeroVector;
}

void AEmpathPlayerCharacter::ResetPoseEntryState(EEmpathCastingPose PoseToIgnore)
{
	// Cannon shot is currently the only pose. We will make this extensible if we add more
	switch (PoseToIgnore)
	{
	case EEmpathCastingPose::NoPose:
	{
		CannonShotData.StaticData.GestureState.ActivationState = EEmpathActivationState::Inactive;
		CannonShotData.StaticData.GestureState.GestureDistanceRight = 0.0f;
		CannonShotData.StaticData.GestureState.GestureDistanceLeft = 0.0f;
		CannonShotData.DynamicData.GestureState.ActivationState = EEmpathActivationState::Inactive;
		CannonShotData.DynamicData.GestureState.GestureDistanceRight = 0.0f;
		CannonShotData.DynamicData.GestureState.GestureDistanceLeft = 0.0f;
		break;
	}
	case EEmpathCastingPose::CannonShotStatic:
	{
		if (CastingPose == EEmpathCastingPose::CannonShotStatic)
		{
			CannonShotData.DynamicData.GestureState.ActivationState = EEmpathActivationState::Inactive;
			CannonShotData.DynamicData.GestureState.GestureDistanceRight = 0.0f;
			CannonShotData.DynamicData.GestureState.GestureDistanceLeft = 0.0f;
		}
		break;
	}

	case EEmpathCastingPose::CannonShotDynamic:
	{
		if (CastingPose == EEmpathCastingPose::CannonShotDynamic)
		{
			CannonShotData.StaticData.GestureState.ActivationState = EEmpathActivationState::Inactive;
			CannonShotData.StaticData.GestureState.GestureDistanceRight = 0.0f;
			CannonShotData.StaticData.GestureState.GestureDistanceLeft = 0.0f;
		}
		break;
	}
	default:
	{
		break;
	}
	}

	return;
}


const FVector AEmpathPlayerCharacter::GetMidpointBetweenKVelComps() const
{
	// Hands
	if (RightHandActor && LeftHandActor)
	{
		UEmpathKinematicVelocityComponent* RightKVelComp = RightHandActor->GetKinematicVelocityComponent();
		UEmpathKinematicVelocityComponent* LeftKVelComp = LeftHandActor->GetKinematicVelocityComponent();
		if (RightKVelComp && LeftKVelComp)
		{
			// Get the vector by getting the midpoint between the two hands, 
			return (RightKVelComp->GetComponentLocation() + LeftKVelComp->GetComponentLocation()) / 2.0f;
		}
	}

	return FVector::ZeroVector;
}

const FVector AEmpathPlayerCharacter::GetDirectionToMotionControllers(float BaseHeightOffset) const
{
	if (RightMotionController && LeftMotionController)
	{
		// Get the vector by getting the midpoint between the two hands, 
		// and the direction to the midpoint from the damage collision capsule
		FVector MidPoint = (RightMotionController->GetComponentLocation() + LeftMotionController->GetComponentLocation()) / 2.0f;
		FVector BasePoint = DamageCapsule->GetComponentLocation();

		// Offset the base point by the inputted offset to allow for different types of angles
		BasePoint.Z += BaseHeightOffset;

		return (MidPoint - BasePoint).GetSafeNormal();
	}

	return FVector::ZeroVector;
}

const FVector AEmpathPlayerCharacter::GetMidpointBetweenKMotionControllers() const
{
	if (RightMotionController && LeftMotionController)
	{
		// Get the vector by getting the midpoint between the two hands, 
		return (RightMotionController->GetComponentLocation() + LeftMotionController->GetComponentLocation()) / 2.0f;
	}

	return FVector::ZeroVector;
}

const FVector AEmpathPlayerCharacter::GetDirectionToPalms(float BaseHeightOffset) const
{
	// Hands
	if (RightHandActor && LeftHandActor)
	{
		const USceneComponent* RightPalmComp = RightHandActor->GetPalmMarkerComponent();
		const USceneComponent* LeftPalmComp = LeftHandActor->GetPalmMarkerComponent();
		if (RightPalmComp && LeftPalmComp)
		{
			// Get the vector by getting the midpoint between the two hands, 
			// and the direction to the midpoint from the damage collision capsule
			FVector MidPoint = (RightPalmComp->GetComponentLocation() + LeftPalmComp->GetComponentLocation()) / 2.0f;
			FVector BasePoint = DamageCapsule->GetComponentLocation();

			// Offset the base point by the inputted offset to allow for different types of angles
			BasePoint.Z += BaseHeightOffset;

			return (MidPoint - BasePoint).GetSafeNormal();
		}
	}

	return FVector::ZeroVector;
}

const FVector AEmpathPlayerCharacter::GetMidpointBetweenPalms() const
{
	// Hands
	if (RightHandActor && LeftHandActor)
	{
		const USceneComponent* RightPalmComp = RightHandActor->GetPalmMarkerComponent();
		const USceneComponent* LeftPalmComp = LeftHandActor->GetPalmMarkerComponent();
		if (RightPalmComp && LeftPalmComp)
		{
			// Get the vector by getting the midpoint between the two hands, 
			// and the direction to the midpoint from the damage collision capsule
			return (RightPalmComp->GetComponentLocation() + LeftPalmComp->GetComponentLocation()) / 2.0f;
			FVector BasePoint = DamageCapsule->GetComponentLocation();
		}
	}

	return FVector::ZeroVector;
}

void AEmpathPlayerCharacter::SetAllPowersEnabled(const bool bNewEnabled)
{
	bBlockEnabled = bNewEnabled;
	bPunchEnabled = bNewEnabled;
	bSlashEnabled = bNewEnabled;
	bCannonShotEnabled = bNewEnabled;
	if (RightHandActor)
	{
		RightHandActor->SetGestureCastingEnabled(bNewEnabled);
	}
	if (LeftHandActor)
	{
		LeftHandActor->SetGestureCastingEnabled(bNewEnabled);
	}
	return;
}

void AEmpathPlayerCharacter::SetAllLocomotionEnabled(const bool bNewEnabled)
{
	bTeleportEnabled = bNewEnabled;
	bPivotEnabled = bNewEnabled;
	bDashEnabled = bNewEnabled;
	bWalkEnabled = bNewEnabled;
	SetClimbEnabled(bNewEnabled);
	SetGripEnabled(bNewEnabled);
	return;
}

void AEmpathPlayerCharacter::SetAllGameplayAbilitiesEnabled(const bool bNewEnabled)
{
	SetAllLocomotionEnabled(bNewEnabled);
	SetAllPowersEnabled(bNewEnabled);
	return;
}

void AEmpathPlayerCharacter::SetGravityEnabled(const bool bNewEnabled)
{
	if (bNewEnabled != bGravityEnabled)
	{
		bGravityEnabled = bNewEnabled;
		if (VRRootReference && TeleportState != EEmpathTeleportState::TeleportingToLocation)
		{
			VRRootReference->SetEnableGravity(bGravityEnabled);
		}
		if (VRMovementReference)
		{
			VRMovementReference->GravityScale = (bGravityEnabled ? 1.0f : 0.0f);
		}
		if (bGravityEnabled)
		{
			OnGravityEnabled();
		}
		else
		{
			OnGravityDisabled();
		}
	}
	return;
}

void AEmpathPlayerCharacter::SetClickEnabled(const bool bNewEnabled)
{
	if (bNewEnabled != bClickEnabled)
	{
		bClickEnabled = bNewEnabled;
		if (bClickEnabled)
		{
			OnClickEnabled();
			if (RightHandActor)
			{
				RightHandActor->OnClickEnabled();
			}
			if (LeftHandActor)
			{
				LeftHandActor->OnClickEnabled();
			}
		}
		else
		{
			OnClickDisabled();
			if (RightHandActor)
			{
				RightHandActor->OnClickDisabled();
			}
			if (LeftHandActor)
			{
				LeftHandActor->OnClickDisabled();
			}
		}
	}
}

bool AEmpathPlayerCharacter::CanClick_Implementation() const
{
	return bClickEnabled;
}

const bool AEmpathPlayerCharacter::CanGestureCast() const
{
	return (RightHandActor && LeftHandActor && RightHandActor->CanGestureCast() && LeftHandActor->CanGestureCast());
}

const float AEmpathPlayerCharacter::GetVREyeHeight() const
{
	return VRReplicatedCamera->GetComponentTransform().GetLocation().Z - GetActorLocation().Z;
}

const FVector AEmpathPlayerCharacter::GetCenterMassLocation() const
{
	FVector ReturnValue = DamageCapsule->GetComponentLocation();
	ReturnValue.Z += CenterMassHeightOffset;

	return ReturnValue;
}

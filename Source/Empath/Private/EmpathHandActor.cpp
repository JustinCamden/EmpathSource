// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathHandActor.h"
#include "Components/SphereComponent.h"
#include "EmpathTypes.h"
#include "EmpathKinematicVelocityComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EmpathGripObjectInterface.h"
#include "EmpathPlayerCharacter.h"
#include "EmpathProjectile.h"
#include "EmpathFunctionLibrary.h"
#include "EmpathDamageType.h"

// Component names
FName AEmpathHandActor::BlockingCollisionName(TEXT("BlockingCollision"));
FName AEmpathHandActor::KinematicVelocityComponentName(TEXT("KinematicVelocityComponent"));
FName AEmpathHandActor::MeshComponentName(TEXT("MeshComponent"));
FName AEmpathHandActor::GripCollisionName(TEXT("GripCollision"));
FName AEmpathHandActor::PalmMarkerName(TEXT("PalmMarker"));

// Log categories
DEFINE_LOG_CATEGORY_STATIC(LogHandGestureRecognition, Log, All);

// Sets default values
AEmpathHandActor::AEmpathHandActor(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;

	// Initial variables
	FollowComponentLostThreshold = 10.0f;
	ControllerOffsetLocation = FVector(8.5f, 1.0f, -2.5f);
	ControllerOffsetRotation = FRotator(-20.0f, -100.0f, -90.0f);
	ActiveOneHandGestureIdx = -1;
	PunchCooldownAfterPunch = 0.2f;
	PunchCooldownAfterSlash = 0.3f;
	PunchCooldownAfterCannonShotStatic = 1.0f;
	PunchCooldownAfterCannonShotDynamic = 1.0f;
	SlashCooldownAfterPunch = 0.3f;
	SlashCooldownAfterSlash = 0.2f;
	SlashCooldownAfterCannonShotStatic = 1.0f;
	SlashCooldownAfterCannonShotDynamic = 1.0f;

	// Initialize components
	// Blocking collision
	BlockingCollision = CreateDefaultSubobject<USphereComponent>(BlockingCollisionName);
	BlockingCollision->InitSphereRadius(7.75f);
	BlockingCollision->SetCollisionProfileName(FEmpathCollisionProfiles::HandCollision);
	BlockingCollision->SetEnableGravity(false);
	RootComponent = BlockingCollision;

	// Mesh component
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(MeshComponentName);
	MeshComponent->SetEnableGravity(false);
	MeshComponent->SetCollisionProfileName(FEmpathCollisionProfiles::NoCollision);
	MeshComponent->SetupAttachment(RootComponent);

	// Grip collision
	GripCollision = CreateDefaultSubobject<USphereComponent>(GripCollisionName);
	GripCollision->InitSphereRadius(10.0f);
	GripCollision->SetCollisionProfileName(FEmpathCollisionProfiles::OverlapAllDynamic);
	GripCollision->SetEnableGravity(false);
	GripCollision->SetupAttachment(MeshComponent);

	// Palm marker
	PalmMarker = CreateDefaultSubobject<USceneComponent>(PalmMarkerName);
	PalmMarker->SetupAttachment(MeshComponent);
	PalmMarker->SetRelativeLocationAndRotation(FVector(2.0f, 0.0f, 12.0f), FRotator(0.0f, 0.0f, 90.0f));

	// Kinematic velocity
	KinematicVelocityComponent = CreateDefaultSubobject<UEmpathKinematicVelocityComponent>(KinematicVelocityComponentName);
	KinematicVelocityComponent->SetupAttachment(MeshComponent);

	// Transform caches
	GestureTransformCaches.Add(EEmpathGestureType::Punching);
	GestureTransformCaches.Add(EEmpathGestureType::Slashing);
	GestureTransformCaches.Add(EEmpathGestureType::CannonShotStatic);
	GestureTransformCaches.Add(EEmpathGestureType::CannonShotDynamic);
}

// Called when the game starts or when spawned
void AEmpathHandActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AEmpathHandActor::Tick(float DeltaTime)
{
	// Call the parent function
	Super::Tick(DeltaTime);

	// Update our position to follow the component
	MoveToFollowComponent();

	// Update blocking state
	TickUpdateBlockingState();

	// Update grip state
	TickUpdateGripState();

}

void AEmpathHandActor::RegisterHand(AEmpathHandActor* InOtherHand, 
	AEmpathPlayerCharacter* InOwningPlayerCharacter,
	USceneComponent* InFollowedComponent,
	EEmpathBinaryHand InOwningHand)
{
	// Cache inputted components
	OtherHand = InOtherHand;
	OwningPlayerCharacter = InOwningPlayerCharacter;
	FollowedComponent = InFollowedComponent;
	OwningHand = InOwningHand;
	KinematicVelocityComponent->OwningPlayer = InOwningPlayerCharacter; 

	// Invert the actor's left / right scale if it's the left hand
	if (OwningHand == EEmpathBinaryHand::Left)
	{
		FVector NewScale = GetActorScale3D();
		NewScale.Y *= -1.0f;
		SetActorScale3D(NewScale);

		// Invert the gestures
		for (int32 Idx = 0; Idx < TypedOneHandedGestures.Num(); Idx++)
		{
			TypedOneHandedGestures[Idx].InvertHand();
		}
		BlockingData.InvertHand();
	}

	// We apply an initial controller offset to the mesh 
	// to compensate for the offset of the tracking origin and the actual center
	MeshComponent->SetRelativeLocationAndRotation(ControllerOffsetLocation, ControllerOffsetRotation);

	OnHandRegistered();
}

void AEmpathHandActor::SetGestureCastingEnabled(const bool bNewEnabled)
{
	if (bNewEnabled != bGestureCastingEnabled)
	{
		bGestureCastingEnabled = bNewEnabled;
		if (bNewEnabled)
		{
			if (!bLostFollowComponent)
			{
				KinematicVelocityComponent->Activate(false);
			}
			OnGestureCastingEnabled();
		}
		else
		{
			KinematicVelocityComponent->Deactivate();
			GestureConditionCheck = FEmpathGestureCheck();
			FrameConditionCheck = FEmpathGestureCheck();
			SetGestureState(EEmpathGestureType::NoGesture);
			OnGestureCastingDisabled();
		}
	}
}

void AEmpathHandActor::MoveToFollowComponent()
{
	if (FollowedComponent)
	{
		// Move to the followed component
		SetActorLocationAndRotation(FollowedComponent->GetComponentLocation(),
			FollowedComponent->GetComponentRotation(),
			true);

		// Check the distance from the follow component
		float CurrentFollowDistance = (FollowedComponent->GetComponentTransform().GetLocation() - GetTransform().GetLocation()).Size();
		if (CurrentFollowDistance >= FollowComponentLostThreshold)
		{
			UpdateLostFollowComponent(true);
		}
		else
		{
			UpdateLostFollowComponent(false);
		}
	}
	else
	{
		// If we don't have a follow component, we've lost it
		UpdateLostFollowComponent(true);
	}
}


void AEmpathHandActor::OnClimbStart()
{
	BlockingCollision->SetCollisionProfileName(FEmpathCollisionProfiles::HandCollisionClimb);
	ReceiveClimbStart();
}

void AEmpathHandActor::OnClimbEnd()
{
	BlockingCollision->SetCollisionProfileName(FEmpathCollisionProfiles::HandCollision);
	ReceiveClimbEnd();
}

void AEmpathHandActor::OnClimbEnabled()
{
	ReceiveClimbEnabled();
	return;
}

void AEmpathHandActor::OnClimbDisabled()
{
	if (GripState == EEmpathGripType::Climb)
	{
		ClearGrip();
	}
	ReceiveClimbDisabled();
	return;
}

void AEmpathHandActor::UpdateLostFollowComponent(bool bLost)
{
	// Only process this function is there a change in result
	if (bLost != bLostFollowComponent)
	{
		// Update variable before firing notifies
		if (bLost)
		{
			bLostFollowComponent = true;
			OnLostFollowComponent();
		}
		else
		{
			bLostFollowComponent = false;
			OnFoundFollowComponent();
		}
	}
}

void AEmpathHandActor::OnLostFollowComponent()
{
	if (KinematicVelocityComponent->IsActive())
	{
		KinematicVelocityComponent->Deactivate();
	}
	ReceiveLostFollowComponent();
}

void AEmpathHandActor::OnFoundFollowComponent()
{
	if (KinematicVelocityComponent->IsActive() == false && bGestureCastingEnabled)
	{
		KinematicVelocityComponent->Activate(false);
	}
	ReceiveFoundFollowComponent();
}

FVector AEmpathHandActor::GetTeleportOrigin_Implementation() const
{
	return GetActorLocation();
}

FVector AEmpathHandActor::GetTeleportDirection_Implementation(FVector LocalDirection) const
{
	return KinematicVelocityComponent->GetComponentTransform().TransformVectorNoScale(LocalDirection.GetSafeNormal());
}

void AEmpathHandActor::GetBestGripCandidate(AActor*& GripActor, UPrimitiveComponent*& GripComponent, EEmpathGripType& GripResponse)
{
	// Get all overlapping components
	TArray<UPrimitiveComponent*> OverlappingComponents;
	GripCollision->GetOverlappingComponents(OverlappingComponents);

	float BestDistance = 99999.0f;
	bool bFoundActor = false;

	// Check each overlapping component
	for (UPrimitiveComponent* CurrComponent : OverlappingComponents)
	{
		AActor* CurrActor = CurrComponent->GetOwner();

		// If this is a new actor, we need to check if it implements the interface, and if so, what the response is
		if (CurrActor != GripActor)
		{
			if (CurrActor->GetClass()->ImplementsInterface(UEmpathGripObjectInterface::StaticClass()))
			{
				EEmpathGripType CurrGripResponse = IEmpathGripObjectInterface::Execute_GetGripResponse(CurrActor, this, CurrComponent);
				if (CurrGripResponse != EEmpathGripType::NoGrip)
				{
					// Check the distance to this component is smaller than the current best. If so, update the current best
					float CurrDist = (CurrComponent->GetComponentLocation() - GripCollision->GetComponentLocation()).Size();
					if (CurrDist < BestDistance)
					{
						GripActor = CurrActor;
						GripComponent = CurrComponent;
						BestDistance = CurrDist;
						GripResponse = CurrGripResponse;
					}
				}
			}
		}

		// If this is our current best actor, then we can skip checking if it implements the interface
		else
		{
			EEmpathGripType CurrGripResponse = IEmpathGripObjectInterface::Execute_GetGripResponse(CurrActor, this, CurrComponent);
			if (CurrGripResponse != EEmpathGripType::NoGrip)
			{
				float CurrDist = (CurrComponent->GetComponentLocation() - GripCollision->GetComponentLocation()).Size();
				if (CurrDist < BestDistance)
				{
					GripComponent = CurrComponent;
					BestDistance = CurrDist;
					GripResponse = CurrGripResponse;
				}
			}
		}
	}
	return;

}

void AEmpathHandActor::OnGripPressed()
{
	if (!bGripPressed)
	{
		bGripPressed = true;
		if (OwningPlayerCharacter && OwningPlayerCharacter->CanGrip())
		{
			// Only attempt to grip if we are not already gripping an object
			if (GripState == EEmpathGripType::NoGrip)
			{
				//Try and get a grip candidate
				AActor* GripCandidate;
				UPrimitiveComponent* GripComponent;
				EEmpathGripType GripResponse;
				GetBestGripCandidate(GripCandidate, GripComponent, GripResponse);

				if (GripCandidate)
				{
					// Branch functionality depending on the type of grip
					switch (GripResponse)
					{
						// Climbing
					case EEmpathGripType::Climb:
					{
						// If we can climb then register the new climb point with the player character and update the grip state
						if (OwningPlayerCharacter->CanClimb())
						{
							FVector GripOffset = GripComponent->GetComponentTransform().InverseTransformPosition(GetActorLocation());
							OwningPlayerCharacter->SetClimbingGrip(this, GripComponent, GripOffset);
							GripState = EEmpathGripType::Climb;
							GrippedActor = GripCandidate;
							OnClimbStart();
							IEmpathGripObjectInterface::Execute_OnGripped(GrippedActor, this, GripComponent);
						}
						break;
					}

					// Grip
					case EEmpathGripType::Grip:
					{
						GrippedActor = GripCandidate;
						GripState = EEmpathGripType::Grip;
						ReceiveGripActorStart(GrippedActor);
						IEmpathGripObjectInterface::Execute_OnGripped(GrippedActor, this, GripComponent);
						break;
					}
					default:
					{
						break;
					}
					}
				}
			}
		}
		ReceiveGripPressed();
	}
	return;
}

void AEmpathHandActor::ClearGrip()
{
	switch (GripState)
	{
	case EEmpathGripType::Climb:
	{
		// If this was our dominant climbing hand, but the other hand is still climbing, check and see if the
		// other hand can find a valid grip object
		if (OwningPlayerCharacter && OwningPlayerCharacter->ClimbHand == this && !OtherHand->CheckForClimbGrip())
		{
			// If not, then clear the player grip state
			OwningPlayerCharacter->ClearClimbingGrip();
		}
		if (GrippedActor)
		{
			if (GrippedActor->GetClass()->ImplementsInterface(UEmpathGripObjectInterface::StaticClass()))
			{
				IEmpathGripObjectInterface::Execute_OnGripReleased(GrippedActor, this);

			}
			GrippedActor = nullptr;
		}
		OnClimbEnd();
		break;
	}
	case EEmpathGripType::Grip:
	{
		AActor* OldGrippedActor = GrippedActor;
		if (GrippedActor)
		{
			if (GrippedActor->GetClass()->ImplementsInterface(UEmpathGripObjectInterface::StaticClass()))
			{
				IEmpathGripObjectInterface::Execute_OnGripReleased(GrippedActor, this);

			}
			GrippedActor = nullptr;
		}
		ReceiveGripActorEnd(OldGrippedActor);
		break;
	}
	default:
	{
		break;
	}
	}
	// Update grip state
	GripState = EEmpathGripType::NoGrip;
}

void AEmpathHandActor::OnGripReleased()
{
	if (bGripPressed)
	{
		bGripPressed = false;
		ClearGrip();
		ReceiveGripReleased();
	}
}

bool AEmpathHandActor::CheckForClimbGrip()
{
	// If we're climbing
	if (GripState == EEmpathGripType::Climb)
	{
		// Try and get a grip candidate
		AActor* GripCandidate;
		UPrimitiveComponent* GripComponent;
		EEmpathGripType GripResponse;
		GetBestGripCandidate(GripCandidate, GripComponent, GripResponse);

		// If we find a climbing grip, then update the climbing grip point on the owning player character
		if (GripResponse == EEmpathGripType::Climb)
		{
			FVector GripOffset = GripComponent->GetComponentTransform().InverseTransformPosition(GetActorLocation());
			OwningPlayerCharacter->SetClimbingGrip(this, GripComponent, GripOffset);
			return true;
		}
	}
	return false;
}

void AEmpathHandActor::OnGripEnabled()
{
	ReceiveGripEnabled();
	return;
}

void AEmpathHandActor::OnGripDisabled()
{
	ClearGrip();
	ReceiveGripDisabled();
	return;
}

void AEmpathHandActor::SetActiveOneHandGestureIdx(int32 Idx)
{
	if (Idx != ActiveOneHandGestureIdx)
	{
		// Check if the idx is valid
		if (Idx > -1 && Idx < TypedOneHandedGestures.Num())
		{
			// If so, update the gesture state
			LastActiveOneHandGestureIdx = ActiveOneHandGestureIdx;
			ActiveOneHandGestureIdx = Idx;
			TypedOneHandedGestures[Idx].GestureState.ActivationState = EEmpathActivationState::Active;
			ResetOneHandeGestureEntryState(Idx);
			EEmpathGestureType GestureKey = UEmpathFunctionLibrary::FromOneHandGestureTypeToGestureType(TypedOneHandedGestures[Idx].GestureType);
			GestureTransformCaches[GestureKey].LastExitStartLocation = KinematicVelocityComponent->GetComponentLocation();
			GestureTransformCaches[GestureKey].LastExitStartRotation = KinematicVelocityComponent->GetComponentRotation();
			GestureTransformCaches[GestureKey].LastExitStartMotionAngle = GestureConditionCheck.MotionAngle;
			GestureTransformCaches[GestureKey].LastExitStartVelocity = KinematicVelocityComponent->GetKinematicVelocity();
			SetGestureState(GestureKey);
		}
		else
		{
			// Otherwise, clear the active gesture
			LastActiveOneHandGestureIdx = ActiveOneHandGestureIdx;
			ActiveOneHandGestureIdx = -1;
			ResetOneHandeGestureEntryState();
			SetGestureState(EEmpathGestureType::NoGesture);
		}
	}

	return;
}

void AEmpathHandActor::SetGestureState(const EEmpathGestureType NewGestureState)
{
	// Update gesture state if appropriate
	if (NewGestureState != ActiveGestureType)
	{
		// Set new gesture state if gesture casting is enabled
		if (CanGestureCast())
		{
			EEmpathGestureType OldGestureState = ActiveGestureType;
			ActiveGestureType = NewGestureState;
			OnGestureStateChanged(OldGestureState, ActiveGestureType);
		}
		else if (ActiveGestureType != EEmpathGestureType::NoGesture)
		{
			EEmpathGestureType OldGestureState = ActiveGestureType;
			ActiveGestureType = EEmpathGestureType::NoGesture;
			OnGestureStateChanged(OldGestureState, ActiveGestureType);
		}
	} 

	return;

}

void AEmpathHandActor::OnGestureStateChanged(const EEmpathGestureType OldGestureState, const EEmpathGestureType NewGestureState)
{

	// Update the exit timestamps
	switch (OldGestureState)
	{
	case EEmpathGestureType::Punching:
	{
		LastPunchExitTimeStamp = GetWorld()->GetRealTimeSeconds();
		break;
	}
	case EEmpathGestureType::Slashing:
	{
		LastSlashExitTimeStamp = GetWorld()->GetRealTimeSeconds();
		break;
	}
	default:
	{
		break;
	}
	}

	// Fire notifies and receives
	ReceiveGestureStateChanged(OldGestureState, NewGestureState);
	if (OtherHand)
	{
		OtherHand->OnOtherHandGestureStateChanged(OldGestureState, NewGestureState);
	}
	if (OwningPlayerCharacter)
	{
		OwningPlayerCharacter->OnGestureStateChanged(OldGestureState, NewGestureState, OwningHand);
	}
}

void AEmpathHandActor::OnOtherHandGestureStateChanged(const EEmpathGestureType OtherHandOldGestureState, const EEmpathGestureType OtherHandNewGestureState)
{
	ReceiveOtherHandGestureStateChanged(OtherHandOldGestureState, OtherHandNewGestureState);
}

void AEmpathHandActor::TickUpdateBlockingState()
{
	// Handle sustaining the gesture
	if (bIsBlocking)
	{
		// Check if sustain conditions are met
		if (CanGestureCast() && BlockingData.AreSustainConditionsMet(GestureConditionCheck, FrameConditionCheck))
		{
			// If so, cancel deactivation and add the current distance traveled
			BlockingData.GestureState.ActivationState = EEmpathActivationState::Active;
			BlockingData.GestureState.GestureDistance += KinematicVelocityComponent->GetDeltaLocation().Size();
			BlockingTransformCache.LastExitStartLocation = KinematicVelocityComponent->GetComponentLocation();
			BlockingTransformCache.LastExitStartRotation = KinematicVelocityComponent->GetComponentRotation();
			BlockingTransformCache.LastExitStartMotionAngle = GestureConditionCheck.MotionAngle;
			BlockingTransformCache.LastExitStartVelocity = KinematicVelocityComponent->GetKinematicVelocity();
		}

		// Otherwise, attempt deactivation
		else
		{
			// Begin deactivating if we have not already
			if (BlockingData.GestureState.ActivationState != EEmpathActivationState::Deactivating)
			{
				BlockingData.GestureState.LastExitStartTime = GetWorld()->GetRealTimeSeconds();
				BlockingData.GestureState.ActivationState = EEmpathActivationState::Deactivating;
			}

			// Deactivate the gesture if appropriate
			if (UEmpathFunctionLibrary::GetRealTimeSince(this, BlockingData.GestureState.LastExitStartTime) >= BlockingData.MinExitTime)
			{
				BlockingData.GestureState.ActivationState = EEmpathActivationState::Inactive;
				SetIsBlocking(false);
			}

			// Otherwise, add the distance traveled to the curr gesture state
			else if (KinematicVelocityComponent)
			{
				BlockingData.GestureState.GestureDistance += KinematicVelocityComponent->GetDeltaLocation().Size();
			}
		}
	}
	// Handle gesture entry
	else
	{
		// Attempt to enter the gesture
		if (CanGestureCast() && CanBlock() && BlockingData.AreEntryConditionsMet(GestureConditionCheck, FrameConditionCheck))
		{
			// Begin activating if we have not already
			if (BlockingData.GestureState.ActivationState != EEmpathActivationState::Activating)
			{
				BlockingData.GestureState.LastEntryStartTime = GetWorld()->GetRealTimeSeconds();
				BlockingTransformCache.LastEntryStartLocation = KinematicVelocityComponent->GetComponentLocation();
				BlockingTransformCache.LastEntryStartRotation = KinematicVelocityComponent->GetComponentRotation();
				BlockingTransformCache.LastEntryStartMotionAngle = GestureConditionCheck.MotionAngle;
				BlockingTransformCache.LastEntryStartVelocity = KinematicVelocityComponent->GetKinematicVelocity();
				BlockingData.GestureState.ActivationState = EEmpathActivationState::Activating;
			}

			// Add the distance traveled to this gesture
			BlockingData.GestureState.GestureDistance += KinematicVelocityComponent->GetDeltaLocation().Size();

			// Activate if appropriate
			if (BlockingData.GestureState.GestureDistance >= BlockingData.MinEntryDistance
				&& UEmpathFunctionLibrary::GetRealTimeSince(this, BlockingData.GestureState.LastEntryStartTime) >= BlockingData.MinEntryTime)
			{
				BlockingData.GestureState.ActivationState = EEmpathActivationState::Active;
				BlockingTransformCache.LastExitStartLocation = KinematicVelocityComponent->GetComponentLocation();
				BlockingTransformCache.LastExitStartRotation = KinematicVelocityComponent->GetComponentRotation();
				BlockingTransformCache.LastExitStartMotionAngle = GestureConditionCheck.MotionAngle;
				BlockingTransformCache.LastExitStartVelocity = KinematicVelocityComponent->GetKinematicVelocity();

				SetIsBlocking(true);
			}
		}

		// Otherwise reset entry
		else if (BlockingData.GestureState.ActivationState != EEmpathActivationState::Inactive)
		{
			BlockingData.GestureState.ActivationState = EEmpathActivationState::Inactive;
		}
	}
}

void AEmpathHandActor::TickUpdateGripState()
{
	if (GripState != EEmpathGripType::NoGrip 
		&& (!OwningPlayerCharacter || !OwningPlayerCharacter->CanGrip() 
			|| (GripState == EEmpathGripType::Climb && !OwningPlayerCharacter->CanClimb())))
	{
		ClearGrip();
	}
}

void AEmpathHandActor::SetIsBlocking(const bool bNewIsBlocking)
{
	if (bNewIsBlocking != bIsBlocking)
	{
		bIsBlocking = bNewIsBlocking;
		if (bIsBlocking)
		{
			OnBlockingStart();
		}
		else
		{
			OnBlockingEnd();
		}
		if (OwningPlayerCharacter)
		{
			OwningPlayerCharacter->OnNewBlockingState(bIsBlocking, OwningHand);
		}
	}
	return;
}

float AEmpathHandActor::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Do nothing if we have no player or are invincible
	if (bHandInvincible || !OwningPlayerCharacter)
	{
		return 0.0f;
	}

	// Cache the initial damage amount
	float AdjustedDamage = DamageAmount;

	// Get the damage type
	UDamageType const* const DamageTypeCDO = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	UEmpathDamageType const* EmpathDamageTypeCDO = Cast<UEmpathDamageType>(DamageTypeCDO);

	// Friendly fire damage adjustment
	EEmpathTeam const InstigatorTeam = UEmpathFunctionLibrary::GetActorTeam(EventInstigator);
	EEmpathTeam const MyTeam = GetTeamNum();
	if (InstigatorTeam == MyTeam)
	{
		// If this damage came from our team and we can't take friendly fire, do nothing
		if (!bCanHandTakeFriendlyFire)
		{
			return 0.0f;
		}

		// Otherwise, scale the damage by the friendly fire damage multiplier
		else if (EmpathDamageTypeCDO)
		{
			AdjustedDamage *= EmpathDamageTypeCDO->FriendlyFireDamageMultiplier;
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
		// Process damage to update health 
		ProcessFinalDamage(ActualDamage, HitInfo, HitImpulseDir, DamageTypeCDO, EventInstigator, DamageCauser);
		OwningPlayerCharacter->TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		return ActualDamage;
	}
	return 0.0f;;

}

void AEmpathHandActor::OnClickPressed()
{
	if (!bClickPressed)
	{
		bClickPressed = true;
		if (OwningPlayerCharacter && OwningPlayerCharacter->CanClick())
		{
			ReceiveClickPressed();
		}
	}
	return;
}

void AEmpathHandActor::OnClickReleased()
{
	if (bClickPressed)
	{
		bClickPressed = false;
		ReceiveClickReleased();
	}
	return;
}

void AEmpathHandActor::OnClickEnabled()
{
	ReceiveClickEnabled();
	if (bClickPressed && OwningPlayerCharacter && OwningPlayerCharacter->CanClick())
	{
		ReceiveClickPressed();
	}
	return;
}

void AEmpathHandActor::OnClickDisabled()
{
	ReceiveClickDisabled();
	if (bClickPressed)
	{
		ReceiveClickReleased();
	}
	return;
}

float AEmpathHandActor::ModifyAnyDamage_Implementation(float DamageAmount, const AController* EventInstigator, const AActor* DamageCauser, const class UDamageType* DamageType)
{
	return DamageAmount;
}

float AEmpathHandActor::ModifyPointDamage_Implementation(float DamageAmount, const FHitResult& HitInfo, const FVector& ShotDirection, const AController* EventInstigator, const AActor* DamageCauser, const class UDamageType* DamageTypeCDO)
{
	return DamageAmount;
}

float AEmpathHandActor::ModifyRadialDamage_Implementation(float DamageAmount, const FVector& Origin, const TArray<struct FHitResult>& ComponentHits, float InnerRadius, float OuterRadius, const AController* EventInstigator, const AActor* DamageCauser, const class UDamageType* DamageTypeCDO)
{
	return DamageAmount;
}

void AEmpathHandActor::ProcessFinalDamage_Implementation(const float DamageAmount, FHitResult const& HitInfo, FVector HitImpulseDir, const UDamageType* DamageType, const AController* EventInstigator, const AActor* DamageCauser)
{
	return;
}

bool AEmpathHandActor::CanGestureCast_Implementation() const
{
	return (bGestureCastingEnabled
		&& KinematicVelocityComponent
		&& OwningPlayerCharacter
		&& OwningPlayerCharacter->GetTeleportState() == EEmpathTeleportState::NotTeleporting
		&& !OwningPlayerCharacter->IsDead()
		&& !OwningPlayerCharacter->IsStunned()
		&& !GetWorld()->IsPaused());
}

void AEmpathHandActor::OnOwningPlayerTeleport()
{
	if (KinematicVelocityComponent->IsActive())
	{
		KinematicVelocityComponent->Deactivate();
	}
	SetGestureState(EEmpathGestureType::NoGesture);
	ReceiveOwningPlayerTeleport();
}


void AEmpathHandActor::OnOwningPlayerTeleportEnd()
{
	if (KinematicVelocityComponent->IsActive() == false && bGestureCastingEnabled)
	{
		KinematicVelocityComponent->Activate(false);
	}

	ReceiveOwningPlayerTeleportEnd();
}

bool AEmpathHandActor::CanPunch_Implementation() const
{
	if (OwningPlayerCharacter && OwningPlayerCharacter->bPunchEnabled)
	{
		float RealTimeSecs = GetWorld()->GetRealTimeSeconds();
		return (RealTimeSecs - LastPunchExitTimeStamp >= PunchCooldownAfterPunch
			&& RealTimeSecs - LastSlashExitTimeStamp >= PunchCooldownAfterSlash
			&& RealTimeSecs - OwningPlayerCharacter->LastCannonShotStaticcExitTimeStamp >= PunchCooldownAfterCannonShotStatic
			&& RealTimeSecs - OwningPlayerCharacter->LastCannonShotDynamicExitTimeStamp >= PunchCooldownAfterCannonShotDynamic);
	}
	return false;
}

bool AEmpathHandActor::CanSlash_Implementation() const
{
	if (OwningPlayerCharacter && OwningPlayerCharacter->bSlashEnabled)
	{
		float RealTimeSecs = GetWorld()->GetRealTimeSeconds();
		return (RealTimeSecs - LastPunchExitTimeStamp >= SlashCooldownAfterPunch
			&& RealTimeSecs - LastSlashExitTimeStamp >= SlashCooldownAfterSlash
			&& RealTimeSecs - OwningPlayerCharacter->LastCannonShotStaticcExitTimeStamp >= SlashCooldownAfterCannonShotStatic
			&& RealTimeSecs - OwningPlayerCharacter->LastCannonShotDynamicExitTimeStamp >= SlashCooldownAfterCannonShotDynamic);
	}
	return false;
}

bool AEmpathHandActor::CanBlock_Implementation() const
{
	return (OwningPlayerCharacter && OwningPlayerCharacter->bBlockEnabled);
}

const bool AEmpathHandActor::AttemptEnterOneHandGesture()
{
	// Check for valid input
	if (CanGestureCast() && bIsPowerCharged)
	{
		// Cache whether we can slash or punch
		bool bCanPunch = CanPunch();
		bool bCanSlash = CanSlash();

		// Loop through each gesture
		for (int32 Idx = 0; Idx < TypedOneHandedGestures.Num(); Idx++)
		{
			// Only check gestures with a gesture state we can enter
			switch (TypedOneHandedGestures[Idx].GestureType)
			{
			case EEmpathOneHandGestureType::Punching:
			{
				if (!bCanPunch)
				{
					continue;
				}
				break;
			}
			case EEmpathOneHandGestureType::Slashing:
			{
				if (!bCanSlash)
				{
					continue;
				}
				break;
			}
			default:
			{
				continue;
				break;
			}
			}

			// Check the entry conditions
			if (TypedOneHandedGestures[Idx].AreEntryConditionsMet(GestureConditionCheck, FrameConditionCheck))
			{
				// Begin activating if we have not already
				if (TypedOneHandedGestures[Idx].GestureState.ActivationState != EEmpathActivationState::Activating)
				{
					// Reset any other gestures that we may be activating
					ResetOneHandeGestureEntryState(Idx);

					// Update variables
					TypedOneHandedGestures[Idx].GestureState.LastEntryStartTime = GetWorld()->GetRealTimeSeconds();
					EEmpathGestureType GestureKey = UEmpathFunctionLibrary::FromOneHandGestureTypeToGestureType(TypedOneHandedGestures[Idx].GestureType);
					GestureTransformCaches[GestureKey].LastEntryStartLocation = KinematicVelocityComponent->GetComponentLocation();
					GestureTransformCaches[GestureKey].LastEntryStartRotation = KinematicVelocityComponent->GetComponentRotation();
					GestureTransformCaches[GestureKey].LastEntryStartMotionAngle = GestureConditionCheck.MotionAngle;
					GestureTransformCaches[GestureKey].LastEntryStartVelocity = KinematicVelocityComponent->GetKinematicVelocity();
					TypedOneHandedGestures[Idx].GestureState.ActivationState = EEmpathActivationState::Activating;
				}

				// Add the distance traveled to this gesture
				TypedOneHandedGestures[Idx].GestureState.GestureDistance += KinematicVelocityComponent->GetDeltaLocation().Size();

				// Activate if appropriate
				if (TypedOneHandedGestures[Idx].GestureState.GestureDistance >= TypedOneHandedGestures[Idx].MinEntryDistance
					&& UEmpathFunctionLibrary::GetRealTimeSince(this, TypedOneHandedGestures[Idx].GestureState.LastEntryStartTime) >= TypedOneHandedGestures[Idx].MinEntryTime)
				{
					SetActiveOneHandGestureIdx(Idx);
				}
				return true;
			}
		}
	}
	// If all checks fail, reset activation and return no gesture
	ResetOneHandeGestureEntryState();
	return false;
}

void AEmpathHandActor::ResetOneHandeGestureEntryState(int32 IdxToIgnore)
{
	if (IdxToIgnore > -1 && IdxToIgnore < TypedOneHandedGestures.Num())
	{
		for (int32 Idx = 0; Idx < TypedOneHandedGestures.Num(); Idx++)
		{
			if (Idx != IdxToIgnore)
			{
				TypedOneHandedGestures[Idx].GestureState.ActivationState = EEmpathActivationState::Inactive;
				TypedOneHandedGestures[Idx].GestureState.GestureDistance = 0.0f;
			}
		}
	}
	else
	{
		for (int32 Idx = 0; Idx < TypedOneHandedGestures.Num(); Idx++)
		{
			TypedOneHandedGestures[Idx].GestureState.ActivationState = EEmpathActivationState::Inactive;
			TypedOneHandedGestures[Idx].GestureState.GestureDistance = 0.0f;
		}
	}

	return;
}

const bool AEmpathHandActor::AttemptSustainOneHandGesture()
{
	// Attempt to sustaining the appropriate gesture
	if (ActiveOneHandGestureIdx > -1 && ActiveOneHandGestureIdx < TypedOneHandedGestures.Num())
	{
		if (CanGestureCast() && bIsPowerCharged)
		{
			// Ensure that the gesture is for a gesture state we can current enter
			bool bGestureEnabled = false;
			switch (TypedOneHandedGestures[ActiveOneHandGestureIdx].GestureType)
			{
			case EEmpathOneHandGestureType::Punching:
			{
				if (CanPunch())
				{
					bGestureEnabled = true;
				}
				break;
			}
			case EEmpathOneHandGestureType::Slashing:
			{
				if (CanSlash())
				{
					bGestureEnabled = true;
				}
				break;
			}
			default:
			{
				bGestureEnabled = false;
				break;
			}
			}

			// Check if sustain conditions are met
			if (bGestureEnabled && TypedOneHandedGestures[ActiveOneHandGestureIdx].AreSustainConditionsMet(GestureConditionCheck, FrameConditionCheck))
			{
				// If so, cancel deactivation and add the current distance traveled
				TypedOneHandedGestures[ActiveOneHandGestureIdx].GestureState.ActivationState = EEmpathActivationState::Active;
				TypedOneHandedGestures[ActiveOneHandGestureIdx].GestureState.GestureDistance += KinematicVelocityComponent->GetDeltaLocation().Size();

				// Update exit start locations
				EEmpathGestureType GestureKey = UEmpathFunctionLibrary::FromOneHandGestureTypeToGestureType(TypedOneHandedGestures[ActiveOneHandGestureIdx].GestureType);
				GestureTransformCaches[GestureKey].LastExitStartLocation = KinematicVelocityComponent->GetComponentLocation();
				GestureTransformCaches[GestureKey].LastExitStartRotation = KinematicVelocityComponent->GetComponentRotation();
				GestureTransformCaches[GestureKey].LastExitStartMotionAngle = GestureConditionCheck.MotionAngle;
				GestureTransformCaches[GestureKey].LastExitStartVelocity = KinematicVelocityComponent->GetKinematicVelocity();

				return true;
			}
		}

		// Otherwise, attempt deactivation
		// Begin deactivating if we have not already
		if (TypedOneHandedGestures[ActiveOneHandGestureIdx].GestureState.ActivationState != EEmpathActivationState::Deactivating)
		{
			TypedOneHandedGestures[ActiveOneHandGestureIdx].GestureState.LastExitStartTime = GetWorld()->GetRealTimeSeconds();
			TypedOneHandedGestures[ActiveOneHandGestureIdx].GestureState.ActivationState = EEmpathActivationState::Deactivating;
		}

		// Deactivate the gesture if appropriate
		if (UEmpathFunctionLibrary::GetRealTimeSince(this, TypedOneHandedGestures[ActiveOneHandGestureIdx].GestureState.LastExitStartTime) >= TypedOneHandedGestures[ActiveOneHandGestureIdx].MinExitTime)
		{
			SetActiveOneHandGestureIdx();
			return false;
		}

		// Otherwise, add the distance traveled to the curr gesture state
		else
		{
			if (KinematicVelocityComponent)
			{
					TypedOneHandedGestures[ActiveOneHandGestureIdx].GestureState.GestureDistance += KinematicVelocityComponent->GetDeltaLocation().Size();
			}
			return true;
		}
	}

	// If the gesture is invalid, reset the active gesture
	else
	{
		SetActiveOneHandGestureIdx();
	}
	return false;
}

EEmpathTeam AEmpathHandActor::GetTeamNum_Implementation() const
{
	// We should always return player for VR characters
	return EEmpathTeam::Player;
}

void AEmpathHandActor::UpdateGestureConditionChecks()
{
	if (CanGestureCast())
	{
		// Cache velocity values
		GestureConditionCheck.CheckVelocity = KinematicVelocityComponent->GetComponentTransform().InverseTransformVectorNoScale(KinematicVelocityComponent->GetKinematicVelocity());
		GestureConditionCheck.VelocityMagnitude = GestureConditionCheck.CheckVelocity.Size();
		GestureConditionCheck.AngularVelocity = KinematicVelocityComponent->GetKAngularVelocityLocal();
		GestureConditionCheck.ScaledAngularVelocity = GestureConditionCheck.AngularVelocity / GestureConditionCheck.VelocityMagnitude;
		GestureConditionCheck.SphericalVelocity = KinematicVelocityComponent->GetSphericalVelocity();
		GestureConditionCheck.RadialVelocity = KinematicVelocityComponent->GetRadialVelocity();
		GestureConditionCheck.VerticalVelocity = KinematicVelocityComponent->GetVerticalVelocity();
		
		// Cache distances
		GestureConditionCheck.SphericalDist = KinematicVelocityComponent->GetLastSphericalDist();
		GestureConditionCheck.RadialDist = KinematicVelocityComponent->GetLastRadialDist();
		GestureConditionCheck.VerticalDist = KinematicVelocityComponent->GetLastVerticalDist();

		// Cache accelerations
		GestureConditionCheck.CheckAcceleration = KinematicVelocityComponent->GetComponentTransform().InverseTransformVectorNoScale(KinematicVelocityComponent->GetKinematicAcceleration());
		GestureConditionCheck.AccelMagnitude = GestureConditionCheck.CheckAcceleration.Size();
		GestureConditionCheck.AngularAcceleration = KinematicVelocityComponent->GetAngularAccelLocal();
		GestureConditionCheck.SphericalAccel = KinematicVelocityComponent->GetSphericalAccel();
		GestureConditionCheck.RadialAccel = KinematicVelocityComponent->GetRadialAccel();
		GestureConditionCheck.VerticalAccel = KinematicVelocityComponent->GetVerticalAccel();

		// Cache distance between hands and interior angle to the other hand
		if (OtherHand && OtherHand->KinematicVelocityComponent)
		{
			FVector DirectionToOtherHand = OtherHand->KinematicVelocityComponent->GetComponentLocation() - KinematicVelocityComponent->GetComponentLocation();
			GestureConditionCheck.DistBetweenHands = DirectionToOtherHand.Size();
			GestureConditionCheck.InteriorAngleToOtherHand = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(KinematicVelocityComponent->GetComponentTransform().TransformVectorNoScale(FVector(0.0f, (OwningHand == EEmpathBinaryHand::Left ? 1.0f : -1.0f), 0.0f)), DirectionToOtherHand.GetSafeNormal())));
		}
		else
		{
			GestureConditionCheck.DistBetweenHands = 0.0f;
			GestureConditionCheck.InteriorAngleToOtherHand = 0.0f;
		}

		// Calculate motion angles
		FVector VelocityNormalized = GestureConditionCheck.CheckVelocity.GetSafeNormal();
		GestureConditionCheck.MotionAngle.X  = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FVector(1.0f, 0.0f, 0.0f), VelocityNormalized)));
		GestureConditionCheck.MotionAngle.Y = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FVector(0.0f, 1.0f, 0.0f), VelocityNormalized)));
		GestureConditionCheck.MotionAngle.Z = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FVector(0.0f, 0.0f, 1.0f), VelocityNormalized)));

		// Cache frame velocity values
		FrameConditionCheck.CheckVelocity = KinematicVelocityComponent->GetComponentTransform().InverseTransformVectorNoScale(KinematicVelocityComponent->GetFrameVelocity());
		FrameConditionCheck.VelocityMagnitude = FrameConditionCheck.CheckVelocity.Size();
		FrameConditionCheck.AngularVelocity = KinematicVelocityComponent->GetFrameAngularVelocityLocal();
		FrameConditionCheck.ScaledAngularVelocity = FrameConditionCheck.AngularVelocity / FrameConditionCheck.VelocityMagnitude;
		FrameConditionCheck.SphericalVelocity = KinematicVelocityComponent->GetFrameSphericalVelocity();
		FrameConditionCheck.RadialVelocity = KinematicVelocityComponent->GetFrameRadialVelocity();
		FrameConditionCheck.VerticalVelocity = KinematicVelocityComponent->GetFrameVerticalVelocity();
		
		// Cache frame distances
		FrameConditionCheck.SphericalDist = KinematicVelocityComponent->GetLastSphericalDist();
		FrameConditionCheck.RadialDist = KinematicVelocityComponent->GetLastRadialDist();
		FrameConditionCheck.VerticalDist = KinematicVelocityComponent->GetLastVerticalDist();

		// Cache accelerations
		FrameConditionCheck.CheckAcceleration = KinematicVelocityComponent->GetComponentTransform().InverseTransformVectorNoScale(KinematicVelocityComponent->GetFrameAcceleration());
		FrameConditionCheck.AccelMagnitude = FrameConditionCheck.CheckAcceleration.Size();
		FrameConditionCheck.AngularAcceleration = KinematicVelocityComponent->GetFrameAngularAccelLocal();
		FrameConditionCheck.SphericalAccel = KinematicVelocityComponent->GetFrameSphericalAccel();
		FrameConditionCheck.RadialAccel = KinematicVelocityComponent->GetFrameRadialAccel();
		FrameConditionCheck.VerticalAccel = KinematicVelocityComponent->GetFrameVerticalAccel();

		// Calculate frame motion angles
		VelocityNormalized = FrameConditionCheck.CheckVelocity.GetSafeNormal();
		FrameConditionCheck.MotionAngle.X = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FVector(1.0f, 0.0f, 0.0f), VelocityNormalized)));
		FrameConditionCheck.MotionAngle.Y = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FVector(0.0f, 1.0f, 0.0f), VelocityNormalized)));
		FrameConditionCheck.MotionAngle.Z = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FVector(0.0f, 0.0f, 1.0f), VelocityNormalized)));

		// Cache distance between hands and interior angle to other hand
		// This should be the same as the normal condition check
		FrameConditionCheck.DistBetweenHands = GestureConditionCheck.DistBetweenHands;
		FrameConditionCheck.InteriorAngleToOtherHand = GestureConditionCheck.InteriorAngleToOtherHand;
	}

	return;
}

void AEmpathHandActor::SetIsPowerCharged(const bool bNewIsCharged)
{
	if (bNewIsCharged != bIsPowerCharged)
	{
		bIsPowerCharged = bNewIsCharged;
		if (bIsPowerCharged)
		{
			OnPowerChargedStart();
		}
		else
		{
			OnPowerChargedEnd();
		}
		if (OwningPlayerCharacter)
		{
			OwningPlayerCharacter->OnNewChargedState(bIsPowerCharged, OwningHand);
		}
	}
	return;
}

void AEmpathHandActor::OnChargePressed()
{
	if (!bChargePressed)
	{
		bChargePressed = true;
		ReceiveChargePressed();
	}
	return;
}

void AEmpathHandActor::OnChargeReleased()
{
	if (bChargePressed)
	{
		bChargePressed = false;
		ReceiveChargePressed();
	}
	return;
}

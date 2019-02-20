// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathLinkedProjectile.h"
#include "Classes/GameFramework/ProjectileMovementComponent.h"
#include "Classes/Components/CapsuleComponent.h"
#include "EmpathGameModeBase.h"
#include "EmpathFunctionLibrary.h"
#include "EmpathFlammableInterface.h"
#include "Classes/Components/BoxComponent.h"
#include "EmpathDamageType.h"

FName AEmpathLinkedProjectile::HeadComponentName(TEXT("HeadComponent"));
FName AEmpathLinkedProjectile::AirTailComponentName(TEXT("AirTailComponent"));
FName AEmpathLinkedProjectile::GroundTailComponentName(TEXT("GroundTailComponent"));

AEmpathLinkedProjectile::AEmpathLinkedProjectile(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// Initialize components
	// Head
	LinkHead = CreateDefaultSubobject<USceneComponent>(HeadComponentName);
	RootComponent = LinkHead;

	// Air Tail
	LinkAirTail = CreateDefaultSubobject<UCapsuleComponent>(AirTailComponentName);
	LinkAirTail->SetCollisionProfileName(FEmpathCollisionProfiles::Projectile);
	LinkAirTail->SetEnableGravity(false);
	LinkAirTail->SetCapsuleRadius(5.0f);
	LinkAirTail->SetCapsuleHalfHeight(5.0f);
	LinkAirTail->SetupAttachment(RootComponent);

	// Ground tail
	LinkGroundTail = CreateDefaultSubobject<UBoxComponent>(GroundTailComponentName);
	LinkGroundTail->SetCollisionProfileName(FEmpathCollisionProfiles::Projectile);
	LinkGroundTail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LinkGroundTail->SetEnableGravity(false);
	LinkGroundTail->SetBoxExtent(FVector(1.0f, 1.0f, 1.0f));
	LinkGroundTail->SetupAttachment(RootComponent);

	// Initialize variables
	ProjectileMovement->Velocity = FVector(1500.0f, 0.0f, 0.0f);
	ProjectileMovement->UpdatedComponent = RootComponent;
	Team = EEmpathTeam::Player;
	LifetimeAfterSpawn = 2.0f;
	MinLifetime = 0.2f;
	GroundedLifetime = 3.0f;
	MaxTailLength = 250.0f;
	GroundTailWidth = 32.0f;
	GroundTailMinHeight = 200.0f;
	GroundedDamageAmountPerImpact = 1.0f;
	GroundedDamageAmountPerSecond = 1.0f;
	InsertedLinkClass = AEmpathLinkedProjectile::StaticClass();
	GroundedDamageTypeClass = UEmpathDamageType::StaticClass();
}

void AEmpathLinkedProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Register the pulse
	if (UWorld* World = GetWorld())
	{
		EmpathGameMode = Cast<AEmpathGameModeBase>(GetWorld()->GetAuthGameMode());
		if (EmpathGameMode)
		{
			EmpathGameMode->OnPulse.AddDynamic(this, &AEmpathLinkedProjectile::OnPulse);
		}
	}
}

void AEmpathLinkedProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister the pulse
	if (EmpathGameMode)
	{
		EmpathGameMode->OnPulse.RemoveDynamic(this, &AEmpathLinkedProjectile::OnPulse);
	}
	RemoveLink();
	Super::EndPlay(EndPlayReason);
}

void AEmpathLinkedProjectile::OnPulse(float PulseDeltaTime)
{

	UpdateAirTailTransform();

	ReceivePulse(PulseDeltaTime);
}

AEmpathLinkedProjectile* AEmpathLinkedProjectile::InsertLink()
{
	if (InsertedLinkClass && GroundedState != EEmpathSlashGroundedState::GroundedTail && !bDead && LinkAirTail && TailTarget && LinkHead && GetRemainingLifetime() >= MinLifetime)
	{
		// Calculate the new link transform
		FVector HeadLoc = LinkHead->GetComponentLocation();
		FVector TargetLoc = TailTarget->GetComponentLocation();
		FVector NewLinkLoc = (HeadLoc + TargetLoc) / 2.0f;
		FQuat NewLinkRot = FQuat::Slerp(FQuat(GetActorRotation()), FQuat(TailTarget->GetComponentRotation()), 0.5f);
		NewLinkRot.Normalize();
		
		// Spawn the link
		FActorSpawnParameters SpawnParams;
		AEmpathLinkedProjectile* NewLink = GetWorld()->SpawnActor<AEmpathLinkedProjectile>(InsertedLinkClass, NewLinkLoc, NewLinkRot.Rotator(), SpawnParams);
		NewLink->TailTarget = TailTarget;
		NewLink->StartDeathTimer(GetRemainingLifetime());
		
		// If we had a previous, set its next to the new link and vice versa
		if (PreviousLink)
		{
			PreviousLink->NextLink = NewLink;
			NewLink->PreviousLink = PreviousLink;
		}

		// Set the new link's next to this one and vice versa
		NewLink->NextLink = this;
		PreviousLink = NewLink;
		TailTarget = NewLink->LinkHead;

		// Tell the new link to update its transform
		NewLink->UpdateAirTailTransform();
		UpdateAirTailTransform();

		return NewLink;
	}

	return nullptr;
}

void AEmpathLinkedProjectile::DetachFromNext()
{
	if (NextLink)
	{
		NextLink->DetachFromPrevious();
	}
}

void AEmpathLinkedProjectile::DetachFromPrevious()
{
	// Destroy tail
	if (LinkAirTail)
	{
		OnBeforeAirTailDestroyed();
		LinkAirTail->DestroyComponent(false);
		LinkAirTail = nullptr;
	}
	TailTarget = nullptr;

	// Update previous link if necessary
	if (PreviousLink)
	{
		PreviousLink->NextLink = nullptr;

		// If the previous link has no tail of its own, destroy it
		if (!PreviousLink->TailTarget)
		{
			PreviousLink->Die();
		}
		else
		{
			PreviousLink->OnBecomeFirstLink();
		}
	}

	// Die if there is no next link
	if (!NextLink)
	{
		Die();
	}

	return;
}

void AEmpathLinkedProjectile::RemoveLink()
{
	DetachFromPrevious();
	DetachFromNext();
}

void AEmpathLinkedProjectile::Die()
{
	if (!bDead)
	{
		AEmpathProjectile::Die();
		RemoveLink();
	}

}

TArray<FVector> AEmpathLinkedProjectile::GetPreviousChainPoints()
{
	TArray<FVector> RetVal;
	AEmpathLinkedProjectile* CurrLink = this;
	if (CurrLink->LinkHead && !CurrLink->bDead)
	{
		RetVal.Add(CurrLink->LinkHead->GetComponentTransform().GetLocation());
		while (CurrLink->PreviousLink && CurrLink->PreviousLink->LinkHead && !CurrLink->PreviousLink->bDead)
		{
			RetVal.Add(CurrLink->PreviousLink->LinkHead->GetComponentTransform().GetLocation());
			CurrLink = CurrLink->PreviousLink;
		}
		if (CurrLink->TailTarget)
		{
			RetVal.Add(CurrLink->TailTarget->GetComponentTransform().GetLocation());
		}
	}
	return RetVal;
}

AEmpathLinkedProjectile* AEmpathLinkedProjectile::GetFirstAirTailLink()
{
	if (!bDead && LinkAirTail)
	{
		AEmpathLinkedProjectile* CurrLink = this;
		while (CurrLink->NextLink && !CurrLink->NextLink->bDead && CurrLink->NextLink->LinkAirTail)
		{
			CurrLink = CurrLink->NextLink;
		}
		return CurrLink;
	}
	return nullptr;
}

void AEmpathLinkedProjectile::GetPreviousAirTailLinksAndPoints(TArray<AEmpathLinkedProjectile*>& OutLinks, TArray<FVector>& OutLinkPoints, USceneComponent*& OutFinalTailTarget)
{
	// Empty variables to start
	OutLinks.Empty();
	OutLinkPoints.Empty();
	OutFinalTailTarget = nullptr;

	// Add this link if valid
	AEmpathLinkedProjectile* CurrLink = this;
	if (!CurrLink->bDead &&CurrLink->LinkHead && CurrLink->LinkAirTail)
	{
		OutLinks.Add(CurrLink);
		OutLinkPoints.Add(CurrLink->LinkHead->GetComponentTransform().GetLocation());

		// Loop through and add valid previous links
		while (CurrLink->PreviousLink && CurrLink->PreviousLink->LinkHead && !CurrLink->PreviousLink->bDead && CurrLink->PreviousLink->LinkAirTail)
		{
			OutLinks.Add(CurrLink->PreviousLink);
			OutLinkPoints.Add(CurrLink->PreviousLink->LinkHead->GetComponentTransform().GetLocation());
			CurrLink = CurrLink->PreviousLink;
		}

		// Add final tail target if valid
		if (CurrLink->TailTarget)
		{
			OutFinalTailTarget = CurrLink->TailTarget;
			OutLinkPoints.Add(OutFinalTailTarget->GetComponentTransform().GetLocation());
		}
	}

	return;
}

TArray<AEmpathLinkedProjectile*> AEmpathLinkedProjectile::GetPreviousAirTailLinks()
{
	// Empty variables to start
	TArray<AEmpathLinkedProjectile*> RetVal;

	// Add this link if valid
	AEmpathLinkedProjectile* CurrLink = this;
	if (!CurrLink->bDead && CurrLink->LinkAirTail)
	{
		RetVal.Add(CurrLink);

		// Loop through and add valid previous links
		while (CurrLink->PreviousLink && !CurrLink->PreviousLink->bDead && CurrLink->PreviousLink->LinkAirTail)
		{
			RetVal.Add(CurrLink->PreviousLink);
			CurrLink = CurrLink->PreviousLink;
		}
	}

	return RetVal;
}

TArray<FVector> AEmpathLinkedProjectile::GetPreviousAirTailPoints()
{
	TArray<FVector> RetVal;
	AEmpathLinkedProjectile* CurrLink = this;
	if (CurrLink->LinkHead && !CurrLink->bDead && CurrLink->LinkAirTail)
	{
		RetVal.Add(CurrLink->LinkHead->GetComponentTransform().GetLocation());
		while (CurrLink->PreviousLink && CurrLink->PreviousLink->LinkHead && !CurrLink->PreviousLink->bDead && CurrLink->PreviousLink->LinkAirTail)
		{
			RetVal.Add(CurrLink->PreviousLink->LinkHead->GetComponentTransform().GetLocation());
			CurrLink = CurrLink->PreviousLink;
		}
		if (CurrLink->TailTarget)
		{
			RetVal.Add(CurrLink->TailTarget->GetComponentTransform().GetLocation());
		}
	}
	return RetVal;
}

AEmpathLinkedProjectile* AEmpathLinkedProjectile::GetFirstGroundTailLink()
{
	if (!bDead 
		&& (GroundedState == EEmpathSlashGroundedState::GroundedTail
			|| (NextLink && NextLink->GroundedState == EEmpathSlashGroundedState::GroundedTail)))
	{
		AEmpathLinkedProjectile* CurrOldest = this;
		while (CurrOldest->NextLink && !CurrOldest->NextLink->bDead && CurrOldest->NextLink->GroundedState == EEmpathSlashGroundedState::GroundedTail)
		{
			CurrOldest = CurrOldest->NextLink;
		}
		return CurrOldest;
	}
	return nullptr;
}

void AEmpathLinkedProjectile::GetPreviousGroundTailLinksAndPoints(TArray<AEmpathLinkedProjectile*>& OutLinks, TArray<FVector>& OutLinkPoints, USceneComponent*& OutFinalTailTarget)
{
	// Empty variables to start
	OutLinks.Empty();
	OutLinkPoints.Empty();
	OutFinalTailTarget = nullptr;

	// Add this link if valid
	AEmpathLinkedProjectile* CurrLink = this;
	if (!CurrLink->bDead &&CurrLink->LinkHead && CurrLink->GroundedState == EEmpathSlashGroundedState::GroundedTail)
	{
		OutLinks.Add(CurrLink);
		OutLinkPoints.Add(CurrLink->LinkHead->GetComponentTransform().GetLocation());

		// Loop through and add valid previous links
		while (CurrLink->PreviousLink && CurrLink->PreviousLink->LinkHead && !CurrLink->PreviousLink->bDead && CurrLink->PreviousLink->GroundedState != EEmpathSlashGroundedState::Airborne)
		{
			OutLinks.Add(CurrLink->PreviousLink);
			OutLinkPoints.Add(CurrLink->PreviousLink->LinkHead->GetComponentTransform().GetLocation());
			CurrLink = CurrLink->PreviousLink;
		}
	}

	return;
}

TArray<AEmpathLinkedProjectile*> AEmpathLinkedProjectile::GetPreviousGroundTailLinks()
{
	// Empty variables to start
	TArray<AEmpathLinkedProjectile*> RetVal;

	// Add this link if valid
	AEmpathLinkedProjectile* CurrLink = this;
	if (!CurrLink->bDead && CurrLink->GroundedState == EEmpathSlashGroundedState::GroundedTail)
	{
		RetVal.Add(CurrLink);

		// Loop through and add valid previous links
		while (CurrLink->PreviousLink && !CurrLink->PreviousLink->bDead && CurrLink->GroundedState != EEmpathSlashGroundedState::Airborne)
		{
			RetVal.Add(CurrLink->PreviousLink);
			CurrLink = CurrLink->PreviousLink;
		}
	}

	return RetVal;
}

TArray<FVector> AEmpathLinkedProjectile::GetPreviousGroundTailPoints()
{
	TArray<FVector> RetVal;
	AEmpathLinkedProjectile* CurrLink = this;
	if (CurrLink->LinkHead && !CurrLink->bDead && CurrLink->GroundedState == EEmpathSlashGroundedState::GroundedTail)
	{
		RetVal.Add(CurrLink->LinkHead->GetComponentTransform().GetLocation());
		while (CurrLink->PreviousLink && CurrLink->PreviousLink->LinkHead && !CurrLink->PreviousLink->bDead && CurrLink->PreviousLink->GroundedState != EEmpathSlashGroundedState::Airborne)
		{
			RetVal.Add(CurrLink->PreviousLink->LinkHead->GetComponentTransform().GetLocation());
			CurrLink = CurrLink->PreviousLink;
		}
	}
	return RetVal;
}

void AEmpathLinkedProjectile::OnBecomeFirstGroundTailLink()
{
	ReceiveBecomeFirstGroundTailLink();
	if (PreviousLink && PreviousLink->GroundedState == EEmpathSlashGroundedState::GroundedTail)
	{
		PreviousLink->OnStopBeingFirstGroundTailLink();
	}
}

void AEmpathLinkedProjectile::OnBecomeFirstLink()
{
	ReceiveBecomeFirstLink();
	if (LinkAirTail)
	{
		OnBecomeFirstAirTailLink();
	}
	else
	{
		OnBecomeFirstGroundTailLink();
	}
}

AEmpathLinkedProjectile* AEmpathLinkedProjectile::GetFirstLink()
{
	if (!bDead)
	{
		AEmpathLinkedProjectile* CurrOldest = this;
		while (CurrOldest->NextLink && !CurrOldest->NextLink->bDead)
		{
			CurrOldest = CurrOldest->NextLink;
		}
		return CurrOldest;
	}
	return nullptr;
}

void AEmpathLinkedProjectile::BecomeGrounded()
{
	FVector GroundedLocation;
	if (GetBestGroundedLocation(GroundedLocation))
	{
		// Update the grounded state
		GroundedState = EEmpathSlashGroundedState::GroundedHead;

		// Momentarily disable tail collision
		if (LinkAirTail)
		{
			LinkAirTail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}


		// Place the link on the ground
		SetActorLocation(GroundedLocation);
		if (ProjectileMovement)
		{
			ProjectileMovement->StopMovementImmediately();
		}

		// Instruct the next link to ground its tail if appropriate
		if (NextLink)
		{
			NextLink->ActivateGroundTail();
		}

		// If our previous link was grounded, ground our tail
		if (PreviousLink)
		{
			ActivateGroundTail();
		}

		// Else, set our air tail to active again and update it
		else
		{
			if (LinkAirTail)
			{
				UpdateAirTailTransform();
				LinkAirTail->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			}
		}

		OnBecomeGrounded();
	}
	else
	{
		Die();
	}

	return;
}

bool AEmpathLinkedProjectile::GetBestGroundedLocation(FVector& OutGroundedLocation)
{
	// Multi line trace to find the best location
	TArray<FHitResult> HitResults;
	FVector TraceStart = GetActorLocation();
	FVector TraceEnd = TraceStart;

	// Start a bit above us and search a bit below us
	TraceStart.Z += 250.0f;
	TraceEnd.Z -= 500.0f;

	// Ignore this actor
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// Perform the trace
	UWorld* World = GetWorld();
	if (World)
	{
		bool bHit = World->LineTraceMultiByChannel(HitResults, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams);
		if (bHit)
		{
			// Check its hit
			for (FHitResult& HitResult : HitResults)
			{
				// Check if hit was blocking and actor is a flammable static
				if (HitResult.bBlockingHit)
				{
					AActor* HitActor = HitResult.Actor.Get();
					if (HitActor && HitActor->GetClass()->ImplementsInterface(UEmpathFlammableInterface::StaticClass()))
					{
						if (IEmpathFlammableInterface::Execute_GetFlammableType(HitActor) == EEmpathFlammableType::FlammableStatic)
						{
							// If so, return the location hit
							OutGroundedLocation = HitResult.Location;
							return true;
						}
					}
				}
			}
		}

		// If we had no hits, try to derive a location from our next and previous link
		// Check previous link
		if (PreviousLink && PreviousLink->GroundedState != EEmpathSlashGroundedState::Airborne)
		{
			OutGroundedLocation = GetActorLocation();

			// Use both links if appropriate
			if (NextLink && NextLink->GroundedState != EEmpathSlashGroundedState::Airborne)
			{
				OutGroundedLocation.Z = FMath::Min3(OutGroundedLocation.Z, NextLink->GetActorLocation().Z, PreviousLink->GetActorLocation().Z);
				return true;
			}

			// Otherwise just use the previous link
			else
			{
				OutGroundedLocation.Z = FMath::Min3(OutGroundedLocation.Z, PreviousLink->GetActorLocation().Z, 0.0f);
				return true;
			}
		}
		// Otherwise check next link
		else if (NextLink && NextLink->GroundedState != EEmpathSlashGroundedState::Airborne)
		{
			OutGroundedLocation = GetActorLocation();
			OutGroundedLocation.Z = FMath::Min3(OutGroundedLocation.Z, NextLink->GetActorLocation().Z, 0.0f);
			return true;
		}
	}

	// Return false if we find no appropriate hits
	OutGroundedLocation = FVector::ZeroVector;
	return false;

}

const bool AEmpathLinkedProjectile::IsLastLink() const
{
	return (!TailTarget && NextLink);
}

const bool AEmpathLinkedProjectile::IsFirstLink() const
{
	return (TailTarget && !NextLink);
}

void AEmpathLinkedProjectile::UpdateAirTailTransform()
{
	if (GroundedState != EEmpathSlashGroundedState::GroundedTail && !bDead && LinkAirTail && TailTarget && LinkHead)
	{
		// Get the distance from the new head to the target
		FVector HeadLoc = LinkHead->GetComponentLocation();
		FVector TargetLoc = TailTarget->GetComponentLocation();
		FVector DirFromHeadToTarget = HeadLoc - TargetLoc;

		// If the distance would be too great, then insert a link or detach appropriately
		if (UEmpathFunctionLibrary::DistanceGreaterThan(HeadLoc, TargetLoc, MaxTailLength))
		{
			if (GetRemainingLifetime() >= MinLifetime && InsertedLinkClass)
			{
				if (PreviousLink)
				{
					if (PreviousLink->GroundedState == EEmpathSlashGroundedState::Airborne
						&& GroundedState == EEmpathSlashGroundedState::Airborne)
					{
						InsertLink();
					}
					else
					{
						DetachFromPrevious();
					}
				}
				else
				{
					InsertLink();
				}
			}
			else
			{
				DetachFromPrevious();
			}
		}
		else
		{
			// Initialize the new transform
			FTransform NewTailTransform;

			// Calculate the location and rotation
			NewTailTransform.SetLocation((TargetLoc + HeadLoc) / 2.0f);
			NewTailTransform.SetRotation(FRotationMatrix::MakeFromZ(DirFromHeadToTarget).ToQuat());
			LinkAirTail->SetWorldTransform(NewTailTransform);

			// Relocating the tail may have invalidated it, so check before setting the capsule height
			if (GroundedState != EEmpathSlashGroundedState::GroundedTail && !bDead && LinkAirTail && TailTarget && LinkHead)
			{
				LinkAirTail->SetCapsuleHalfHeight(DirFromHeadToTarget.Size() / 2.0f);
			}
		}
	}
	return;
}

void AEmpathLinkedProjectile::ActivateGroundTail()
{
	// Ensure the conditions for activating the ground tail are valid
	if (!bDead 
		&& LinkHead
		&& LinkAirTail
		&& LinkGroundTail
		&& TailTarget
		&& GroundedState == EEmpathSlashGroundedState::GroundedHead
		&& PreviousLink
		&& PreviousLink->GroundedState != EEmpathSlashGroundedState::Airborne)
	{
		// Update the grounded state
		GroundedState = EEmpathSlashGroundedState::GroundedTail;

		// Set the new lifetime
		float NewLifeTime = GroundedLifetime;
		if (NextLink && NextLink->GroundedState == EEmpathSlashGroundedState::GroundedTail)
		{
			if (PreviousLink->GroundedState != EEmpathSlashGroundedState::GroundedTail)
			{
				NewLifeTime = FMath::Max3(MinLifetime, NextLink->GetRemainingLifetime(), 0.0f);
			}
			else
			{
				NewLifeTime = FMath::Max3(MinLifetime, (NextLink->GetRemainingLifetime() + PreviousLink->GetRemainingLifetime()) / 2.0f, 0.0f);
				PreviousLink->OnStopBeingFirstGroundTailLink();
			}
		}
		else
		{
			OnBecomeFirstGroundTailLink();
			if (PreviousLink->GroundedState == EEmpathSlashGroundedState::GroundedTail)
			{
				NewLifeTime = FMath::Max3(MinLifetime, PreviousLink->GetRemainingLifetime(), 0.0f);
			}
		}
		StartDeathTimer(NewLifeTime);

		// Destroy the air tail
		OnBeforeAirTailDestroyed();
		LinkAirTail->DestroyComponent();
		LinkAirTail = nullptr;

		// Position and scale the ground tail
		// Calc location
		FVector HeadLoc = LinkHead->GetComponentLocation();
		FVector TargetLoc = PreviousLink->LinkHead->GetComponentLocation();
		FVector TailLoc = (HeadLoc + TargetLoc) / 2.0f;

		// Calc rotation
		FVector DirToTarget = (TargetLoc - HeadLoc);
		FQuat TailRot = FRotationMatrix::MakeFromZY(FVector(0.0f, 0.0f, 1.0f), DirToTarget).ToQuat();

		// Calc scale
		float GroundTailHeightOffset = FMath::Abs(HeadLoc.Z - TargetLoc.Z);
		float TailLength = DirToTarget.Size();

		// Update ground tail size and location
		LinkGroundTail->SetBoxExtent(FVector(GroundTailWidth / 2.0f, TailLength / 2.0f, (GroundTailHeightOffset + GroundTailMinHeight) / 2.0f), false);
		LinkGroundTail->SetWorldLocationAndRotation(TailLoc, TailRot, false);

		// Activate collision on the ground tail
		LinkGroundTail->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TailTarget = nullptr;
		OnGroundTailActivated();
	}

	return;
}

void AEmpathLinkedProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ApplyGroundedDamageOverTime(DeltaTime);
}

void AEmpathLinkedProjectile::OnImpact_Implementation(UPrimitiveComponent* ImpactingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, const FHitResult& HitResult)
{
	switch (GroundedState)
	{
	case EEmpathSlashGroundedState::Airborne:
	{
		// By default, apply point damage to the other actor and trigger the die event
		FPointDamageEvent DamageEvent(ImpactDamageAmount, HitResult, GetActorForwardVector(), DamageTypeClass);
		OtherActor->TakeDamage(ImpactDamageAmount, DamageEvent, GetInstigatorController(), this);

		// Check if the other object is still valid and a flammable static.
		if (OtherActor && OtherActor->GetClass()->ImplementsInterface(UEmpathFlammableInterface::StaticClass()))
		{
			if (IEmpathFlammableInterface::Execute_GetFlammableType(OtherActor) == EEmpathFlammableType::FlammableStatic)
			{
				BecomeGrounded();
				return;
			}
		}

		// Otherwise, begin the die response.
		Die();
		break;
	}
	case EEmpathSlashGroundedState::GroundedHead:
	{
		// By default, apply point damage to the other actor and trigger the die event
		FPointDamageEvent DamageEvent(ImpactDamageAmount, HitResult, GetActorForwardVector(), DamageTypeClass);
		OtherActor->TakeDamage(ImpactDamageAmount, DamageEvent, GetInstigatorController(), this);

		// Check if the other object is a flammable static.
		if (OtherActor->GetClass()->ImplementsInterface(UEmpathFlammableInterface::StaticClass()))
		{
			if (IEmpathFlammableInterface::Execute_GetFlammableType(OtherActor) == EEmpathFlammableType::FlammableStatic)
			{
				// Since we're already grounded, do nothing
				return;
			}
		}

		// Otherwise, begin the die response.
		Die();
		break;
	}
	case EEmpathSlashGroundedState::GroundedTail:
	{
		// Apply ground damage
		FDamageEvent DamageEvent(GroundedDamageTypeClass);
		OtherActor->TakeDamage(GroundedDamageAmountPerImpact, DamageEvent, GetInstigatorController(), this);
		break;
	}
	default:
	{
		break;
	}
	}


	return;
}

void AEmpathLinkedProjectile::ApplyGroundedDamageOverTime(const float DeltaTime)
{
	// Damage overlapping actors if appropriate
	if (!bDead && GroundedState == EEmpathSlashGroundedState::GroundedTail && GroundedDamageAmountPerSecond > 0.0f && LinkGroundTail)
	{
		// Initialize variables
		TSet<AActor*> DamagedActors;
		TArray<UPrimitiveComponent*> OverlappingComps;
		LinkGroundTail->GetOverlappingComponents(OverlappingComps);

		// Loop through overlapping components
		for (UPrimitiveComponent* CurrComp : OverlappingComps)
		{
			if (CurrComp)
			{
				AActor* OtherActor = CurrComp->GetOwner();
				if (OtherActor && !DamagedActors.Contains(OtherActor) && ShouldTriggerImpact(LinkGroundTail, CurrComp->GetOwner(), CurrComp))
				{
					// Apply ground damage
					FDamageEvent DamageEvent(GroundedDamageTypeClass);
					OtherActor->TakeDamage(GroundedDamageAmountPerSecond * DeltaTime, DamageEvent, GetInstigatorController(), this);
					DamagedActors.Add(OtherActor);
				}
			}
		}
	}
}

void AEmpathLinkedProjectile::GetPreviousChainLinksAndPoints(TArray<AEmpathLinkedProjectile*>& OutLinks, TArray<FVector>& OutLinkPoints, USceneComponent*& OutFinalTailTarget)
{
	// Empty variables to start
	OutLinks.Empty();
	OutLinkPoints.Empty();
	OutFinalTailTarget = nullptr;

	// Add this link if valid
	AEmpathLinkedProjectile* CurrLink = this;
	if (!CurrLink->bDead &&CurrLink->LinkHead)
	{
		OutLinks.Add(CurrLink);
		OutLinkPoints.Add(CurrLink->LinkHead->GetComponentTransform().GetLocation());

		// Loop through and add valid previous links
		while (CurrLink->PreviousLink && CurrLink->PreviousLink->LinkHead && !CurrLink->PreviousLink->bDead)
		{
			OutLinks.Add(CurrLink->PreviousLink);
			OutLinkPoints.Add(CurrLink->PreviousLink->LinkHead->GetComponentTransform().GetLocation());
			CurrLink = CurrLink->PreviousLink;
		}

		// Add final tail target if valid
		if (CurrLink->TailTarget)
		{
			OutFinalTailTarget = CurrLink->TailTarget;
			OutLinkPoints.Add(OutFinalTailTarget->GetComponentTransform().GetLocation());
		}
	}

	return;
}

TArray<AEmpathLinkedProjectile*> AEmpathLinkedProjectile::GetPreviousChainLinks()
{
	// Empty variables to start
	TArray<AEmpathLinkedProjectile*> RetVal;

	// Add this link if valid
	AEmpathLinkedProjectile* CurrLink = this;
	if (!CurrLink->bDead)
	{
		RetVal.Add(CurrLink);

		// Loop through and add valid previous links
		while (CurrLink->PreviousLink && !CurrLink->PreviousLink->bDead)
		{
			RetVal.Add(CurrLink->PreviousLink);
			CurrLink = CurrLink->PreviousLink;
		}
	}

	return RetVal;
}

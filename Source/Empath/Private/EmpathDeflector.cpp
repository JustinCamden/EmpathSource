// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathDeflector.h"
#include "EmpathIgnoreCollisionInterface.h"
#include "EmpathDeflectableInterface.h"
#include "EmpathFunctionLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "TimerManager.h"

AEmpathDeflector::AEmpathDeflector(const FObjectInitializer& ObjectInitializer)
{
	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;
	InitialDeflectedActorsClearTime = 0.2f;
}

void AEmpathDeflector::SetDeflectActive(const bool bNewActive)
{
	if (bNewActive != bDeflectActive)
	{
		bDeflectActive = bNewActive;
		if (bNewActive)
		{
			OnDeflectActivated();
		}
		else
		{
			InitialDeflectedActors.Empty();
			OnDeflectDeactivated();
		}
	}
	return;
}

// Called when the game starts or when spawned
void AEmpathDeflector::BeginPlay()
{
	Super::BeginPlay();
	
}

bool AEmpathDeflector::ShouldTriggerDeflect_Implementation(const UPrimitiveComponent* DeflectingComponent, const AActor* OtherActor, const UPrimitiveComponent* OtherActorComponent)
{
	// Protect from invalid input
	if (!OtherActor || !OtherActorComponent || !DeflectingComponent)
	{
		return false;
	}
	if (OtherActorComponent->GetOwner() != OtherActor || DeflectingComponent->GetOwner() != this)
	{
		return false;
	}
	if (InitialDeflectedActors.Num() > 0)
	{
		AActor* const MutableOther = const_cast<AActor*>(OtherActor);
		if (InitialDeflectedActors.Contains(MutableOther))
		{
			return false;
		}
	}

	// Check if we should ignore collision with the other actor
	if (OtherActor->GetClass()->ImplementsInterface(UEmpathIgnoreCollisionInterface::StaticClass()))
	{
		if (IEmpathIgnoreCollisionInterface::Execute_ShouldOtherActorIgnoreCollision(OtherActor, OtherActorComponent))
		{
			return false;
		}
	}

	// Don't impact friendly actors unless deflect friendlies is enabled
	if (!bEnableDeflectFriendlies && UEmpathFunctionLibrary::GetActorTeam(OtherActor) == Team)
	{
		return false;
	}

	// Check if we are deflecting
	if (IEmpathDeflectorInterface::Execute_IsDeflecting(this, DeflectingComponent))
	{
		// If so check if the other actor is deflectable
		if (OtherActor->GetClass()->ImplementsInterface(UEmpathDeflectableInterface::StaticClass()))
		{
			if (IEmpathDeflectableInterface::Execute_IsDeflectable(OtherActor, OtherActorComponent))
			{
				return true;
			}
		}
	}

	return false;
}

const bool AEmpathDeflector::AttemptDeflectActor(UPrimitiveComponent* DeflectingComponent, AActor* OtherActor, UPrimitiveComponent* OtherActorComponent, UPARAM(ref) const FHitResult& HitResult)
{
	// Check if we should trigger deflect
	if (ShouldTriggerDeflect(DeflectingComponent, OtherActor, OtherActorComponent))
	{
		// Protect against invalid input
		if (OtherActor->GetClass()->ImplementsInterface(UEmpathDeflectableInterface::StaticClass()))
		{
			// If successful, perform the deflect and call the deflect event
			FVector DeflectionImpulse = GetDeflectionImpulse();
			IEmpathDeflectableInterface::Execute_BeDeflected(OtherActor, OtherActorComponent, DeflectionImpulse, GetInstigatorController(), this);
			OnDeflected(DeflectingComponent, OtherActor, OtherActorComponent, DeflectionImpulse, HitResult);

			return true;
		}
	}
	
	return false;
}

void AEmpathDeflector::DeflectInitialOverlappingActors(UPrimitiveComponent* DeflectingComponent)
{
	if (DeflectingComponent)
	{
		// Get each overlapping component
		TArray<FHitResult> InitialOverlappingHits = SweepDeflectionCollision(DeflectingComponent);
		for (FHitResult& CurrHit : InitialOverlappingHits)
		{
			// Get the owning actor of the hit and check if we have already deflected it
			AActor* CurrOwner = CurrHit.GetActor();
			if (CurrOwner && CurrOwner != this && !InitialDeflectedActors.Contains(CurrOwner))
			{
				// Attempt to deflect the hit
				UPrimitiveComponent* CurrComp = CurrHit.GetComponent();
				if (CurrComp)
				{
					// If successful, add the component to the list of those initially overlapped
					if (AttemptDeflectActor(DeflectingComponent, CurrOwner, CurrComp, CurrHit))
					{
						InitialDeflectedActors.Emplace(CurrOwner);
					}
				}
			}
		}
	}

	// Queue clearing initial deflected actors
	if (InitialDeflectedActors.Num() > 0)
	{
		if (InitialDeflectedActorsClearTime > 0.0f)
		{
			if (!ClearInitialDeflectedActorsTimerHandle.IsValid())
			{
				GetWorldTimerManager().SetTimer(ClearInitialDeflectedActorsTimerHandle, this, &AEmpathDeflector::ClearInitialDeflectedActors, InitialDeflectedActorsClearTime, false);
			}
		}
		else
		{
			InitialDeflectedActors.Reset();
		}
	}

	return;
}

void AEmpathDeflector::ClearInitialDeflectedActors()
{
	InitialDeflectedActors.Empty();
}

// Called every frame
void AEmpathDeflector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector AEmpathDeflector::GetDeflectionImpulse_Implementation() const
{
	return GetActorForwardVector();
}

EEmpathTeam AEmpathDeflector::GetTeamNum_Implementation() const
{
	return Team;
}

bool AEmpathDeflector::IsDeflecting_Implementation(const UPrimitiveComponent* DeflectingComponent) const
{
	// By default, return whether deflect is active
	if (DeflectingComponent)
	{
		return bDeflectActive && DeflectingComponent->GetOwner() == this;
	}
	return false;
}

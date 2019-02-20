// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathTrigger.h"
#include "Components/BoxComponent.h"
#include "EmpathTypes.h"
#include "EmpathPlayerCharacter.h"

const FName AEmpathTrigger::TriggerVolumeName(TEXT("TriggerVolume0"));

AEmpathTrigger::AEmpathTrigger(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	:Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	bDestroyAfterTrigger = true;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TriggerVolumeName);
	TriggerVolume->InitBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->SetCollisionProfileName(FEmpathCollisionProfiles::EmpathTrigger);
	RootComponent = TriggerVolume;
}

void AEmpathTrigger::BeginPlay()
{
	Super::BeginPlay();
	if (TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AEmpathTrigger::OnTriggerVolumeOverlap);
	}
}

void AEmpathTrigger::OnTriggerVolumeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (ShouldTrigger(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult))
	{
		OnTriggered(OtherActor);
		OnTriggeredDelegate.Broadcast(this, OtherActor);

		if (bDestroyAfterTrigger)
		{
			SetLifeSpan(0.0001f);
			SetActorEnableCollision(false);
		}
	}

	return;
}

bool AEmpathTrigger::ShouldTrigger_Implementation(const UPrimitiveComponent* OverlappedComponent, const AActor* OtherActor, const UPrimitiveComponent* OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult & SweepResult) const
{
	const AEmpathPlayerCharacter* PlayerActor = Cast<AEmpathPlayerCharacter>(OtherActor);
	if (PlayerActor)
	{
		return true;
	}

	return false;
}
// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathTeleportMarker.h"


// Sets default values
AEmpathTeleportMarker::AEmpathTeleportMarker()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEmpathTeleportMarker::BeginPlay()
{
	Super::BeginPlay();
	if (!bMarkerVisible)
	{
		OnHideTeleportMarker();
	}
}

// Called every frame
void AEmpathTeleportMarker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEmpathTeleportMarker::ShowTeleportMarker(const EEmpathTeleportTraceResult CurrentTraceResult)
{
	if (!bMarkerVisible)
	{
		bMarkerVisible = true;
		OnShowTeleportMarker(CurrentTraceResult);
	}
	return;
}

void AEmpathTeleportMarker::HideTeleportMarker()
{
	if (bMarkerVisible)
	{
		bMarkerVisible = false;
		OnHideTeleportMarker();
	}
}

void AEmpathTeleportMarker::UpdateMarkerLocationAndYaw_Implementation(const FVector& TeleportLocation,
	const FVector& ImpactPoint,
	const FVector& ImpactNormal,
	const FRotator& TeleportYawRotation,
	const EEmpathTeleportTraceResult& TraceResult,
	AEmpathCharacter* TargetedTeleportCharacter,
	AEmpathTeleportBeacon* TargetedTeleportBeacon)
{
	// For now, just set our location to the teleport location
	SetActorLocationAndRotation(TeleportLocation, TeleportYawRotation);
	return;
}
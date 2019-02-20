// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "EmpathTypes.h"
#include "GameFramework/Actor.h"
#include "EmpathTeleportMarker.generated.h"

class AEmpathCharacter;
class AEmpathTeleportBeacon;
class AEmpathPlayerCharacter;

UCLASS()
class EMPATH_API AEmpathTeleportMarker : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEmpathTeleportMarker();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Called when tracing teleport to update the location of the teleport marker. */
	UFUNCTION(BlueprintNativeEvent, Category = EmpathTeleportMarker)
	void UpdateMarkerLocationAndYaw(const FVector& TeleportLocation, 
		const FVector& ImpactPoint, 
		const FVector& ImpactNormal, 
		const FRotator& TeleportYawRotation,
		const EEmpathTeleportTraceResult& TraceResult,
		AEmpathCharacter* TargetedTeleportCharacter, 
		AEmpathTeleportBeacon* TargetedTeleportBeacon);



	/** Called by the controlling Character when the marker is made visible. */
	void ShowTeleportMarker(const EEmpathTeleportTraceResult CurrentTraceResult);

	/** Called by the controlling Character when the marker is made visible. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathTeleportMarker)
	void OnShowTeleportMarker(const EEmpathTeleportTraceResult CurrentTraceResult);

	/** Called by the controlling Character when the marker is made invisible. */
	void HideTeleportMarker();

	/** Called by the controlling Character when the marker is made invisible. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathTeleportMarker)
	void OnHideTeleportMarker();

	/** Reference to the player character controlling this teleport marker. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathTeleportMarker)
	AEmpathPlayerCharacter * OwningCharacter;

	/** Called when the teleport trace type changes */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathTeleportMarker)
	void OnTeleportTraceResultChanged(const EEmpathTeleportTraceResult& NewTraceResult, const EEmpathTeleportTraceResult& OldTraceResult);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Whether the marker is currently shown. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathTeleportMarker, meta = (AllowPrivateAccess = true))
	bool bMarkerVisible;
};

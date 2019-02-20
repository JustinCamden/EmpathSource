// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmpathTrigger.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTriggeredDelegate, AEmpathTrigger*, Trigger, AActor*, TriggeringActor);

class UBoxComponent;

UCLASS()
class EMPATH_API AEmpathTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	static const FName TriggerVolumeName;

	// Sets default values for this actor's properties
	AEmpathTrigger(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Called when the trigger volume begins overlap with another actor. */
	void OnTriggerVolumeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	/*
	* Returns whether we should be triggered by an overlapping actor. 
	* By default, true if the other actor is an EmpathPlayerCharacter.
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = EmpathTrigger)
	bool ShouldTrigger(const UPrimitiveComponent* OverlappedComponent, const AActor* OtherActor, const UPrimitiveComponent* OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult & SweepResult) const;

	/** Called when our trigger is successfully triggered. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathTrigger)
	void OnTriggered(AActor* TriggeringActor);

	/** Called when our trigger is successfully triggered. */
	UPROPERTY(BlueprintAssignable, Category = EmpathTrigger)
	FOnTriggeredDelegate OnTriggeredDelegate;

	/** Whether the trigger should destroy itself after being triggered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathTrigger)
	bool bDestroyAfterTrigger;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:

	/** The trigger volume component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EmpathTrigger, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* TriggerVolume;


};

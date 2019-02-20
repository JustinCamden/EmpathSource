// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EmpathTimeDilationComponent.generated.h"

class AEmpathTimeDilator;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class EMPATH_API UEmpathTimeDilationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	// Sets default values for this component's properties
	UEmpathTimeDilationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	/*
	* Called to set the time dilation for this component.
	* Will automatically account for the EmpathTimeDilator.
	* Will only take effect if the component is active.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathTimeDilationComponent)
	void SetActorTimeDilation(const float NewTimeDilation = 1.0f);

	/*
	* Gets the cached reference to the Empath Time Dilator.
	* If null, attempts to find the time dilator.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathTimeDilationComponent)
	AEmpathTimeDilator* GetTimeDilator();

	/** Called whenever time dilation is changed by the Empath Time Dilator. */
	UFUNCTION()
	void OnGlobalTimeDilationChanged(float NewTimeDilation);

	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/*
	* The desired time dilation value for this actor.
	* Will constantly set time dilation on this actor to this value, accounting for the EmpathTimeDilator.
	* Will only take effect if the component is active.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = EmpathTimeDilationComponent, meta = (AllowPrivateAccess = true, ClampMin = "0.0001", ClampMax = "20.0", UIMin = "0.0001", UIMax = "20.0"))
	float ActorTimeDilation;

	/** Reference to the EmpathTimeDilator. */
	AEmpathTimeDilator* TimeDilator;
};

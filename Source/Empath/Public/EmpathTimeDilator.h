// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmpathTypes.h"
#include "EmpathTimeDilator.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnTimeDilationEndCallback, int32, RequestID, bool, bAborted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeDilationChangedDelegate, float, NewTimeDilation);

UCLASS()
class EMPATH_API AEmpathTimeDilator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEmpathTimeDilator(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Override so we can reset time dilation
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/*
	* The speed by which we interpolate towards the target time scale. 
	* This is measured by how much the timescale changes in one second, 
	* where 1 = normal time, 0 = stopped time, and 0.5 = half time.
	* A speed of 2 would scale from 0 to 1 in 0.5 seconds.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathTimeDilator)
	float TimeDilationSpeed;

	/*
	* Sends a request to dilate time. 
	* The request will persist for the inputted duration, 
	* and time will be dilated down to the lowest active request. 
	* Returns the ID of the request that can be canceled later.
	* If an ID is provided, it will instead change the remaining time or scale on the request with this ID.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathTimeDilator, meta = (AutoCreateRefTerm = "OnTimeDilationEndCallback"))
	int32 RequestTimeDilation(const float Duration, const float TimeScale, const FOnTimeDilationEndCallback& OnTimeDilationEndCallback, int32 RequestID = 0);

	/*
	* Cancels an active time dilation request by ID. 
	* Returns whether a time dilation request was found and canceled.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathTimeDilator)
	bool CancelTimeDilationRequest(int32 RequestId);

	/** Clears all currently active time dilation requests. */
	UFUNCTION(BlueprintCallable, Category = EmpathTimeDilator)
	void ClearTimeDilationRequests();

	/** Gets the delta time unscaled by time dilation. */
	UFUNCTION(BlueprintCallable, Category = EmpathTimeDilator)
	const float GetUndilatedDeltaTime() const { return UndilatedDeltaTime; }

	/** Updates the undilated delta time. */
	void UpdateUndilatedDeltaTime();

	/** Updates time dilation and time dilation requests. */
	void UpdateTimeDilation();

	/** Gets whether time is currently dilated. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathTimeDilator)
	const bool IsTimeDilated() const { return bTimeDilated; }

	/** Gets whether the time dilator is currently active. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathTimeDilator)
	const bool IsActive() const { return bTimeDilationActive; }

	/** Enabled the time dilator. */
	UFUNCTION(BlueprintCallable, Category = EmpathTimeDilator)
	void EnableTimeDilator();

	/** Disables time dilator. */
	UFUNCTION(BlueprintCallable, Category = EmpathTimeDilator)
	void DisableTimeDilator();

	/** Called when time dilation starts. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathTimeDilator)
	void OnTimeDilationStart();

	/** Called when time dilation end. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathTimeDilator)
	void OnTimeDilationEnd();

	/** Called whenever time dilation is changed. */
	FOnTimeDilationChangedDelegate OnTimeDilationChanged;
private:
	/* Whether the time dilator is currently active. */
	bool bTimeDilationActive;

	/** The currently active time dilation requests. */
	TArray<FEmpathTimeDilationRequest> TimeDilationRequests;

	/** Delta time unscaled by time dilation. */
	float UndilatedDeltaTime;

	/** The timestamp of the last frame. */
	float LastFrameTimeStamp;

	/** Whether time is currently dilated. */
	bool bTimeDilated;
};

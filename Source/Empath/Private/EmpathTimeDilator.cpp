// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathTimeDilator.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Engine/Public/EngineUtils.h"


// Sets default values
AEmpathTimeDilator::AEmpathTimeDilator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
 	// Enable tick on this actor
	PrimaryActorTick.bCanEverTick = true;

	// Set default timescale lerp speed
	TimeDilationSpeed = 3.0f;
	bTimeDilationActive = true;
}

// Called when the game starts or when spawned
void AEmpathTimeDilator::BeginPlay()
{
	Super::BeginPlay();
	
}

void AEmpathTimeDilator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bTimeDilated)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);

		for (FEmpathTimeDilationRequest CurrRequest : TimeDilationRequests)
		{
			CurrRequest.OnEndDelegate.Broadcast(CurrRequest.RequestID, true);
		}
	}
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AEmpathTimeDilator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateUndilatedDeltaTime();
	UpdateTimeDilation();
}

int32 AEmpathTimeDilator::RequestTimeDilation(const float Duration, const float TimeScale, const FOnTimeDilationEndCallback& OnTimeDilationEndCallback, int32 RequestID)
{

	// Modify existing entry if appropriate
	if (RequestID > 0)
	{
		// Search for the request with this id
		for (int32 Idx = 0; Idx < TimeDilationRequests.Num(); Idx++)
		{
			if (TimeDilationRequests[Idx].RequestID == RequestID)
			{
				TimeDilationRequests[Idx].TimeScale = TimeScale;
				TimeDilationRequests[Idx].RemainingTime = Duration;
				return RequestID;
			}
		}
	}

	// Otherwise, initialize new request struct
	int32 NewIdx = TimeDilationRequests.Add(FEmpathTimeDilationRequest());

	TimeDilationRequests[NewIdx].RemainingTime = Duration;
	TimeDilationRequests[NewIdx].TimeScale = FMath::Clamp(TimeScale, 0.000100f, 20.000000f);

	// Get the largest number for the new quest id
	for (FEmpathTimeDilationRequest CurrRequest : TimeDilationRequests)
	{
		if (CurrRequest.RequestID >= TimeDilationRequests[NewIdx].RequestID)
		{
			TimeDilationRequests[NewIdx].RequestID = CurrRequest.RequestID + 1;
		}
	}

	// Add the callback
	TimeDilationRequests[NewIdx].OnEndDelegate.Add(OnTimeDilationEndCallback);

	return 	TimeDilationRequests[NewIdx].RequestID;
}

bool AEmpathTimeDilator::CancelTimeDilationRequest(int32 RequestID)
{
	// Search for and remove an entry with the inputted ID
	for (int32 CurrIdx = 0; CurrIdx < TimeDilationRequests.Num(); CurrIdx ++)
	{
		if (TimeDilationRequests[CurrIdx].RequestID == RequestID)
		{
			TimeDilationRequests[CurrIdx].OnEndDelegate.Broadcast(TimeDilationRequests[CurrIdx].RequestID, true);
			TimeDilationRequests.RemoveAtSwap(CurrIdx);
			return true;
		}
	}

	return false;
}

void AEmpathTimeDilator::ClearTimeDilationRequests()
{
	for (FEmpathTimeDilationRequest CurrRequest : TimeDilationRequests)
	{
		CurrRequest.OnEndDelegate.Broadcast(CurrRequest.RequestID, true);
	}
	TimeDilationRequests.Empty();
}

void AEmpathTimeDilator::UpdateUndilatedDeltaTime()
{
	UndilatedDeltaTime = GetWorld()->GetRealTimeSeconds() - LastFrameTimeStamp;
	LastFrameTimeStamp = GetWorld()->GetRealTimeSeconds();
	return;
}

void AEmpathTimeDilator::UpdateTimeDilation()
{
	if (bTimeDilationActive)
	{
		// Initialize variables
		float TargetTimeScale = 1.0f;

		// Get the current time dilation
		float CurrTimeDilation = GetWorld()->GetWorldSettings()->TimeDilation;

		// Check if any requests remain active
		if (TimeDilationRequests.Num() > 0)
		{
			// Iterate through the requests
			float LowestTimeScale = 1.0f;
			int32 CurrIdx = 0;
			while (CurrIdx < TimeDilationRequests.Num())
			{
				// Decrement the remaining time 
				if (TimeDilationRequests[CurrIdx].RemainingTime > 0.0f)
				{
					TimeDilationRequests[CurrIdx].RemainingTime -= UndilatedDeltaTime;

					// If there is still time left
					if (TimeDilationRequests[CurrIdx].RemainingTime > 0.0f)
					{
						// Check if the remaining time is lower than the current lowest time
						if (TimeDilationRequests[CurrIdx].TimeScale < LowestTimeScale)
						{
							// If so, update the lowest timescale
							LowestTimeScale = TimeDilationRequests[CurrIdx].TimeScale;
						}

						// Increment the index so we can check the next element
						CurrIdx++;
					}

					// If there is not time left, remove this index
					else
					{
						TimeDilationRequests[CurrIdx].OnEndDelegate.Broadcast(TimeDilationRequests[CurrIdx].RequestID, false);
						TimeDilationRequests.RemoveAtSwap(CurrIdx);

						// Do not increment the index so that we check the index which was swapped inside
					}
				}

				// If the time dilation timer has an initial lifetime of 0 or less, it is infinite
				else
				{
					// Check if the remaining time is lower than the current lowest time
					if (TimeDilationRequests[CurrIdx].TimeScale < LowestTimeScale)
					{
						// If so, update the lowest timescale
						LowestTimeScale = TimeDilationRequests[CurrIdx].TimeScale;
					}

					// Increment the index so we can check the next element
					CurrIdx++;
				}

			}

			// If there are any remaining valid requests, our target scale is the lowest scale
			if (TimeDilationRequests.Num() > 0)
			{
				TargetTimeScale = LowestTimeScale;
			}

			// Otherwise, our target time scale should be 1
		}

		// If our target timescale is not our current time scale
		if (TargetTimeScale != CurrTimeDilation)
		{
			// Get the new time scale
			float NewTimeScale = FMath::FInterpTo(CurrTimeDilation, TargetTimeScale, UndilatedDeltaTime, TimeDilationSpeed);

			// Update depending on whether we are going up or down
			if ((TargetTimeScale < CurrTimeDilation && NewTimeScale < TargetTimeScale)
				|| (TargetTimeScale > CurrTimeDilation && NewTimeScale > TargetTimeScale))
			{
				NewTimeScale = TargetTimeScale;
			}

			// Update the time dilation
			UGameplayStatics::SetGlobalTimeDilation(this, NewTimeScale);

			// Fire events if appropriate
			OnTimeDilationChanged.Broadcast(NewTimeScale);
			if (NewTimeScale != 1.0f && !bTimeDilated)
			{
				bTimeDilated = true;
				OnTimeDilationStart();
			}
			else if (NewTimeScale == 1.0f && bTimeDilated)
			{
				bTimeDilated = false;
				OnTimeDilationEnd();
			}
		}
	}
	
	return;
}

void AEmpathTimeDilator::EnableTimeDilator()
{
	bTimeDilationActive = true;
}

void AEmpathTimeDilator::DisableTimeDilator()
{
	if (bTimeDilated)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
		bTimeDilated = false;
		OnTimeDilationChanged.Broadcast(1.0f);
		OnTimeDilationEnd();
	}
	ClearTimeDilationRequests();
	bTimeDilationActive = false;
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EmpathGameModeBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPulseDelegate, float, PulseDeltaTime);

class AEmpathAIManager;
class AEmpathBishopManager;
class AEmpathTimeDilator;
class AEmpathSoundManager;

/**
 * 
 */
UCLASS()
class EMPATH_API AEmpathGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	AEmpathGameModeBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Gets the world AI Manager */
	UFUNCTION(Category = EmpathGameModeBase, BlueprintCallable, BlueprintPure)
	AEmpathAIManager* GetAIManager() const { return AIManager; }

	/** Gets the world AI Manager */
	UFUNCTION(Category = EmpathGameModeBase, BlueprintCallable, BlueprintPure)
	AEmpathTimeDilator* GetTimeDilator() const { return TimeDilator; }

	/** Gets the world Bishop Manager */
	UFUNCTION(Category = EmpathGameModeBase, BlueprintCallable, BlueprintPure)
	AEmpathBishopManager* GetBishopManager() const { return BishopManager; }

	/** Gets the world Sound Manager. */
	UFUNCTION(Category = EmpathGameModeBase, BlueprintCallable, BlueprintPure)
	AEmpathSoundManager* GetSoundManager() const { return SoundManager; }

	/** Called at the end of each pulse. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathGameModeBase, meta = (DisplayName = "Pulse"))
	void ReceivePulse(const float PulseDeltaTime);

	/** Called at the end of each pulse. */
	UPROPERTY(BlueprintAssignable, Category = EmpathGameModeBase)
	FOnPulseDelegate OnPulse;

	virtual void BeginPlay() override;
private:

	/** The AI Manager class to spawn. */
	UPROPERTY(Category = EmpathGameModeBase, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEmpathAIManager> AIManagerClass;

	/** Reference to our AI Manager. */
	AEmpathAIManager* AIManager;

	/** The bishop Manager class to spawn. */
	UPROPERTY(Category = EmpathGameModeBase, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEmpathBishopManager> BishopManagerClass;

	/** Reference to our bishop Manager. */
	AEmpathBishopManager* BishopManager;

	/** The time dilator class to spawn. */
	UPROPERTY(Category = EmpathGameModeBase, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEmpathTimeDilator> TimeDilatorClass;

	/** Reference to our Time Dilator. */
	AEmpathTimeDilator* TimeDilator;

	/** The sound manager class to spawn. */
	UPROPERTY(Category = EmpathGameModeBase, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AEmpathSoundManager> SoundManagerClass;

	/** Reference to our Sound Manager. */
	AEmpathSoundManager* SoundManager;

	/** The time between pulses. Will be ignored if below 0.01. */
	UPROPERTY(Category = EmpathGameModeBase, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float TimeBetweenPulses;

	/** The timer delegate for calling pulses. */
	FTimerHandle PulseTimerHandle;

	/** Called at the end of each pulse. */
	void Pulse();

	/** The time of the last pulse. */
	float LastPulseTimeStamp;
};

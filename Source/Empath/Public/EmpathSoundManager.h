// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmpathTypes.h"
#include "EmpathSoundManager.generated.h"

class AEmpathAIManager;

UCLASS()
class EMPATH_API AEmpathSoundManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEmpathSoundManager();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Registers the sound manager with the AI manager. */
	void RegisterAIManager(AEmpathAIManager* AIManagerToRegister);

	/** Called when the player awareness state changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathSoundManager)
	void OnNewPlayerAwarenessState(const EEmpathPlayerAwarenessState NewAwarenessState, const EEmpathPlayerAwarenessState OldAwarenessState);

	/** Called when the AI manager is registered. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathSoundManager)
	void OnAIManagerRegistered();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:	
	/** Reference to the AI manager */
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = EmpathSoundManager)
	AEmpathAIManager* AIManager;


};

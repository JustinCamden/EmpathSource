// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EmpathTypes.h"
#include "EmpathSpeakerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueFinishedDelegate, bool, bSuccess, UEmpathSpeakerComponent*, Speaker);


UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class EMPATH_API UEmpathSpeakerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEmpathSpeakerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Plays an individual dialogue block on the inputted speaker. */
	UFUNCTION(BlueprintCallable, Category = EmpathSpeakerComponent)
	const bool PlayDialogue(const FString& DialogueSpeakerName, const FEmpathDialogueBlock& DialogueBlock);

	/** Implementation for playing an individual dialogue block on the inputted speaker. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathSpeakerComponent, meta = (DisplayName = "Play Dialogue" ))
	bool ReceivePlayDialogue(const FString& DialogueSpeakerName, const FEmpathDialogueBlock& DialogueBlock);

	/** Call to signal the speaker and any listeners that the current dialogue block is finished. */
	UFUNCTION(BlueprintCallable, Category = EmpathSpeakerComponent)
	void FinishedDialogue(bool bSuccess);

	/** Called when the speaker finishes a dialogue block. */
	void OnDialogueFinished(bool bSuccess);

	/** Called when the speaker finishes a dialogue block. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathSpeakerComponent, meta = (DisplayName = "On Dialogue Finished"))
	void ReceiveDialogueFinished(bool bSuccess);

	/** Called when the speaker finishes a dialogue block. */
	UPROPERTY(BlueprintAssignable, Category = EmpathSpeakerComponent)
	FOnDialogueFinishedDelegate OnDialogueFinishedDelegate;

	/** Called to abort dialogue. */
	UFUNCTION(BlueprintCallable, Category = EmpathSpeakerComponent)
	void AbortDialogue();

	/** Called to abort dialogue. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathSpeakerComponent, meta = (DisplayName = "Abort Dialogue"))
	void ReceiveAbortDialogue();

	/** Returns whether dialogue is currently active. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathSpeakerComponent)
	const bool IsDialogueActive() { return bDialogueActive; }

private:

	/** Whether dialogue is currently active. */
	bool bDialogueActive;
};

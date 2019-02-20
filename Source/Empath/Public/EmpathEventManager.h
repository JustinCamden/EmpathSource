// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmpathTypes.h"
#include "EmpathEventManager.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnConversationFinishedCallback, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConversationFinishedDelegate, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConvoFinishedCallback, bool, bSuccess);


class UEmpathSpeakerComponent;

UCLASS()
class EMPATH_API AEmpathEventManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEmpathEventManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/*
	* Plays sequence of conversation blocks. 
	* @Param Conversation	Array of conversation blocks to play. Each conversation block will be fed to a speaker in the Speakers a array index matching its SpeakerIDX value.
	* @param Speakers	Array of Speaker components to play from. Each conversation block will be fed to a speaker with an array index matching its SpeakerIDX value.
	*/
	UFUNCTION(BlueprintCallable, Category = EmpathEventManager, meta = (AutoCreateRefTerm = "OnConversationFinishedCallback"))
	const bool PlayConversation(const TArray<FEmpathConversationBlock>& Conversation, const TArray<FEmpathConversationSpeaker> Speakers, const FOnConversationFinishedCallback& OnConversationFinishedCallback);

	/** Called when a conversation block finishes. */
	UFUNCTION()
	void OnConversationBlockFinished(bool bSuccess, UEmpathSpeakerComponent* Speaker);

	/** Called when a conversation block finishes. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathEventManager, meta = (DisplayName = "On Conversation Block Finished "))
	void ReceiveConversationBlockFinished(bool bSuccess, UEmpathSpeakerComponent* Speaker);

	/** Called when a conversation sequence finishes. */
	void OnConversationFinished(bool bSuccess);

	/** Called when a conversation finishes. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathEventManager, meta = (DisplayName = "On Conversation Finished "))
	void ReceiveConversationFinished(bool bSuccess);

	/** Called when a conversation finishes. */
	UPROPERTY(BlueprintAssignable, Category = EmpathEventManager)
	FOnConversationFinishedDelegate OnConversationFinishedDelegate;

	/** Called to abort a conversation sequence. */
	UFUNCTION(BlueprintCallable, Category = EmpathEventManager)
	void AbortConversation();

	/** Returns whether a conversation is currently active. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathSpeakerComponent)
	const bool IsConversationActive() { return bConversationActive; }

private:
	/** The speakers in the last inputted conversation. */
	TArray<FEmpathConversationSpeaker> CurrSpeakers;

	/** The conversation blocks in the last inputted conversation. */
	TArray<FEmpathConversationBlock> CurrConversation;

	/** The index of the last played conversation block. */
	int32 CurrConversationIDX;

	/** The current callback for the conversation. */
	FOnConvoFinishedCallback CurrConvoCallback;

	/** Whether a conversation is currently active. */
	bool bConversationActive;


	/** Handle to manage dialogue interrupt timer */
	FTimerHandle InterruptTimerHandle;
};

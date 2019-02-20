// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathEventManager.h"
#include "EmpathSpeakerComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"



// Sets default values
AEmpathEventManager::AEmpathEventManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEmpathEventManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEmpathEventManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


const bool AEmpathEventManager::PlayConversation(const TArray<FEmpathConversationBlock>& Conversation, const TArray<FEmpathConversationSpeaker> Speakers, const FOnConversationFinishedCallback& OnConversationFinishedCallback)
{
	// Abort any ongoing conversations 
	AbortConversation();

	// Protect ourselves from invalid input
	if (Conversation.Num() > 0 && Speakers.Num() > 0)
	{
		if (Conversation[0].SpeakerIDX > -1 && Conversation[0].SpeakerIDX < Speakers.Num())
		{
			if (Speakers[Conversation[0].SpeakerIDX].SpeakerRef)
			{
				// Play the dialogue block and bind callback
				if (Speakers[Conversation[0].SpeakerIDX].SpeakerRef->PlayDialogue(Speakers[Conversation[0].SpeakerIDX].SpeakerName, Conversation[0].DialogueBlock))
				{
					// If successful, cache the arrays and the index
					bConversationActive = true;
					CurrSpeakers = Speakers;
					CurrConversation = Conversation;
					CurrConversationIDX = 0;

					// Bind the delegate
					Speakers[Conversation[0].SpeakerIDX].SpeakerRef->OnDialogueFinishedDelegate.AddDynamic(this, &AEmpathEventManager::OnConversationBlockFinished);
					CurrConvoCallback.Add(OnConversationFinishedCallback);
					return true;
				}
			}
		}
	}

	return false;
}

void AEmpathEventManager::OnConversationBlockFinished(bool bSuccess, UEmpathSpeakerComponent* Speaker)
{
	if (bConversationActive)
	{
		ReceiveConversationBlockFinished(bSuccess, Speaker);

		// Go to next dialogue block if appropriate
		if (bSuccess && CurrConversationIDX < CurrConversation.Num() - 1)
		{
			CurrConversationIDX++;

			// Ensure input is in range
			if (CurrConversation[CurrConversationIDX].SpeakerIDX > -1 && CurrConversation[CurrConversationIDX].SpeakerIDX < CurrSpeakers.Num())
			{
				if (CurrSpeakers[CurrConversation[CurrConversationIDX].SpeakerIDX].SpeakerRef)
				{
					if (CurrSpeakers[CurrConversation[CurrConversationIDX].SpeakerIDX].SpeakerRef->PlayDialogue(CurrSpeakers[CurrConversation[CurrConversationIDX].SpeakerIDX].SpeakerName, CurrConversation[CurrConversationIDX].DialogueBlock))
					{
						// Rebind event if appropriate
						if (Speaker && Speaker != CurrSpeakers[CurrConversation[CurrConversationIDX].SpeakerIDX].SpeakerRef)
						{
							FTimerDelegate TimerDel;
							// If the line should be interrupted, add a timer instead
							if (CurrConversation[CurrConversationIDX].DialogueBlock.InterruptedAfterTime > 0.0f) {
								TimerDel.BindUFunction(this, FName("OnConversationBlockFinished"), bSuccess, Speaker);
								GetWorld()->GetTimerManager().SetTimer(InterruptTimerHandle, TimerDel, CurrConversation[CurrConversationIDX].DialogueBlock.InterruptedAfterTime, false);
							}
							else
							{
								Speaker->OnDialogueFinishedDelegate.RemoveDynamic(this, &AEmpathEventManager::OnConversationBlockFinished);
								CurrSpeakers[CurrConversation[CurrConversationIDX].SpeakerIDX].SpeakerRef->OnDialogueFinishedDelegate.AddDynamic(this, &AEmpathEventManager::OnConversationBlockFinished);
							}
						}
						// Otherwise leave it bound if the speaker was the same
						return;
					}
				}
			}
			OnConversationFinished(false);
		}

		// Otherwise, we have finished the conversation
		else
		{
			// Signal conversation over
			OnConversationFinished(bSuccess);
		}
	}

	// Unbind delegate
	if (Speaker)
	{
		Speaker->OnDialogueFinishedDelegate.RemoveDynamic(this, &AEmpathEventManager::OnConversationBlockFinished);
	}
	return;
}

void AEmpathEventManager::OnConversationFinished(bool bSuccess)
{
	bConversationActive = false;
	ReceiveConversationFinished(bSuccess);
	OnConversationFinishedDelegate.Broadcast(bSuccess);
	CurrConvoCallback.Broadcast(bSuccess);
	CurrConvoCallback.Clear();
}

void AEmpathEventManager::AbortConversation()
{
	if (bConversationActive)
	{
		// Ensure input is in range
		if (CurrConversation[CurrConversationIDX].SpeakerIDX > -1 && CurrConversation[CurrConversationIDX].SpeakerIDX < CurrSpeakers.Num())
		{
			if (CurrSpeakers[CurrConversation[CurrConversationIDX].SpeakerIDX].SpeakerRef)
			{

				CurrSpeakers[CurrConversation[CurrConversationIDX].SpeakerIDX].SpeakerRef->AbortDialogue();
			}
		}
		else
		{
			OnConversationBlockFinished(false, nullptr);
		}
	}
	return;
}


// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathSpeakerComponent.h"


// Sets default values for this component's properties
UEmpathSpeakerComponent::UEmpathSpeakerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UEmpathSpeakerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UEmpathSpeakerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UEmpathSpeakerComponent::FinishedDialogue(bool bSuccess)
{
	if (bDialogueActive)
	{
		OnDialogueFinished(bSuccess);
		OnDialogueFinishedDelegate.Broadcast(bSuccess, this);
		return;
	}
}

void UEmpathSpeakerComponent::OnDialogueFinished(bool bSuccess)
{
	bDialogueActive = false;
	ReceiveDialogueFinished(bSuccess);
	return;
}

void UEmpathSpeakerComponent::AbortDialogue()
{
	if (bDialogueActive)
	{
		ReceiveAbortDialogue();
		FinishedDialogue(false);
	}
	return;
}

const bool UEmpathSpeakerComponent::PlayDialogue(const FString& DialogueSpeakerName, const FEmpathDialogueBlock& DialogueBlock)
{
	AbortDialogue();
	bool bSuccess = ReceivePlayDialogue(DialogueSpeakerName, DialogueBlock);
	if (bSuccess)
	{
		bDialogueActive = true;
	}
	return bSuccess;
}

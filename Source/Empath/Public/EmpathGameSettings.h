// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EmpathGameSettings.generated.h"

/**
 * 
 */
UCLASS()
class EMPATH_API UEmpathGameSettings : public USaveGame
{
	GENERATED_BODY()
public:
	/** The world meters scale after calibrating the player height. */
	UPROPERTY(BlueprintReadWrite, Category = EmpathGameSettings)
	float CalibratedWorldMetersScale;
	
	UEmpathGameSettings();
};

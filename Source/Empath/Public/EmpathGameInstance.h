// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "EmpathGameInstance.generated.h"

class UEmpathGameSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerProgressChangeDelegate, int32, NewPlayerProgressIdx);

/**
 * 
 */
UCLASS()
class EMPATH_API UEmpathGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** The currently loaded game settings. */
	UPROPERTY(BlueprintReadWrite, Category = EmpathGameInstance)
	UEmpathGameSettings* LoadedSettings;

	/** The currently loaded game settings. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathGameInstance)
	FString SettingsSlot;

	/** The current player progress. */
	UPROPERTY(BlueprintReadOnly, Category = EmpathGameInstance)
	int32 PlayerProgressIdx;

	/** Set the current player progress. */
	UFUNCTION(BlueprintCallable, Category = EmpathGameInstance)
	bool SetPlayerProgressIdx(int32 NewPlayerProgressIdx);

	/** Called on player progress change. */
	UPROPERTY(BlueprintAssignable, Category = "EmpathPlayerCharacter|Combat")
	FOnPlayerProgressChangeDelegate OnPlayerProgressChange;

	/** Set the current player progress and reload the level. */
	UFUNCTION(Exec)
	void EmpathJumpToProgress(int32 NewPlayerProgressIdx);

	/** Set the current player progress and reload the level. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = EmpathGameInstance)
	void JumpToProgress(int32 NewPlayerProgressIdx);

	/** Called to load the game settings. Returns whether load succeed*/
	UFUNCTION(BlueprintCallable, Category = EmpathGameInstance)
	void SavePowerMetrics();

	/** Map to hold values for powers. */
	UPROPERTY(BlueprintReadWrite, Category = EmpathGameInstance)
	TMap<FString, FString> PowerMetrics;

	/** Called to load the game settings. Returns whether load succeed*/
	UFUNCTION(BlueprintCallable, Category = EmpathGameInstance)
	bool LoadSettings();

	/** Called to save the game settings. Returns whether save succeeds. */
	UFUNCTION(BlueprintCallable, Category = EmpathGameInstance)
	bool SaveSettings();

	/** Called whenever settings are successfully loaded. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathGameInstance, meta = (DisplayName = "On Settings Loaded"))
	void ReceiveSettingsLoaded();

	/** Called whenever settings are successfully saved. */
	UFUNCTION(BlueprintImplementableEvent, Category = EmpathGameInstance, meta = (DisplayName = "On Settings Saved"))
	void ReceiveSettingsSaved();

	/** Whether we should save power metrics on Shutdown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathGameInstance)
	bool bShouldSavePowerMetrics;

	/** The min world meters scale allowed when calibrating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathGameInstance)
	float MinWorldMetersScale;

	/** The max world meters scale allowed when calibrating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathGameInstance)
	float MaxWorldMetersScale;

	/** How high we want the player to be when we calibrate height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EmpathGameInstance)
	float PlayerBaseHeight;

	virtual void Init() override;
	virtual void Shutdown() override;

	UEmpathGameInstance();
};

// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathGameInstance.h"
#include "EmpathGameSettings.h"
#include "Classes/Kismet/GameplayStatics.h"
#include "EmpathFunctionLibrary.h"
#include "EmpathPlayerCharacter.h"


//FAutoConsoleCommand CSetPlayerIdx(
//	TEXT("Empath.PlayerProgressIdx"),
//	TEXT("Sets hero stamina percentage\n"),
//	FConsoleCommandWithArgsDelegate::CreateStatic(&SetPlayerProgressIdx),
//	ECVF_Cheat);

UEmpathGameInstance::UEmpathGameInstance()
{
	SettingsSlot = "Settings";
	MinWorldMetersScale = 50.0f;
	MaxWorldMetersScale = 150.0f;
	PlayerBaseHeight = 170.0f;
}

bool UEmpathGameInstance::LoadSettings()
{
	USaveGame* CurrSave = UGameplayStatics::LoadGameFromSlot(SettingsSlot, 0);
	LoadedSettings = Cast<UEmpathGameSettings>(CurrSave);
	if (LoadedSettings)
	{
		ReceiveSettingsLoaded();
		return true;
	}
	else
	{
		LoadedSettings = Cast<UEmpathGameSettings>(UGameplayStatics::CreateSaveGameObject(UEmpathGameSettings::StaticClass()));
		if (LoadedSettings)
		{
			ReceiveSettingsLoaded();
			return true;
		}
	}
	UE_LOG(LogTemp, Error, TEXT("Empath Settings failed to load!"));
	return false;
}

bool UEmpathGameInstance::SaveSettings()
{
	if (UGameplayStatics::SaveGameToSlot(LoadedSettings, SettingsSlot, 0))
	{
		ReceiveSettingsSaved();
		return true;
	}
	UE_LOG(LogTemp, Error, TEXT("Empath Settings failed to save!"));
	return false;
}

void UEmpathGameInstance::SavePowerMetrics() {

	TArray<FString> PowerNames; 
	TArray<FString> NumberStatistics;
	PowerMetrics.GenerateKeyArray(PowerNames);
	PowerMetrics.GenerateValueArray(NumberStatistics);

	UEmpathFunctionLibrary::SaveStringArrayToCSV("","PowerMetrics", PowerNames, true, true); 
	UEmpathFunctionLibrary::SaveStringArrayToCSV("", "PowerMetrics", NumberStatistics, true, true);

}

void UEmpathGameInstance::Init()
{
	LoadSettings();
	Super::Init();
}

void UEmpathGameInstance::Shutdown()
{
	if (bShouldSavePowerMetrics)
	{
		SavePowerMetrics();
	}

	SaveSettings();
	Super::Shutdown();
}

bool UEmpathGameInstance::SetPlayerProgressIdx(int32 NewPlayerProgressIdx)
{
	if (PlayerProgressIdx != NewPlayerProgressIdx)
	{
		PlayerProgressIdx = NewPlayerProgressIdx;
		OnPlayerProgressChange.Broadcast(PlayerProgressIdx);
		return true;
	}
	return false;
}

void UEmpathGameInstance::EmpathJumpToProgress(int32 NewPlayerProgressIdx)
{
	JumpToProgress(NewPlayerProgressIdx);
}
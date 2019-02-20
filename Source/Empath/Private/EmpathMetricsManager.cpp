// Copyright 2018 Team Empath All Rights Reserved

#include "EmpathMetricsManager.h"
#include "EmpathFunctionLibrary.h"
#include "FileManager.h"

// Sets default values
AEmpathMetricsManager::AEmpathMetricsManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEmpathMetricsManager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEmpathMetricsManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEmpathMetricsManager::BeginRecordingMetrics_Implementation(const FString& SaveDirectory, const FString& filename, bool truncate) {
	if (bRecording) {
		recordingStartTime = FDateTime::Now();
	}
	else {
		bRecording = true;
		saveDirectory = SaveDirectory;
		outFile = filename;
		bTruncating = truncate;
	}
}

void AEmpathMetricsManager::StopRecordingMetrics_Implementation() {
	if (bRecording) {
		bRecording = false;
		recordingEndTime = FDateTime::Now();
		FlushData();
	}
}

void AEmpathMetricsManager::FlushData_Implementation() {
	TArray<FString> s{
		"Metrics Start Time," + recordingStartTime.ToString(),
		"Metrics End Time," + recordingEndTime.ToString(),
	};
	WriteMetrics(s);
}

void AEmpathMetricsManager::WriteMetrics(TArray<FString>& Lines) {
	UEmpathFunctionLibrary::SaveStringArrayToCSV(saveDirectory, outFile, Lines, bTruncating, true);
}

FString AEmpathMetricsManager::DateTimeToString(FDateTime dateTime) {
	return dateTime.ToString();
}
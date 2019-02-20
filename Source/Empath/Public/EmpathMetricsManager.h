// Copyright 2018 Team Empath All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmpathMetricsManager.generated.h"

UCLASS()
class EMPATH_API AEmpathMetricsManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEmpathMetricsManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Called to begin recording metrics */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathMetricsManager)
	void BeginRecordingMetrics(const FString& SaveDirectory, const FString& filename, bool truncate);

	/** Called to stop recording metrics */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathMetricsManager)
	void StopRecordingMetrics();

	/** Called to write to output file */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = EmpathMetricsManager)
	void FlushData();

	/** Called to write to output file */
	UFUNCTION(BlueprintCallable, Category = EmpathMetricsManager)
	void WriteMetrics(UPARAM(ref) TArray<FString>& Lines);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = EmpathMetricsManager)
	FString DateTimeToString(FDateTime dateTime);

private:
	FString saveDirectory;
	FString outFile;
	FDateTime recordingStartTime;
	FDateTime recordingEndTime;
	bool bRecording;
	bool bTruncating;
};

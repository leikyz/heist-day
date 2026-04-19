// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACameraConsole.generated.h"

UCLASS()
class FINALGAME_API AACameraConsole : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AACameraConsole();

	UFUNCTION(BlueprintCallable)
	void OnInteract(bool action);

	UFUNCTION(BlueprintCallable)
	void NavigateCamera(int stepDir);

	UFUNCTION(CallInEditor, Category = "Camera management")
	void SpawnCamera();

	UFUNCTION(CallInEditor, Category = "Camera management")
	void ClearCamera();

	
	APawn* PlayerPawn;
	APlayerController* PlayerController;

	int IndexCamera = 0;
	bool ConsoleInteract = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Console param")
	float DistInteract = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Console param")
	TSubclassOf<APawn> CameraToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Console param")
	TArray<APawn*> CameraList;


};

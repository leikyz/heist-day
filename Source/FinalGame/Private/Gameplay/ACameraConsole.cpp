// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/ACameraConsole.h"

// Sets default values
AACameraConsole::AACameraConsole()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AACameraConsole::BeginPlay()
{
	Super::BeginPlay();
}

APawn* AACameraConsole::OnInteract(bool action, APawn* pawn)
{
	if (!ConsoleInteract && action)
	{
		PlayerPawn = pawn;

		if (CameraList.Num() > IndexCamera)
		{
			ConsoleInteract = true;
			return CameraList[IndexCamera];
		}
	}
	else if (ConsoleInteract && !action)
	{
		APawn* ref = PlayerPawn;

		PlayerPawn = nullptr;

		ConsoleInteract = false;
		return ref;
	}

	return nullptr;
}

APawn* AACameraConsole::NavigateCamera(int stepDir)
{
	if (ConsoleInteract)
	{
		if (CameraList.Num() > 0)
		{
			IndexCamera += stepDir;

			IndexCamera = IndexCamera < 0 ? CameraList.Num() - 1 : IndexCamera;
			IndexCamera %= CameraList.Num();

			return CameraList[IndexCamera];
		}

		UE_LOG(LogTemp, Warning, TEXT("No camera in camera list"));
	}
	
	return nullptr;
}

void AACameraConsole::ClearCamera()
{
	for (int i = 0; i < CameraList.Num(); i++)
	{
		if (CameraList[i])
		{
			CameraList[i]->Destroy();
		}
	}

	CameraList.Empty();
}

void AACameraConsole::SpawnCamera()
{
	if (CameraToSpawn)
	{
		UWorld* World = GetWorld();

		if (World)
		{
			FVector Location = GetActorLocation();
			FRotator Rotation = GetActorRotation();

			APawn* NewCamera = World->SpawnActor<APawn>(CameraToSpawn, Location, Rotation);

			CameraList.Add(NewCamera);
		}
	}

}


void AACameraConsole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


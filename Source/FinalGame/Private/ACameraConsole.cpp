// Fill out your copyright notice in the Description page of Project Settings.

#include "ACameraConsole.h"

// Sets default values
AACameraConsole::AACameraConsole()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AACameraConsole::BeginPlay()
{

	if (GetNetMode() == NM_DedicatedServer)
		return;

	PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
	PlayerController = GetWorld()->GetFirstPlayerController();

	Super::BeginPlay();

}

void AACameraConsole::OnInteract(bool action)
{
	if (!ConsoleInteract && action)
	{
		FVector PlayerPos = PlayerPawn->GetActorLocation();
		FVector ConsolePos = GetActorLocation();

		if (CameraList.Num() > IndexCamera)
		{
			PlayerController->Possess(CameraList[IndexCamera]);
			ConsoleInteract = true;

			EnableInput(PlayerController);
		}
	}
	else if (ConsoleInteract && !action)
	{
		PlayerController->Possess(PlayerPawn);
		ConsoleInteract = false;

		DisableInput(PlayerController);
	}
}

void AACameraConsole::NavigateCamera(int stepDir)
{
	if (CameraList.Num() > 0)
	{
		IndexCamera += stepDir;

		IndexCamera = IndexCamera < 0 ? CameraList.Num() - 1 : IndexCamera;
		IndexCamera %= CameraList.Num();

		PlayerController->Possess(CameraList[IndexCamera]);
	}
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

	UE_LOG(LogTemp, Warning, TEXT("Spawn camera"));
}


void AACameraConsole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "EOSServerSubsystem.generated.h"

UCLASS()
class FINALGAME_API UEOSServerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

#pragma region Lifecycle

	// Ensures this subsystem is only instantiated on dedicated servers, saving client memory
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

#pragma endregion

#pragma region Server Session

	// Configures and creates the EOS session tailored for a dedicated server environment
	UFUNCTION(BlueprintCallable, Category = "EOS|Server")
	void CreateServerSession();

	// Hooks to register the server's IP and Port with the custom Go matchmaking backend
	void RegisterServerWithBackend();
	void OnServerRegistrationComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

#pragma endregion

private:

#pragma region Session Callbacks

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

#pragma endregion

#pragma region Cached Data

	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;

#pragma endregion
};
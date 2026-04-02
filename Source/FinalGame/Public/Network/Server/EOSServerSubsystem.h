#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h" // OSSv1 Sessions
#include "EOSServerSubsystem.generated.h"

UCLASS()
class FINALGAME_API UEOSServerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "EOS Server")
	void CreateServerSession();

private:
	// Callback for when the session finishes creating
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
};
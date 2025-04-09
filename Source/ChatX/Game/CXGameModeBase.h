#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameModeBase.generated.h"

class ACXPlayerController;

UCLASS()
class CHATX_API ACXGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void OnPostLogin(AController* NewPlayer) override;

	FString GenerateSecretNumber();

	bool IsGuessNumberString(const FString& InNumberString);

	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString);

	virtual void BeginPlay() override;

	void PrintChatMessageString(ACXPlayerController* InChattingPlayerController, const FString& InChatMessageString);

	void IncreaseGuessCount(ACXPlayerController* InChattingPlayerController);

	void ResetGame();

	void JudgeGame(ACXPlayerController* InChattingPlayerController, int InStrikeCount);

	ACXPlayerController* GetCurrentTurnPlayerController() const;

private:
	UFUNCTION()
	void OnMainTimerElapsed();
	
protected:
	FString SecretNumberString;

	TArray<TObjectPtr<ACXPlayerController>> AllPlayerControllers;

	FTimerHandle MainTimerHandle;

	int32 CurrentTurnIndex = 0;
};

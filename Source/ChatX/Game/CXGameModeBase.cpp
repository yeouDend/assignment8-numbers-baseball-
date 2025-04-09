#include "Game/CXGameModeBase.h"
#include "CXGameStateBase.h"
#include "Player/CXPlayerController.h"
#include "EngineUtils.h"
#include "Player/CXPlayerState.h"


void ACXGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);

	// ACXGameStateBase* CXGameStateBase = GetGameState<ACXGameStateBase>();
	// if (IsValid(CXGameStateBase) == true)
	// {
	// 	CXGameStateBase->MulticastRPCBroadcastLoginMessage(TEXT("XXXXXX"));
	// }
	//
	// ACXPlayerController* CXPlayerController = Cast<ACXPlayerController>(NewPlayer);
	// if (IsValid(CXPlayerController) == true)
	// {
	// 	AllPlayerControllers.Add(CXPlayerController);
	// }

	ACXPlayerController* CXPlayerController = Cast<ACXPlayerController>(NewPlayer);
	if (IsValid(CXPlayerController) == true)
	{
		//CXPlayerController->NotificationText = FText::FromString(TEXT("Connected to the game server."));
		
		AllPlayerControllers.Add(CXPlayerController);

		ACXPlayerState* CXPS = CXPlayerController->GetPlayerState<ACXPlayerState>();
		if (IsValid(CXPS) == true)
		{
			CXPS->PlayerNameString = TEXT("Player") + FString::FromInt(AllPlayerControllers.Num());
		}

		ACXGameStateBase* CXGameStateBase = GetGameState<ACXGameStateBase>();
		if (IsValid(CXGameStateBase) == true)
		{
			CXGameStateBase->MulticastRPCBroadcastLoginMessage(CXPS->PlayerNameString);
		}

		if (CurrentTurnIndex == AllPlayerControllers.Num() - 1)
		{
			CXPlayerController->NotificationText = FText::FromString(TEXT("It's your turn!"));
		}
		else
		{
			CXPlayerController->NotificationText = FText::FromString(TEXT("Waiting for other player..."));
		}
	}



	
}

void ACXGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	SecretNumberString = GenerateSecretNumber();

	GetWorldTimerManager().SetTimer(MainTimerHandle, this, &ACXGameModeBase::OnMainTimerElapsed, 15.f, true);
}

FString ACXGameModeBase::GenerateSecretNumber()
{
	TArray<int32> Numbers;
	for (int32 i = 0; i <=9; ++i)
	{
		Numbers.Add(i);
	}

	FMath::RandInit(FDateTime::Now().GetTicks());
	Numbers = Numbers.FilterByPredicate([](int32 Num) {return Num > 0;});

	FString Result;
	for (int32 i =0; i<3; ++i)
	{
		int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}
	return Result;
}

// 3자리 숫자 받아 만드는 함수~
bool ACXGameModeBase::IsGuessNumberString(const FString& InNumberString)
{
	bool bCanPlay = false;

	do
	{
		if (InNumberString.Len() != 3)
		{
			break;
		}

		bool bIsUnique = true;
		TSet<TCHAR> UniqueDigits;
		for (TCHAR C : InNumberString)
		{
			if (FChar::IsDigit(C) == false || C == '0')
			{
				UniqueDigits.Add(C);
			}
		}

		if (bIsUnique == false)
		{
			break;
		}

		bCanPlay = true;
	}
	while (false);
	
	return bCanPlay;
}


FString ACXGameModeBase::JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString)
{
	int32 StrikeCount = 0, BallCount = 0;

	for (int32 i =0; i <3; ++i)
	{
		if (InSecretNumberString[i] == InGuessNumberString[i])
		{
			StrikeCount++;
		}
		else
		{
			FString PlayerGuessChar = FString::Printf(TEXT("%c"), InGuessNumberString[i]);
			if (InSecretNumberString.Contains(PlayerGuessChar))
			{
				BallCount++;
			}
		}
	}

	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}
	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

void ACXGameModeBase::PrintChatMessageString(ACXPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	FString ChatMessageString = InChatMessageString;
	int Index = InChatMessageString.Len() - 3;
	FString GuessNumberString = InChatMessageString.RightChop(Index);
	if (IsGuessNumberString(GuessNumberString) == true && InChattingPlayerController == GetCurrentTurnPlayerController())
	{
		FString JudgeResultString = JudgeResult(SecretNumberString, GuessNumberString);
		IncreaseGuessCount(InChattingPlayerController);
		for (TActorIterator<ACXPlayerController> It(GetWorld()); It; ++It)
		{
			ACXPlayerController* CXPlayerController = *It;
			if (IsValid(CXPlayerController) == true)
			{
				FString CombinedMessageString = InChatMessageString + TEXT(" -> ") + JudgeResultString;
				CXPlayerController->ClientRPCPrintChatMessageString(CombinedMessageString);

				int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));
				JudgeGame(InChattingPlayerController, StrikeCount);
			}
		}
	}
	else
	{
		for (TActorIterator<ACXPlayerController> it(GetWorld()); it; ++it)
		{
			ACXPlayerController* CXPlayerController = *it;
			if (IsValid(CXPlayerController) == true)
			{
				CXPlayerController->ClientRPCPrintChatMessageString(InChatMessageString);
			}
		}
	}
}

void ACXGameModeBase::IncreaseGuessCount(ACXPlayerController* InChattingPlayerController)
{
	ACXPlayerState* CXPS = InChattingPlayerController->GetPlayerState<ACXPlayerState>();
	if (IsValid(CXPS) == true)
	{
		CXPS->CurrentGuessCount++;
	}
}

void ACXGameModeBase::ResetGame()
{
	SecretNumberString = GenerateSecretNumber();

	for (const auto& CXPlayerController : AllPlayerControllers)
	{
		ACXPlayerState* CXPS = CXPlayerController->GetPlayerState<ACXPlayerState>();

		if (IsValid(CXPS) == true)
		{
			CXPS->CurrentGuessCount = 0;
		}
	}
	CurrentTurnIndex = 0;
}

void ACXGameModeBase::JudgeGame(ACXPlayerController* InChattingPlayerController, int InStrinkeCount)
{
	if ( 3 == InStrinkeCount)
	{
		ACXPlayerState* CXPS = InChattingPlayerController->GetPlayerState<ACXPlayerState>();
		for (const auto& CXPlayerController : AllPlayerControllers)
		{
			if (IsValid(CXPS) == true)
			{
				FString CombinedMessageString = CXPS->PlayerNameString + TEXT(" has won the game");
				CXPlayerController->NotificationText = FText::FromString(CombinedMessageString);

				ResetGame();
			}
		}
	}
	else
	{
		bool bIsDraw = true;
		for (const auto& CXPlayerController : AllPlayerControllers)
		{
			ACXPlayerState* CXPS = CXPlayerController->GetPlayerState<ACXPlayerState>();
			if (IsValid(CXPS) == true)
			{
				if (CXPS->CurrentGuessCount < CXPS->MaxGuessCount)
				{
					bIsDraw = false;
					break;
				}
			}
		}

		if (true == bIsDraw)
		{
			for (const auto& CXPlayerController : AllPlayerControllers)
			{
				CXPlayerController->NotificationText = FText::FromString(TEXT("Draw..."));

				ResetGame();
			}
		}
	}
}

ACXPlayerController* ACXGameModeBase::GetCurrentTurnPlayerController() const
{
	if (AllPlayerControllers.IsEmpty() == true)
	{
		return nullptr;
	}

	if (AllPlayerControllers.IsValidIndex(CurrentTurnIndex))
	{
		return AllPlayerControllers[CurrentTurnIndex];
	}
	return nullptr;
}

void ACXGameModeBase::OnMainTimerElapsed()
{
	if (AllPlayerControllers.Num() == 0)
	{
		return;
	}

	CurrentTurnIndex = (CurrentTurnIndex + 1) % AllPlayerControllers.Num();

	for (int32 i = 0; i < AllPlayerControllers.Num(); ++i)
	{
		if (IsValid(AllPlayerControllers[i]) == true)
		{
			if (i == CurrentTurnIndex)
			{
				AllPlayerControllers[i]->NotificationText = FText::FromString(TEXT("It's your turn!"));
			}
			else
			{
				AllPlayerControllers[i]->NotificationText = FText::FromString(TEXT("Waiting for other player..."));
			}
		}
	}
}
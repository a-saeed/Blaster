// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	//check if a certain number of players have joined before travelling to the map
	int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

	if (NumberOfPlayers >= 2)
	{
		UWorld* World = GetWorld();

		if (World)
		{
			bUseSeamlessTravel = true;
			World->ServerTravel(FString("/Game/Maps/Blastermap?listen"));
		}
	}
}

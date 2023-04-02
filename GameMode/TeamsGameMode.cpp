// Fill out your copyright notice in the Description page of Project Settings.


#include "TeamsGameMode.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Kismet/GamePlayStatics.h"

void ATeamsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));

	if (BlasterGameState)
	{
		ABlasterPlayerState* NewPlayerState = NewPlayer->GetPlayerState<ABlasterPlayerState>();

		if (NewPlayerState && NewPlayerState->GetTeam() == ETeam::ET_NoTeam)
		{
			// Add the new player to the least team in numbers.

			if (BlasterGameState->BlueTeam.Num() <= BlasterGameState->RedTeam.Num())
			{
				BlasterGameState->BlueTeam.AddUnique(NewPlayerState);
				NewPlayerState->SetTeam(ETeam::ET_BlueTeam);
			}
			else
			{
				BlasterGameState->RedTeam.AddUnique(NewPlayerState);
				NewPlayerState->SetTeam(ETeam::ET_RedTeam);
			}
		}
	}
}

void ATeamsGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
	ABlasterPlayerState* LeavingPlayerState = Exiting->GetPlayerState<ABlasterPlayerState>();

	if (BlasterGameState && LeavingPlayerState)
	{
		if (BlasterGameState->BlueTeam.Contains(LeavingPlayerState))
		{
			BlasterGameState->BlueTeam.Remove(LeavingPlayerState);
		}

		if (BlasterGameState->RedTeam.Contains(LeavingPlayerState))
		{
			BlasterGameState->RedTeam.Remove(LeavingPlayerState);
		}
	}
}

void ATeamsGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));

	if (BlasterGameState)
	{
		for (auto& PlayerState : BlasterGameState->PlayerArray)	//PlayerArray is a GameStateBase array for all player states
		{
			ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(PlayerState.Get());

			if (BlasterPlayerState && BlasterPlayerState->GetTeam() == ETeam::ET_NoTeam)	// players start of with no teams
			{
				// sort players to teams

				if (BlasterGameState->BlueTeam.Num() <= BlasterGameState->RedTeam.Num())
				{
					BlasterGameState->BlueTeam.AddUnique(BlasterPlayerState);
					BlasterPlayerState->SetTeam(ETeam::ET_BlueTeam);
				}
				else
				{
					BlasterGameState->RedTeam.AddUnique(BlasterPlayerState);
					BlasterPlayerState->SetTeam(ETeam::ET_RedTeam);
				}
			}
		}
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Gameframework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/GameState/BlasterGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			/*
			* UWorld* World = GetWorld();
			if (World)
			{
				bUseSeamlessTravel = true;
				World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
			}
			*/

			RestartGame();
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	//inform every player controller in the game of the current match state. 
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayerController)
		{
			BlasterPlayerController->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimPlayerController, ABlasterPlayerController* AttackerPlayerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerPlayerController ? Cast<ABlasterPlayerState>(AttackerPlayerController->PlayerState) : nullptr; //check if controller is not null
	ABlasterPlayerState* VictimPlayerState = VictimPlayerController ? Cast<ABlasterPlayerState>(VictimPlayerController->PlayerState) : nullptr;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	/*if the player didn't eliminate himself, add to his score*/
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		// before updating score.. check players currently in the lead.
		TArray<ABlasterPlayerState*> PlayersCurrentlyInTheLead;
		PlayersCurrentlyInTheLead = BlasterGameState->TopScoringPlayers;

		// update score
		AttackerPlayerState->AddToScore(1.f);
		BlasterGameState->UpdateTopScore(AttackerPlayerState);

		// new leader if found on TopScoring array and not found in previous TopScoring players
		if (BlasterGameState->TopScoringPlayers.Contains(AttackerPlayerState) && !PlayersCurrentlyInTheLead.Contains(AttackerPlayerState))
		{
			ABlasterCharacter* Leader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MultiCastGainedTheLead();
			}
		}

		// check if any player(s) lost the lead
		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			// if a former leader doesn't exist in the new leaders array
			if (!BlasterGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				ABlasterCharacter* LeadLoser = Cast<ABlasterCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (LeadLoser)
				{
					LeadLoser->MultiCastLostTheLead();
				}
			}
		}
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Eliminate(false);
	}

	//Broadcat Elimination Announcement. 
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayerController && AttackerPlayerState && VictimPlayerState)
		{
			BlasterPlayerController->BroadcastElim(AttackerPlayerState, VictimPlayerState);
		}
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	if (ElimmedController)
	{
		AActor* PlayerStart = FindPlayerStartWithLeastPlayersInrange();

		RestartPlayerAtPlayerStart(ElimmedController, PlayerStart); //character gets destroyed.. but the controller still exists to posses another.
	}
}

void ABlasterGameMode::PlayerLeftGame(class ABlasterPlayerState* LeavingPlayerState)
{
	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState && LeavingPlayerState)
	{
		if (BlasterGameState->TopScoringPlayers.Contains(LeavingPlayerState))	//remove from top scoring players
		{
			BlasterGameState->TopScoringPlayers.Remove(LeavingPlayerState);
		}
		//	player is leaving the game
		ABlasterCharacter* LeavingCharacter = Cast<ABlasterCharacter>(LeavingPlayerState->GetPawn());
		if (LeavingCharacter)
		{
			LeavingCharacter->Eliminate(true);
		}
	}
}

AActor* ABlasterGameMode::FindPlayerStartWithLeastPlayersInrange()
{
	//respawn player at player start with least players in range
	TArray<AActor*> PlayerStarts;
	TArray<AActor*> BlasterCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABlasterCharacter::StaticClass(), BlasterCharacters);

	int32 LeastPlayerStartIndex = 0;
	int32 PlayerCountInRange = 0;
	int32 MinPlayerCount = TNumericLimits<int32>::Max();

	for (int32 i = 0; i < PlayerStarts.Num(); i++)
	{
		for (AActor* Character : BlasterCharacters)
		{
			float DistanceToActor = (PlayerStarts[i]->GetActorLocation() - Character->GetActorLocation()).Size();

			if (DistanceToActor < PlayerStartRange) //can be changed in BP
			{
				PlayerCountInRange++;
			}
		}

		if (PlayerCountInRange < MinPlayerCount)
		{
			LeastPlayerStartIndex = i;
			MinPlayerCount = PlayerCountInRange;
		}
		PlayerCountInRange = 0;
	}

	return PlayerStarts[LeastPlayerStartIndex];
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Gameframework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimPlayerController, ABlasterPlayerController* AttackerPlayerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerPlayerController ? Cast<ABlasterPlayerState>(AttackerPlayerController->PlayerState) : nullptr; //check if controller is not null
	ABlasterPlayerState* VictimPlayerState = VictimPlayerController ? Cast<ABlasterPlayerState>(VictimPlayerController->PlayerState) : nullptr;
	/*if the player didn't eliminate himself, add to his score*/
	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}

	if (ElimmedCharacter)
	{
		ElimmedCharacter->Eliminate();
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

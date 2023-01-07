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
		//respawn at at a random player start
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);

		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]); //character gets destroyed.. but the controller still exists to posses another.
	}
}

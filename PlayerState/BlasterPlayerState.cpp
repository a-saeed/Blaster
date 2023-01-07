// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerState.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);
	//once the score changes, we need to update the HUD.
	UpdateHUDScore();
}

void ABlasterPlayerState::OnRep_Score()
{
	//once the score changes,inform all clients.
	UpdateHUDScore();
}

void ABlasterPlayerState::UpdateHUDScore()
{
	BlasterCharacter = BlasterCharacter == nullptr ? Cast<ABlasterCharacter>(GetPawn()) : BlasterCharacter;
	if (BlasterCharacter)
	{
		BlasterController = BlasterController == nullptr ? Cast<ABlasterPlayerController>(BlasterCharacter->Controller) : BlasterController;
		if (BlasterController)
		{
			BlasterController->SetHUDScore(GetScore());
		}
	}
}

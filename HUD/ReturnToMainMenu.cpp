// Fill out your copyright notice in the Description page of Project Settings.


#include "ReturnToMainMenu.h"
#include "GameFrameWork/PlayerController.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "Blaster/Character/BlasterCharacter.h"

bool UReturnToMainMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	return true;
}

void UReturnToMainMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	PlayerController = PlayerController == nullptr ? GetWorld()->GetFirstPlayerController() : PlayerController;
	if (PlayerController)
	{
		FInputModeGameAndUI InputModeData;
		InputModeData.SetWidgetToFocus(TakeWidget());
		PlayerController->SetInputMode(InputModeData);
		PlayerController->SetShowMouseCursor(true);
	}

	if (ReturnButton && !ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem)
		{
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UReturnToMainMenu::OnDestroySession);
		}
	}
}

void UReturnToMainMenu::MenuTearDown()
{
	RemoveFromParent();

	PlayerController = PlayerController == nullptr ? GetWorld()->GetFirstPlayerController() : PlayerController;
	if (PlayerController)
	{
		FInputModeGameOnly InputModeData;
		PlayerController->SetInputMode(InputModeData);
		PlayerController->SetShowMouseCursor(false);
	}

	if (ReturnButton && ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
	}

	if (MultiplayerSessionsSubsystem && MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
	{
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &UReturnToMainMenu::OnDestroySession);
	}
}

void UReturnToMainMenu::ReturnButtonClicked()
{
	/**
	* -Player is tryirng to leave the game by pressing the return to main menu button.
	* -Disable the button once clicked to prevent spamming.
	* -Check to see if this player controller is currently controlling a character.
	*	-Yes: access this chcracter and call ABlasterCharacter::ServerLeaveGame_Implementation() informing the server he's trying to leave; eventually calling UReturnToMainMenu::OnPlayerLeftGame() and destroying the session
	*	-No: means the player is trying to leave in the middle of an elimination montage; reenable the button and make him press again once he's respawnewd.
	* -first player controller will be the player controller for the local player. So it will give you the correct one based on which machine you are on.
	*/

	ReturnButton->SetIsEnabled(false);

	PlayerController = PlayerController == nullptr ? GetWorld()->GetFirstPlayerController() : PlayerController;	

	if (PlayerController)
	{
		ABlasterCharacter* LeavingCharacter = Cast<ABlasterCharacter>(PlayerController->GetPawn());
		if (LeavingCharacter)
		{
			LeavingCharacter->ServerLeaveGame();
			LeavingCharacter->OnLeftGame.AddDynamic(this, &UReturnToMainMenu::OnPlayerLeftGame);
		}
		else
		{
			ReturnButton->SetIsEnabled(true);
		}
	}
}

void UReturnToMainMenu::OnPlayerLeftGame()	//needs to be bound To BlasterCharacter's OnLeftGameDelegate 
{
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
}

void UReturnToMainMenu::OnDestroySession(bool bWasuccesful)
{
	if (!bWasuccesful)
	{
		ReturnButton->SetIsEnabled(true);
		return;
	}

	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGameModeBase>();
	if (GameMode)	//On Server
	{
		GameMode->ReturnToMainMenuHost();
	}
	else			//On Client
	{
		PlayerController = PlayerController == nullptr ? GetWorld()->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			PlayerController->ClientReturnToMainMenuWithTextReason(FText());
		}
	}
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "Announcement.h"
#include "SniperScope.h"
#include "ElimAnnouncement.h"
#include"Blueprint/WidgetLayoutLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "Blaster/HUD/ChatWidget.h"
#include "Components/EditableText.h"
#include "Blaster/HUD/ChatMessageWidget.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"

void ABlasterHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter(ViewportSize.X / 2.f, ViewportSize.Y / 2.f); //can't assign it with ( = )

		float SpreadScaled = CrosshairsSpreadMax * HUDPackage.CrosshairSpread; //in case we need to scale by a "CrosshairsSpreadMax" amount.. since spread will be in [0,1] range

		if (HUDPackage.CrosshairsCenter)
		{
			FVector2D Spread (0.f, 0.f); //no need to spread the center
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}

		if (HUDPackage.CrosshairsTop)
		{
			FVector2D Spread (0.f, -SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}

		if (HUDPackage.CrosshairsRight)
		{
			FVector2D Spread (SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}

		if (HUDPackage.CrosshairsLeft)
		{
			FVector2D Spread (-SpreadScaled, 0.f);
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}

		if (HUDPackage.CrosshairsBottom)
		{
			FVector2D Spread (0.f, SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, Spread, HUDPackage.CrosshairsColor);
		}
	}

}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairsColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();

	const FVector2D TextureDrawPoint (
		ViewportCenter.X - (TextureWidth / 2.f) + Spread.X , //Drag half its width and height left and up respectively
		ViewportCenter.Y - (TextureHeight / 2.f) + Spread.Y
	);

	DrawTexture(Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		CrosshairsColor
	);
}

/*
* OVERLAY WIDGETS
*/

void ABlasterHUD::AddCharacterOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
		CharacterOverlay->AddToViewport();
	}
}

void ABlasterHUD::AddAnnouncement()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && AnnouncementClass)
	{
		Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
		Announcement->AddToViewport();
	}
}

void ABlasterHUD::AddSniperScope()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController && SniperScopeClass)
	{
		SniperScope = CreateWidget<USniperScope>(PlayerController, SniperScopeClass);
		SniperScope->AddToViewport();
	}
}

/*
* Elim Announcement Widget
*/

void ABlasterHUD::AddElimAnnouncement(FString Attacker, FString Victim)
{
	OwnerPlayerController = OwnerPlayerController == nullptr ? GetOwningPlayerController() : OwnerPlayerController;
	if(OwnerPlayerController && ElimAnnouncementClass)
	{
		UElimAnnouncement* ElimAnnouncementWidget = CreateWidget<UElimAnnouncement>(OwnerPlayerController, ElimAnnouncementClass);
		if (ElimAnnouncementWidget)
		{
			ElimAnnouncementWidget->SetElimAnnouncementText(Attacker, Victim);
			ElimAnnouncementWidget->AddToViewport();

			/* Move all old Elim messages up before adding a new message */
			for (UElimAnnouncement* ElimMsg : ElimMessagesArray)
			{
				if (ElimMsg && ElimMsg->AnnouncementBox)
				{
					UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ElimMsg->AnnouncementBox);
					if (CanvasSlot)
					{
						FVector2d BoxPosition = CanvasSlot->GetPosition();	//position of the canvas slot associated with the horizontal box
						// move up by height of the box
						FVector2d NewPosition(
							BoxPosition.X,
							BoxPosition.Y - CanvasSlot->GetSize().Y		// (-) means up
						);

						CanvasSlot->SetPosition(NewPosition);
					}

				}
			}
			// Add New ELimMsg
			ElimMessagesArray.Add(ElimAnnouncementWidget);

			/* Start a local timer for each Annoucement once it's added to the viewport */
			FTimerHandle ElimMsgTimerHandle;
			FTimerDelegate ElimMsgDelegate;		// the function we eant to bind takes a paramater
			ElimMsgDelegate.BindUFunction(this, FName("ElimAnnouncementTimerFinished"), ElimAnnouncementWidget);	// bind with delegate

			GetWorldTimerManager().SetTimer(ElimMsgTimerHandle,
				ElimMsgDelegate,
				ElimAnnouncementTime,
				false
			);
		}
	}
}

void ABlasterHUD::ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove)
{
	if (MsgToRemove)
	{
		ElimMessagesArray.Remove(MsgToRemove);
		MsgToRemove->RemoveFromParent();
	}
}

/**
* Chat Widget 
* ----- (Controller - HUD - Player State) ------
*	-- Player presses a chat key from player controller to call AddChatWidget() --
*	-- Bind OnEnterMsg() to OnTextCommited delegate --
*	-- OnEnterMsg() -> send server rpc to controller -> send rpc to player state -> call AddChatMessage() --
*/

void ABlasterHUD::AddChatWidget(bool bFocusOnText)
{
	OwnerPlayerController = OwnerPlayerController == nullptr ? GetOwningPlayerController() : OwnerPlayerController;
	if (!OwnerPlayerController || !ChatClass) return;

	if (!ChatWidget)
	{
		ChatWidget = CreateWidget<UChatWidget>(OwnerPlayerController, ChatClass);
		if (ChatWidget) ChatWidget->AddToViewport();	// add to the viewport once
	}

	if (ChatWidget)
	{
		if (bFocusOnText)
		{
			ChatWidget->ChatText->SetUserFocus(OwnerPlayerController);
			if (!ChatWidget->ChatText->OnTextCommitted.IsBound())
			{
				ChatWidget->ChatText->OnTextCommitted.AddDynamic(this, &ABlasterHUD::OnEnterMsg);	// Bound only once
			}
		}
		else   // reset focus on gameplay.
		{
			FInputModeGameOnly InputModeData;
			OwnerPlayerController->SetInputMode(InputModeData);
		}
	}
}

void ABlasterHUD::OnEnterMsg(const FText& Text, ETextCommit::Type CommitMethod)
{
	ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(GetOwningPlayerController());

	if (ChatWidget && BlasterPlayerController && CommitMethod == ETextCommit::OnEnter)
	{
		// reset chat text to empty & move focus from text to gameplay
		ChatWidget->ChatText->SetText(FText::FromString(""));
		FInputModeGameOnly InputModeData;
		BlasterPlayerController->SetInputMode(InputModeData);
		BlasterPlayerController->SetChatTextFocus(false);
		// Check for empty text/empty white spaces
		if (Text.ToString().TrimStartAndEnd().IsEmpty()) return;
		// call server RPC
		BlasterPlayerController->ServerShowChatMessage(Text);
		//ChatWidget->ChatText->OnTextCommitted.RemoveDynamic(this, &ABlasterHUD::OnEnterMsg);
	}
}

// to be called from BlasterPlayerState class
void ABlasterHUD::AddChatMessage(FString PalyerName, FString Msg)
{
	OwnerPlayerController = OwnerPlayerController == nullptr ? GetOwningPlayerController() : OwnerPlayerController;
	if (OwnerPlayerController && ChatMessageClass)
	{
		UChatMessageWidget* ChatMessageWidget = CreateWidget<UChatMessageWidget>(OwnerPlayerController, ChatMessageClass);
		if (ChatMessageWidget)
		{
			ChatMessageWidget->SetMessageText(PalyerName, Msg);
			ChatMessageWidget->AddToViewport();

			/* Move all old chat messages up before adding a new message */
			for (UChatMessageWidget* ChatMsg : ChatMessagesArray)
			{
				if (ChatMsg && ChatMsg->ChatBox)
				{
					UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ChatMsg->ChatBox);
					if (CanvasSlot)
					{
						FVector2d BoxPosition = CanvasSlot->GetPosition();	//position of the canvas slot associated with the horizontal box
						// move up by height of the box
						FVector2d NewPosition(
							BoxPosition.X,
							BoxPosition.Y - CanvasSlot->GetSize().Y		// (-) means up
						);

						CanvasSlot->SetPosition(NewPosition);
					}
				}
			}
			// Add New ChatMsg
			ChatMessagesArray.Add(ChatMessageWidget);

			/* Start a local timer for each chat msg once it's added to the viewport */
			FTimerHandle ChatMsgTimerHandle;
			FTimerDelegate ChatMsgDelegate;		// the function we eant to bind takes a paramater
			ChatMsgDelegate.BindUFunction(this, FName("ChatTimerFinished"), ChatMessageWidget);	// bind with delegate

			GetWorldTimerManager().SetTimer(ChatMsgTimerHandle,
				ChatMsgDelegate,
				ChatMessageTime,
				false
			);
		}
	}
}

void ABlasterHUD::ChatTimerFinished(UChatMessageWidget* MsgToRemove)
{
	if (MsgToRemove)
	{
		ChatMessagesArray.Remove(MsgToRemove);
		MsgToRemove->RemoveFromParent();
	}
}


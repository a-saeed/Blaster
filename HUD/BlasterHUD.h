// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

/**
*
*/
USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:

	UPROPERTY()
		class UTexture2D* CrosshairsCenter;
	UPROPERTY()
		UTexture2D* CrosshairsTop;
	UPROPERTY()
		UTexture2D* CrosshairsRight;
	UPROPERTY()
		UTexture2D* CrosshairsLeft;
	UPROPERTY()
		UTexture2D* CrosshairsBottom;

	float CrosshairSpread;
	FLinearColor CrosshairsColor;
};
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:

	virtual void DrawHUD() override;
	
	void AddCharacterOverlay(); //called by the player controller
	void AddAnnouncement();
	void AddSniperScope();
	void AddElimAnnouncement(FString Attacker, FString Victim);
	void AddChatWidget(bool bFocusOnText);
	void AddChatMessage(FString PalyerName, FString Msg);
	void AddArtifactStatus(FString ArtifactStatus, FSlateColor Color);

protected:

	virtual void BeginPlay() override;
	
private:

	FHUDPackage HUDPackage;

	UPROPERTY()
	class APlayerController* OwnerPlayerController;

	UPROPERTY(EditAnywhere)
		float CrosshairsSpreadMax = 16.f;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairsColor);
	/*
	* CHARACTER OVERLAY WIDGET
	*/
	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "PlayerStats")
		TSubclassOf<class UUserWidget> CharacterOverlayClass;
	/*
	* ANNOUNCEMENT WIDGET
	*/
	UPROPERTY()
	class UAnnouncement* Announcement;

	UPROPERTY(EditAnywhere, Category = "Announcements")
		TSubclassOf<class UUserWidget> AnnouncementClass;
	/*
	* Sniper Scope WIDGET
	*/
	UPROPERTY()
		class USniperScope* SniperScope;

	UPROPERTY(EditAnywhere, Category = "Sniper Scope")
		TSubclassOf<class UUserWidget> SniperScopeClass;

	/*
	* Elim Announcement Widget
	*/

	UPROPERTY(EditAnywhere, Category = "ElimAnnouncement")
	TSubclassOf<class UElimAnnouncement> ElimAnnouncementClass;

	UPROPERTY(EditAnywhere, Category = "ElimAnnouncement")
	float ElimAnnouncementTime = 2.5f;

	UFUNCTION()
	void ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove);

	UPROPERTY()
	TArray<UElimAnnouncement*> ElimMessagesArray;

	/*
	* Chat Widget
	*/

	UPROPERTY()
	class UChatWidget* ChatWidget;

	UPROPERTY(EditAnywhere, Category = "Chat")
	TSubclassOf<UChatWidget> ChatClass;

	UPROPERTY(EditAnywhere, Category = "Chat")
	TSubclassOf<class UChatMessageWidget> ChatMessageClass;

	UPROPERTY(EditAnywhere, Category = "Chat")
	float ChatMessageTime = 2.5f;

	UPROPERTY()
	TArray<UChatMessageWidget*> ChatMessagesArray;

	UFUNCTION()
	void OnEnterMsg(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void ChatTimerFinished(UChatMessageWidget* MsgToRemove);

	/*
	* Artifact status
	*/

	UPROPERTY()
	class UArtifactStatus* ArtifactStatusWidget;

	UPROPERTY(EditAnywhere, Category = "Artifact Status")
	TSubclassOf<UArtifactStatus> ArtifactStatusClass;

	FTimerHandle ArtifactStatusTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Artifact Status")
	float ArtifactStatusTime = 2.5f;

	UFUNCTION()
	void ArtifactStatusTimerFinished();

public:

	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

	FORCEINLINE UCharacterOverlay* GetCharacterOverlay(){ return CharacterOverlay; }

	FORCEINLINE UAnnouncement* GetAnnouncement(){ return Announcement; }

	FORCEINLINE USniperScope* GetSniperScope() { return SniperScope; }

	FORCEINLINE UChatWidget* GetChatWidget() const { return ChatWidget; }
};

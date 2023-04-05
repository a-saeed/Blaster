// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
	
private:
	/*
	* HEALTH
	*/	
	UPROPERTY(meta = (BindWidget))
		class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
		class UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* ShieldBar;
	/*
	* SCORE
	*/
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreText;
	/*
	* DEFEATS
	*/
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefeatsAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefeatsText;

	/*
	* Ammo
	*/
	UPROPERTY(meta = (BindWidget))
		UTextBlock* WeaponAmmoAmount;

	UPROPERTY(meta = (BindWidget))
		UTextBlock* CarriedAmmoAmount;
	/*
	* Weapon Type
	*/
	UPROPERTY(meta = (BindWidget))
		UTextBlock* WeaponTypeText;
	/*
	* Match Countdown Timer
	*/
	UPROPERTY(meta = (BindWidget))
		UTextBlock* MatchCountdownText;
	/*
	* Grenadees
	*/
	UPROPERTY(meta = (BindWidget))
	UTextBlock* GrenadesText;

	/*
	* Wifi Image
	*/
	UPROPERTY(meta = (BindWidget))
	class UImage* HighPingImage;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	UWidgetAnimation* HIghPingAnimation;

	/*
	* Teams
	*/

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BlueTeamScore;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RedTeamScore;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreSpacer;

public:

	FORCEINLINE UProgressBar* GetHealthBar() const { return HealthBar; }
	FORCEINLINE UTextBlock* GetHealthText() const { return HealthText; }
	FORCEINLINE UProgressBar* GetShieldBar() const { return ShieldBar; }

	FORCEINLINE UTextBlock* GetScoreAmount() const { return ScoreAmount; }
	FORCEINLINE UTextBlock* GetScoreText() const { return ScoreText; }

	FORCEINLINE UTextBlock* GetDefeatsAmount() const { return DefeatsAmount; }
	FORCEINLINE UTextBlock* GetDefeatsText() const { return DefeatsText; }

	FORCEINLINE UTextBlock* GetAmmoText() const { return WeaponAmmoAmount; }
	FORCEINLINE UTextBlock* GetCarriedAmmoText() const { return CarriedAmmoAmount; }

	FORCEINLINE UTextBlock* GetWeaponTypeText() const { return WeaponTypeText; }

	FORCEINLINE UTextBlock* GetMatchCountdownText() const { return MatchCountdownText; }

	FORCEINLINE UTextBlock* GetGrenadesText() const { return GrenadesText; }

	FORCEINLINE UImage* GetWifiImage() const { return HighPingImage; }
	FORCEINLINE UWidgetAnimation* GetWifiAnimation() const { return HIghPingAnimation; }

	FORCEINLINE UTextBlock* GetBlueScoreText() const { return BlueTeamScore; }
	FORCEINLINE UTextBlock* GetRedScoreText() const { return RedTeamScore; }
	FORCEINLINE UTextBlock* GetScoreSpacerText() const { return ScoreSpacer; }
};

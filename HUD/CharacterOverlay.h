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
	/*
	* SCORE
	*/
	UPROPERTY(meta = (BindWidget))
		UTextBlock* ScoreAmount;
	/*
	* DEFEATS
	*/
	UPROPERTY(meta = (BindWidget))
		UTextBlock* DefeatsAmount;
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
public:

	FORCEINLINE UProgressBar* GetHealthBar() const { return HealthBar; }
	FORCEINLINE UTextBlock* GetHealthText() const { return HealthText; }

	FORCEINLINE UTextBlock* GetScoreText() const { return ScoreAmount; }

	FORCEINLINE UTextBlock* GetDefeatsText() const { return DefeatsAmount; }

	FORCEINLINE UTextBlock* GetAmmoText() const { return WeaponAmmoAmount; }
	FORCEINLINE UTextBlock* GetCarriedAmmoText() const { return CarriedAmmoAmount; }

	FORCEINLINE UTextBlock* GetWeaponTypeText() const { return WeaponTypeText; }

	FORCEINLINE UTextBlock* GetMatchCountdownText() const { return MatchCountdownText; }
};

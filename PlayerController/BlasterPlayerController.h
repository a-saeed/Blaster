// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/Weapon/WeaponTypes.h"
#include "BlasterPlayerController.generated.h"
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	/*
	* Blaster Character HUD
	*/
	void SetHUDHealth(float Health, float MaxHealth); //called in blaster character
	/*
	* Player state HUD
	*/
	void SetHUDScore(float Score);

	void SetHUDDefeats(int32 Defeats);
	/*
	* Ammo HUD
	*/
	void SetHUDWeaponAmmo(int32 WeaponAmmoAmount);
	void SetHUDCarriedAmmo(int32 CarriedAmmoAmount);
	/*
	* Weapon Type HUD
	*/
	void SetHUDWeaponType(FText WeaponTypeText);
protected:

	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

public:

};

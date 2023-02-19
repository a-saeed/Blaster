#pragma once

UENUM(BlueprintType) //in case we need to use it in blueprints
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket launcher"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_SubmachineGun UMETA(DisplayName = "SubmachineGun "),

	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};
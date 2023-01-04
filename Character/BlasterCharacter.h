// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"

#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:

	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//C7_2: to make a class able to replicate variables, we have to override getLifeTimeProps to be able to replicate for this class
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override; //an inherited function to access the component once it's initialized.
	/*
	* ANIMATION MONTAGES
	*/
	void PlayFireMontage(bool bAiming);
	/*
	* ELIMINATION
	*/
	void Eliminate();

protected:

	virtual void BeginPlay() override;

	//input Functions.. protected as they might be needed if a class inherits from character
	void MoveForward(float value);
	void moveRight(float value);
	void LookUp(float value);
	void Turn(float value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void AimOffset(float DeltaTime);
	void TurnInPlace(float DeltaTime);
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();

private:
	/*
	* BODY
	*/
	UPROPERTY(VisibleAnywhere, Category = "Camera")
		class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
		class UCameraComponent* FollowCamera;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon) //C7_1: used to designate this variable to be a replicated variable. linked to C7_8
		class AWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere)
		class UCombatComponent* Combat;

	class ABlasterPlayerController* BlasterPlayerController;

	void LimitPitchView();
	/***************** ON_REPs & RPCs *******************/

	UFUNCTION()
		void OnRep_OverlappingWeapon(AWeapon* LastWeapon); //C7_8: on replicating this variable (overlappingWeapon), execute this function. it can only take a param of the type of variable it replicates

	UFUNCTION(Server, Reliable)
		void ServerEquipButtonPressed();

	/***************** AIM OFFSET & TURNING IN PLACE ************************/
	float AO_Yaw;
	float AO_Pitch;
	float InterpAO_Yaw;
	FRotator LastAimRotation;

	ETurningInPlace TurningInPlace;
	/*
	* ANIMATION MONTAGE
	*/
	UPROPERTY(EditAnywhere, Category = "Combat")
		class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
		class UAnimMontage* HitReactMontage;

	void PlayHitreactMontage(); //used in multicast rpc
	/*
	* CAMERA
	*/
	UPROPERTY(EditAnywhere)
		float CameraThreshold = 200.f; //how close to other objects until our character dissapear

	void HideCameraIfCharatcterClose();
	/*
	* PLAYER STATS
	*/
	UPROPERTY(EditAnywhere, Category = "PlayerStats")
		float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
		float Health = 100.f;

	//Callback to Unreal's OnTakeAnyDamage() Delegate
	UFUNCTION()
		void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCausor);

	//called each time damage is received
	void UpdateHUDHealth();

	//health rep notify
	UFUNCTION()
		void OnRep_Health();
public:	

	//C7_4
	void SetOverlappingWeapon(AWeapon* Weapon);

	bool isWeaponEquipped() const;

	bool isAiming() const;

	FORCEINLINE float GetAo_Yaw() { return AO_Yaw;} //used in anime instance class
	FORCEINLINE float GetAo_Pitch() { return AO_Pitch;} //used in anime instance class

	AWeapon* GetEquippedWeapon();

	FORCEINLINE ETurningInPlace GetTurningInPlace() { return TurningInPlace; }
	
	FVector GetHitTarget();

	FORCEINLINE UCameraComponent* GetFollowCamera() { return FollowCamera; } //used to access the FOV
};

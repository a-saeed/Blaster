// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"

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
	void PlayElimMontage();
	/*
	* ELIMINATION
	*/
	void Eliminate(); //only on the server

	UFUNCTION(NetMulticast, Reliable)
		void MulticastEliminate();

	virtual void Destroyed() override;

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

	UPROPERTY(EditAnywhere, Category = "Combat")
		class UAnimMontage* ElimMontage;

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
	UPROPERTY(EditAnywhere, Category = "Player Stats")
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
	/*
	* ELIMINATION
	*/
	bool bElimed = false; //used in anime instance

	FTimerHandle ElimTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Elimination")
		float ElimDelay = 3.f;

	void ElimTimerFinished();
	/*
	* DISSOLVE EFFECTS
	*/
	UPROPERTY(VisibleAnywhere)
		UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere, Category = "Elimination")
		UCurveFloat* DissolveCurve;

	UPROPERTY(VisibleAnywhere, Category = "Elimination") //dynamic instance we can change at runtime
		UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = "Elimination") //material instance set on the blueprint, used with the dyncamic material instance
		UMaterialInstance* DissolveMaterialInstance;

	UFUNCTION()
		void UpdateDissolveMaterial(float DissolveValue);

	void StartDissolve();
	/*
	* ELIM BOT
	*/
	UPROPERTY(EditAnywhere, Category = "ElimBot")
		class UParticleSystem* ElimBotEffect;

	UPROPERTY(EditAnywhere, Category = "ElimBot")
		class UParticleSystemComponent* ElimBotComponent; //to store the bot and destroy it

	UPROPERTY(EditAnywhere, Category = "ElimBot")
		class USoundCue* ElimSound;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
		class USoundCue* SpawnSound;
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

	FORCEINLINE bool IsElimmed() const { return bElimed; }
};

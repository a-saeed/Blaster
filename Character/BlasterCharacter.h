// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Blaster/BlasterTypes/CombatState.h"

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
	void PlayReloadMontage();
	void PlayThrowGrenadeMontage();

	/*
	* ELIMINATION
	*/

	void Eliminate(); //only on the server

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminate();

	virtual void Destroyed() override;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	/*
	* HUD Health / Shield
	*/

	void UpdateHUDHealth();
	void UpdateHUDShield();

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
	void ReloadButtonPressed();
	void GrenadeButtonPressed();
	/*poll for ane relevant classes(player state) and once they're available, initialize our HUD*/
	void PollInit();

private:

	/*
	* BODY
	*/

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	class UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

	void LimitPitchView();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	/*
	* Default Weapon
	*/

	UPROPERTY()
	class AWeapon* StartingWeapon;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AWeapon> DefaultWeaponClass;

	void SpawnDefaultWeapon();

	/*
	* OVERLAPPING WEAPON
	*/

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);	//only take a param of the type of variable it replicates

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	/*
	* AIM OFFSET & TURNING IN PLACE
	*/

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

	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	class UAnimMontage* ThrowGrenadeMontage;

	void PlayHitreactMontage(); //used in multicast rpc

	/*
	* CAMERA
	*/

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f; //how close to other objects until our character dissapear

	void HideCameraIfCharatcterClose();

	/*
	* HEALTH / SHIELD
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, VisibleAnywhere, Category = "Player Stats")
	float Shield = 100.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCausor);

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

	FORCEINLINE UCombatComponent* GetCombatComponent() const { return Combat; }
	FORCEINLINE UBuffComponent* GetBuffComponent() const { return Buff; }

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

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }

	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }

	ECombatState GetCombatState();

	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }

	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
};
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//C7_2: to make a class able to replicate variables, we have to override getLifeTimeProps to be able to replicate for this class
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override; //an inherited function to access the component once it's initialized.

protected:

	virtual void BeginPlay() override;

	//input Functions.. protected as they might be needed if a class inherits from character
	void MoveForward(float value);
	void moveRight(float value);
	void LookUp(float value);
	void Turn(float value);
	void EquipButtonPressed();

private:

	UPROPERTY(VisibleAnywhere, Category = "Camera")
		class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
		class UCameraComponent* FollowCamera;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon) //C7_1: used to designate this variable to be a replicated variable. linked to C7_8
		class AWeapon* OverlappingWeapon;

	UFUNCTION()
		void OnRep_OverlappingWeapon(AWeapon* LastWeapon); //C7_8: on replicating this variable (overlappingWeapon), execute this function. it can only take a param of the type of variable it replicates

	UPROPERTY(VisibleAnywhere)
		class UCombatComponent* Combat;

	UFUNCTION(Server, Reliable)
		void ServerEquipButtonPressed();
public:	

	//C7_4
	void SetOverlappingWeapon(AWeapon* Weapon);

};

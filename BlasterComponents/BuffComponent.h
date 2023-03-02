// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UBuffComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	friend class ABlasterCharacter;
	
	bool HealPlayer(float HealAmount, float HealingTime);
	bool BuffShield(float ReplenishAmount, float ReplenishTime);
	void SpeedPlayer(float BaseBuffedSpeed, float CrouchBuffedSpeed, float BuffTime);
	void BuffJump(float JumpZVelocity, float BuffTime);

protected:

	virtual void BeginPlay() override;

private:
	/*
	*	BODY
	*/

	UPROPERTY()
	class ABlasterCharacter* Character;

	/*
	* 
	*	Heal Buff
	* 
	*/

	bool bHealing = false;
	float HealingRate = 0.f;
	float AmountToHeal = 0.f;
	void HealRampUp(float DeltaTime);

	/*
	*
	*	Shield Buff
	*
	*/

	bool bReplenishing = false;
	float ReplenishRate = 0.f;
	float AmountToReplenish = 0.f;
	void ReplenishRampUp(float DeltaTime);

	/*
	*
	*	Speed Buff
	*
	*/

	float InitialBaseSpeed;
	float InitialCrouchSpeed;
	FTimerHandle SpeedTimerHandle;
	void ResetSpeeds();

	UPROPERTY(EditAnywhere)
	class UParticleSystem* SpeedEffect;

	class UParticleSystemComponent* SpeedComponent;

	void PlaySpeedEffects();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/*
	*
	*	Jump Buff
	*
	*/

	float InitialZVelocity;
	FTimerHandle JumpTimerHandle;
	void ResetJump();

	UPROPERTY(EditAnywhere)
	class UParticleSystem* JumpEffect;

	class UParticleSystemComponent* JumpComponent;

	void PlayJumpEffects();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);

public:		

	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void SetInitialJumpVelocity(float Velocity);
};

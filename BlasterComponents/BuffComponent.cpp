// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "TimerManager.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
	ReplenishRampUp(DeltaTime);
}

/*
*
*	Heal Buff
*
*/

bool UBuffComponent::HealPlayer(float HealAmount, float HealingTime)
{
	if (!Character || Character->GetHealth() == Character->GetMaxHealth()) return false;

	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal = HealAmount;

	return true;
}


void UBuffComponent::HealRampUp(float DeltaTime)
{
	if (!bHealing || !Character || Character->IsElimmed()) return;

	const float HealThisFrame = HealingRate * DeltaTime;

	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth()));
	Character->UpdateHUDHealth();		//Update Health for the server

	AmountToHeal -= HealThisFrame;
	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0.f;				//if it's not already!
	}
}


/*
*
*	Shield Buff
*
*/

bool UBuffComponent::BuffShield(float ReplenishAmount, float ReplenishTime)
{
	if (!Character || Character->GetShield() == Character->GetMaxShield()) return false;

	bReplenishing = true;
	ReplenishRate = ReplenishAmount / ReplenishTime;
	AmountToReplenish = ReplenishAmount;

	return true;
}

void UBuffComponent::ReplenishRampUp(float DeltaTime)
{
	if (!bReplenishing || !Character || Character->IsElimmed()) return;

	const float ReplenishThisFrame = ReplenishRate * DeltaTime;

	Character->SetShield(FMath::Clamp(Character->GetShield() + ReplenishThisFrame, 0.f, Character->GetMaxShield()));
	Character->UpdateHUDShield();		//Update Health for the server

	AmountToReplenish -= ReplenishThisFrame;
	if (AmountToReplenish <= 0.f || Character->GetShield() >= Character->GetMaxShield())
	{
		bReplenishing = false;
		AmountToReplenish = 0.f;				//if it's not already!
	}
}

/*
*
*	Speed Buff
*
*/

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed)
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::SpeedPlayer(float BaseBuffedSpeed, float CrouchBuffedSpeed, float BuffTime)
{
	if (!Character) return;

	Character->GetWorldTimerManager().SetTimer(SpeedTimerHandle, this, &UBuffComponent::ResetSpeeds, BuffTime);

	MulticastSpeedBuff(BaseBuffedSpeed, CrouchBuffedSpeed);
}

void UBuffComponent::ResetSpeeds()
{
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BaseSpeed, float CrouchSpeed)
{
	if (!Character || !Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;

	if (BaseSpeed > InitialBaseSpeed)
	{
		PlaySpeedEffects();
	}
	else
	{
		if (SpeedComponent)
		{
			SpeedComponent->DestroyComponent();
		}
	}
}

void UBuffComponent::PlaySpeedEffects()
{
	if (SpeedEffect)
	{
		SpeedComponent = UGameplayStatics::SpawnEmitterAttached(
			SpeedEffect,
			Character->GetRootComponent(),
			FName(""),
			FVector(Character->GetActorLocation().X, Character->GetActorLocation().Y, Character->GetActorLocation().Z - 100),
			Character->GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

/*
*
*	Jump Buff
*
*/

void UBuffComponent::SetInitialJumpVelocity(float Velocity)
{
	InitialZVelocity = Velocity;
}

void UBuffComponent::BuffJump(float JumpZVelocity, float BuffTime)
{
	if (!Character) return;

	Character->GetWorldTimerManager().SetTimer(SpeedTimerHandle, this, &UBuffComponent::ResetJump, BuffTime);

	MulticastJumpBuff(JumpZVelocity);
}

void UBuffComponent::ResetJump()
{
	MulticastJumpBuff(InitialZVelocity);
}

void UBuffComponent::MulticastJumpBuff_Implementation(float JumpVelocity)
{
	if (!Character || !Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->JumpZVelocity = JumpVelocity;

	if (JumpVelocity > InitialZVelocity)
	{
		PlayJumpEffects();
	}
	else
	{
		if (SpeedComponent)
		{
			SpeedComponent->DestroyComponent();
		}
	}
}

void UBuffComponent::PlayJumpEffects()
{
	if (JumpEffect)
	{
		SpeedComponent = UGameplayStatics::SpawnEmitterAttached(
			JumpEffect,
			Character->GetRootComponent(),
			FName(""),
			FVector(Character->GetActorLocation().X, Character->GetActorLocation().Y, Character->GetActorLocation().Z - 100),
			Character->GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			false
		);
	}
}

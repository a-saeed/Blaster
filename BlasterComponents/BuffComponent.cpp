// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "Blaster/Character/BlasterCharacter.h"

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


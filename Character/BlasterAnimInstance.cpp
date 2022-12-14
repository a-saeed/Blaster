// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFrameWork/CharacterMovementComponent.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner()); //this function only exists for Anime Isntance
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (BlasterCharacter == nullptr)
	{
		BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner()); //Defensive. as native update could be called before native initialize
	}

	if (BlasterCharacter == nullptr) return;

	//set speed
	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f; //we only care about forward/right movement
	speed = Velocity.Length();

	//set is in air
	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();

	//set is accelerating (if player is pressing move buttons)
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Length() > 0.f ? true : false;

}

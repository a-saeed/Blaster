// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterAnimInstance.h"
#include "BlasterCharacter.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/Weapon/Weapon.h"

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

	//set is weapon equipped
	bWeaponEquipped = BlasterCharacter->isWeaponEquipped();

	//set equipped weapon
	EquippedWeapon = BlasterCharacter->GetEquippedWeapon();

	//set is character crouched
	bIsCrouched = BlasterCharacter->bIsCrouched; //from character movement component

	//set is aiming
	bAiming = BlasterCharacter->isAiming();

	//Offset Yaw for strafing
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation(); //this is the rotation of aiming (Controller rotation) regardless the player rotation
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity()); //this is the rotation of the player regardless the rotation of the aiming
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation); //this will result in values passing from (0 90 180 -90 0) each value results in a different animation played.
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 10.f);
	YawOffset = DeltaRotation.Yaw;

	//set Lean
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.0f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	//set aim offsets
	AO_Yaw = BlasterCharacter->GetAo_Yaw();
	AO_Pitch = BlasterCharacter->GetAo_Pitch();

	//set left hand transform
	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && BlasterCharacter->GetMesh())
	{
		//set LHT to the world transform of the socket on the weapon
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		//transform it from world space to bone relative space on the skeleton of the character mesh.(we transform realtive to the right hand)
		FVector OutPosition;
		FRotator OutRotation; //the position and rotation of the left hand socket transformed to hand_r bone space.
		BlasterCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		
		//now LHT is set to the location of the socket but it's transforme realtive to hand_r bone space.
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
		//we use this in the FABRIK algorithm im the anime bp
	}

	//set ETurningInPlace
	TurningInPlace = BlasterCharacter->GetTurningInPlace();
}
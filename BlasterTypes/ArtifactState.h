#pragma once

UENUM(BlueprintType)
enum class EArtifactState : uint8
{
	EAS_Equipped UMETA(DisplayName = "Equipped"),
	EAS_Dropped UMETA(DisplayName = "Dropped"),
	EAS_Captured UMETA(DisplayName = "Captured"),

	EWS_MAX UMETA(DisplayName = "DefaultMAX"),
};

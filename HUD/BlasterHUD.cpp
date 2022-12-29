// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterHUD.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter = (ViewportSize / 2.f, ViewportSize / 2.f);

		if (HUDPackage.CrosshairsCenter)
		{
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter);
		}

		if (HUDPackage.CrosshairsTop)
		{
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter);
		}

		if (HUDPackage.CrosshairsRight)
		{
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter);
		}

		if (HUDPackage.CrosshairsLeft)
		{
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter);
		}

		if (HUDPackage.CrosshairsBottom)
		{
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter);
		}
	}

}


void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter)
{
	float TextureWidth = Texture->GetSizeX();
	float TextureHeight = Texture->GetSizeY();

	FVector2D TextureDrawPoint = (
		ViewportCenter - (TextureWidth / 2.f), //Drag half its width and height left and up respectively
		ViewportCenter - (TextureHeight / 2.f)
	);

	DrawTexture(Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		FLinearColor::White
	);
}
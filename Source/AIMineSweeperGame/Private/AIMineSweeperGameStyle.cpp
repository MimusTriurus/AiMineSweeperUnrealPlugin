// Copyright Epic Games, Inc. All Rights Reserved.

#include "AIMineSweeperGameStyle.h"
#include "AIMineSweeperGame.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FAIMineSweeperGameStyle::StyleInstance = nullptr;

void FAIMineSweeperGameStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FAIMineSweeperGameStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FAIMineSweeperGameStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("AIMineSweeperGameStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FAIMineSweeperGameStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("AIMineSweeperGameStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("AIMineSweeperGame")->GetBaseDir() / TEXT("Resources"));

	Style->Set("AIMineSweeperGame.PluginAction", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));
	return Style;
}

void FAIMineSweeperGameStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FAIMineSweeperGameStyle::Get()
{
	return *StyleInstance;
}

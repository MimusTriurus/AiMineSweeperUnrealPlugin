#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FMineCell
{
	int32 Value = 0;
	bool bRevealed = false;
};

class SMinesweeperWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMinesweeperWidget) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);

private:
    TArray<TArray<TSharedPtr<SButton>>> MineCells;
    TSharedPtr<SEditableTextBox> TokenInputBox;
    TSharedPtr<SEditableTextBox> ChatInputBox;
    TSharedPtr<SEditableTextBox> InstructionsInputBox;
    TSharedPtr<SButton> ButtonStart;

    TSharedPtr<SVerticalBox> MineFieldPanel;
    TSharedPtr<SVerticalBox> ControlPanel;
    
    TArray<TArray<FMineCell>> MineField;
    FReply OnTryToDemine(int32 X, int32 Y);
    FReply OnStartGame();
    TSharedRef<SWidget> GenerateMineFieldUI();
};
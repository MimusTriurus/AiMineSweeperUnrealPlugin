#include "SAIMineSweeperWindow.h"

#include "HttpModule.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

namespace
{
    const FString MINE_TITLE = TEXT("M");
    const int32 MINE_VALUE = -1;
    const FString CELL_TITLE = TEXT("?");
    
    const FString MSG_VICTORY = TEXT("Victory!");
    const FString MSG_GAMEOVER = TEXT("Game is over!");
    const FString MSG_ERROR_ON_NEW_GAME = TEXT("An error occurred when creating a minefield. Click 'New Game' again. Check logs.");
    const FString MSG_ERROR_ON_GET_FROM_LLM = TEXT("Error when requesting data from LLM. Check logs.");

    const FString LLM_API_ENDPOINT = TEXT("https://api.openai.com/v1/chat/completions");
    const FString LLM_MODEL = TEXT("gpt-4o-mini");

    const FString DEFAULT_REQUEST = TEXT("Generate a new 4x4 Minesweeper grid with 1 mines.");
    const FString DEFAULT_INSTRUCTION = TEXT("Return only json string describing game field. Nothing other than this json should be sent to the user. The mine must be marked with the -1 number.");
    
    
    FString ExtractContentFromJson(const FString& JsonString)
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
            if (JsonObject->TryGetArrayField(TEXT("choices"), ChoicesArray) && ChoicesArray->Num() > 0)
            {
                TSharedPtr<FJsonObject> MessageObject = (*ChoicesArray)[0]->AsObject()->GetObjectField(TEXT("message"));
                if (MessageObject.IsValid())
                {
                    FString Content;
                    if (MessageObject->TryGetStringField(TEXT("content"), Content))
                    {
                        return Content;
                    }
                }
            }
        }
        return TEXT("Error: Failed to extract content");
    }

    FString CleanJsonString(const FString& RawJson)
    {
        FString CleanedJson = RawJson;
        CleanedJson.RemoveFromStart(TEXT("```json"));
        CleanedJson.RemoveFromEnd(TEXT("```"));
        CleanedJson = CleanedJson.Replace(TEXT("\n"), TEXT(""));
        return CleanedJson;
    }

    bool ParseMinefieldJson(const FString& JsonString, TArray<TArray<FMineCell>>& OutMinefield)
    {
        OutMinefield.Empty();
        TSharedPtr<FJsonValue> JsonParsed;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

        if (FJsonSerializer::Deserialize(Reader, JsonParsed) && JsonParsed.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>* JsonArray;
            if (JsonParsed->TryGetArray(JsonArray))
            {
                for (const TSharedPtr<FJsonValue>& RowValue : *JsonArray)
                {
                    const TArray<TSharedPtr<FJsonValue>>* RowArray;
                    if (RowValue->TryGetArray(RowArray))
                    {
                        TArray<FMineCell> RowData;
                        for (const TSharedPtr<FJsonValue>& Cell : *RowArray)
                        {
                            RowData.Add(FMineCell{static_cast<int32>(Cell->AsNumber()), false});
                        }
                        OutMinefield.Add(RowData);
                    }
                }
                return true;
            }
        }
        return false;
    }

    bool CheckWinCondition(const TArray<TArray<FMineCell>>& Minefield)
    {
        for (const TArray<FMineCell>& Row : Minefield)
        {
            for (const FMineCell& Cell : Row)
            {
                if (Cell.Value != MINE_VALUE && !Cell.bRevealed)
                {
                    return false;
                }
            }
        }
        return true;
    }
}


class IHttpRequest;BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMinesweeperWidget::Construct(const FArguments& InArgs)
{
    MineFieldPanel = SNew(SVerticalBox);
    MineFieldPanel->AddSlot()
    [
        GenerateMineFieldUI()
    ];
    
    ControlPanel = SNew(SVerticalBox);
    ControlPanel->AddSlot()
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Access token:"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SAssignNew(TokenInputBox, SEditableTextBox)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Request:"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SAssignNew(ChatInputBox, SEditableTextBox)
            .Text(FText::FromString(DEFAULT_REQUEST))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Instruction:"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SAssignNew(InstructionsInputBox, SEditableTextBox)
            .Text(FText::FromString(DEFAULT_INSTRUCTION))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(5)
        [
            SAssignNew(ButtonStart, SButton)
            .Text(FText::FromString(TEXT("New game")))
            .OnClicked(FOnClicked::CreateSP(this, &SMinesweeperWidget::OnStartGame))
        ]
    ];

    
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            ControlPanel.ToSharedRef()
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MineFieldPanel.ToSharedRef()
        ]
    ];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SMinesweeperWidget::OnTryToDemine(int32 X, int32 Y)
{
    if (MineField[X][Y].Value == MINE_VALUE)
    {
        for (int32 i = 0; i < MineField.Num(); ++i)
        {
            for (int32 j = 0; j < MineField[i].Num(); ++j)
            {
                if (MineField[i][j].Value == MINE_VALUE)
                {
                    MineCells[i][j]->SetContent(SNew(STextBlock).Text(FText::FromString(MINE_TITLE)));
                }
            }
        }
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(MSG_GAMEOVER));
        return FReply::Handled();
    }

    int32 AdjacentMines = 0;
    for (int32 i = -1; i <= 1; ++i)
    {
        for (int32 j = -1; j <= 1; ++j)
        {
            int32 AdjacentX = X + i;
            int32 AdjacentY = Y + j;
            if (AdjacentX >= 0 && AdjacentX < MineField.Num() && AdjacentY >= 0 && AdjacentY < MineField[X].Num())
            {
                if (MineField[AdjacentX][AdjacentY].Value == MINE_VALUE)
                {
                    ++AdjacentMines;
                }
            }
        }
    }
    MineField[X][Y].bRevealed = true;
    MineCells[X][Y]->SetContent(SNew(STextBlock).Text(FText::AsNumber(AdjacentMines)));
    
    if (CheckWinCondition(MineField))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(MSG_VICTORY));
        MineFieldPanel->ClearChildren();
    }
    return FReply::Handled();
}

FReply SMinesweeperWidget::OnStartGame()
{
    FString Message = ChatInputBox->GetText().ToString();
    
    TSharedPtr<FJsonObject> ContentObject = MakeShareable(new FJsonObject);
    ContentObject->SetStringField("model", ::LLM_MODEL);
    ContentObject->SetBoolField("store", true);

    TSharedPtr<FJsonObject> MessageObject = MakeShareable(new FJsonObject);
    MessageObject->SetStringField("role", "user");
    auto ContentTxt = FText::Format(FText::FromString("{0}. {1}"), ChatInputBox->GetText(), InstructionsInputBox->GetText());
    MessageObject->SetStringField("content", ContentTxt.ToString());
    
    TArray<TSharedPtr<FJsonValue>> MessagesArray;
    MessagesArray.Add(MakeShareable(new FJsonValueObject(MessageObject)));

    ContentObject->SetArrayField("messages", MessagesArray);

    FString Content;
    TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&Content);
    FJsonSerializer::Serialize(ContentObject.ToSharedRef(), JsonWriter);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(::LLM_API_ENDPOINT);
    Request->SetVerb("POST");
    Request->AppendToHeader("Content-Type", "application/json");
    Request->AppendToHeader("Authorization", FString::Format(TEXT("Bearer {0}"), { TokenInputBox->GetText().ToString() }));

    Request->SetContentAsString(Content);

    Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr, FHttpResponsePtr Response, bool)
    {
        if (Response.IsValid() && Response->GetResponseCode() == 200)
        {
            const FString JsonResponse = Response->GetContentAsString();
            const FString Content = ExtractContentFromJson(JsonResponse);
            const FString CleanedJson = CleanJsonString(Content);
            if (ParseMinefieldJson(CleanedJson, MineField))
            {
                MineFieldPanel->ClearChildren();
                MineFieldPanel->AddSlot()
                [
                    GenerateMineFieldUI()
                ];
            }
            else
            {
                FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(MSG_ERROR_ON_NEW_GAME));
                UE_LOG(LogTemp, Error, TEXT("Failed to parse Minefield JSON! %s"), *CleanedJson);
            }
        }
        else
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(MSG_ERROR_ON_GET_FROM_LLM));
            UE_LOG(LogTemp, Error, TEXT("Response is invalid: %d"), Response->GetResponseCode());
        }
    });
    Request->ProcessRequest();
    return FReply::Handled();
}

TSharedRef<SWidget> SMinesweeperWidget::GenerateMineFieldUI()
{
    TSharedRef<SGridPanel> GridPanel = SNew(SGridPanel);

    MineCells.SetNum(MineField.Num());
    for (int32 X = 0; X < MineField.Num(); ++X)
    {
        MineCells[X].SetNum(MineField[X].Num());
        for (int32 Y = 0; Y < MineField[X].Num(); ++Y)
        {
            TSharedPtr<SButton> Button = SNew(SButton)
                .Text(FText::FromString(CELL_TITLE))
                .OnClicked(FOnClicked::CreateSP(this, &SMinesweeperWidget::OnTryToDemine, X, Y));

            MineCells[X][Y] = Button;

            GridPanel->AddSlot(X, Y)
            [
                Button.ToSharedRef()
            ];
        }
    }

    return GridPanel;
}
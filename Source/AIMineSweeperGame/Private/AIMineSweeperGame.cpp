#include "AIMineSweeperGame.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "SAIMineSweeperWindow.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FAIMineSweeperGameModule"

void FAIMineSweeperGameModule::StartupModule()
{
    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

    TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
    MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, nullptr, 
        FMenuExtensionDelegate::CreateRaw(this, &FAIMineSweeperGameModule::AddMenuExtension));

    LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner("MineSweeperUITab",
        FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& Args)
        {
            return SNew(SDockTab)
                .TabRole(ETabRole::NomadTab)
                [
                    SNew(SMinesweeperWidget)
                ];
        }))
        .SetDisplayName(FText::FromString("AI Minesweeper"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FAIMineSweeperGameModule::ShutdownModule()
{
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("MineSweeperUITab");
}

void FAIMineSweeperGameModule::AddMenuExtension(FMenuBuilder& MenuBuilder)
{
    MenuBuilder.AddMenuEntry(
        LOCTEXT("OpenMineSweeperUI", "AI Minesweeper"),
        LOCTEXT("OpenMineSweeperUI_Tooltip", "Launch AI Minesweeper UI"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FAIMineSweeperGameModule::OpenMineSweeperUI))
    );
}

void FAIMineSweeperGameModule::OpenMineSweeperUI()
{
    FGlobalTabmanager::Get()->TryInvokeTab(FName("MineSweeperUITab"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAIMineSweeperGameModule, AIMineSweeperGame)
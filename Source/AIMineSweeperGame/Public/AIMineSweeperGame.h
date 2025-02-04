#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FAIMineSweeperGameModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void AddMenuExtension(FMenuBuilder& MenuBuilder);
	void OpenMineSweeperUI();
};

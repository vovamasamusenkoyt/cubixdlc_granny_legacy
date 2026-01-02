#pragma once

// Module system header
// Include this to access the module system

#include "il2cpp/il2cpp.h"

// Example usage:
// 
// #include "modules/modules.h"
// 
// class MyModule : public CubixDLC::IL2CPP::ModuleBase
// {
// public:
//     MyModule() 
//     {
//         CubixDLC::IL2CPP::ModuleManager::GetInstance().RegisterModule(this);
//     }
//     
//     void OnEnable() override { /* initialization */ }
//     void OnDisable() override { /* cleanup */ }
//     void OnUpdate() override { /* called every frame */ }
// };
//
// // In your code:
// static MyModule g_MyModule;
// g_MyModule.SetEnabled(true); // Enable module


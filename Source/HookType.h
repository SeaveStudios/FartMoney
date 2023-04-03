#pragma once

#include "Hooks/MinHook.h"
#include "Platform/Macros/IsPlatform.h"

#if IS_WIN32()
// Easily switch hooking method for all hooks, choose between MinHook/VmtHook/VmtSwap
using HookType = MinHook;
#endif

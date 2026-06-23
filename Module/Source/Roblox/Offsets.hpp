#pragma once

#include "Structs.hpp"

#include <cstdint>
#include <Windows.h>

#define REBASE(Address) (Address + reinterpret_cast<uintptr_t>(GetModuleHandleA("RobloxPlayerBeta.exe")))
#define HYPREBASE(Address) (Address + reinterpret_cast<uintptr_t>(GetModuleHandleA("RobloxPlayerBeta.dll")))

// Updated for version 76173e47a79145c7

namespace Offsets
{
    const uintptr_t Print = REBASE(0x1E08380);
    const uintptr_t TaskDefer = REBASE(0x1DE3270); // Conflict: 0x1DE3270 vs 0x1DE3240
    const uintptr_t IsLegalSendEvent = REBASE(0xA18810); // Unused
    const uintptr_t AppDataInfo = REBASE(0x7F31380); // Unused
    const uintptr_t GetLuaStateForInstance = REBASE(0x1CB0530);

    namespace DataModel
    {
        const uintptr_t FakeDataModelPointer = REBASE(0x7A39AD8);
        const uintptr_t FakeDataModelToDataModel = 0x1D8;
        const uintptr_t GameLoaded = 0x670;
        const uintptr_t PlaceId = 0x1a0;
        const uintptr_t GameId = 0x198;
        const uintptr_t CreatorId = 0x190;
        const uintptr_t JobId = 0x138;
        const uintptr_t Workspace = 0x178;
    }

    namespace TaskScheduler
    {
        const uintptr_t Pointer = REBASE(0x7FCB088);
        const uintptr_t JobStart = 0xC8;
        const uintptr_t JobEnd = 0xD0;
        const uintptr_t JobName = 0x18;
        const uintptr_t MaxFPS = 0xB0;
    }

    namespace Luau
    {
        const uintptr_t Luau_Execute = REBASE(0x45AB200);
        const uintptr_t LuaO_NilObject = REBASE(0x67AE440);
        const uintptr_t LuaH_DummyNode = REBASE(0x67AE2E8);
        const uintptr_t LuaVM_Load = REBASE(0x1CC8D60);
        const uintptr_t LuaC_Step = REBASE(0x453D620);
        const uintptr_t OpcodeLookupTable = REBASE(0x6056C90);
    }

    namespace Identity
    {
        const uintptr_t Pointer = REBASE(0x7FBD4A0);
        const uintptr_t GetIdentityStruct = REBASE(0x82D0); //0x51D9EC0 //0x49896e0
        namespace IdentityStruct
        {
            const uintptr_t Capabilities = 0x28;
        }
    }

    namespace Reflection
    {
        const uintptr_t PushInstance = REBASE(0x1CA3840);
        const uintptr_t PushInstance2 = REBASE(0x1CA3880);
        const uintptr_t GetPropertyData = REBASE(0x1C91ED0); // Conflict: GetValues (0x1C91ED0) vs GetProperty (0x352F4A0)
        const uintptr_t KTable = REBASE(0x7FCCAC0);
        const uintptr_t CastArgs = REBASE(0x1C77C00); // Unable to update
    }

    namespace Input
    {
        const uintptr_t FireMouseClick = REBASE(0x2566BF0);
        const uintptr_t FireRightMouseClick = REBASE(0x2567260);
        const uintptr_t FireMouseHoverEnter = REBASE(0x2566E00);
        const uintptr_t FireMouseHoverLeave = REBASE(0x2566FF0);
        const uintptr_t FireTouchInterest = REBASE(0x29BDAC0);
        const uintptr_t FireProximityPrompt = REBASE(0x74C500);
    }

    namespace FastFlags
    {
        const uintptr_t GetFFlag = REBASE(0x48CE390); // Unable to update
        const uintptr_t SetFFlag = REBASE(0x48C4C40); // Unable to update
    }

    namespace BasePart
    {
        inline constexpr uintptr_t Primitive = 0x148;
        inline constexpr uintptr_t Overlap = 0x1F0;
    }

    namespace Instance
    {
        inline constexpr uintptr_t ChildrenStart = 0x78;
        inline constexpr uintptr_t ChildrenEnd = 0x8;
        inline constexpr uintptr_t ClassDescriptor = 0x18;
        inline constexpr uintptr_t ClassName = 0x8;
        inline constexpr uintptr_t Name = 0xB0;
        inline constexpr uintptr_t Parent = 0x70;
        inline constexpr uintptr_t This = 0x8;
    }

    namespace RenderJob
    {
        inline constexpr uintptr_t RenderJobToView = 0xA0;
        inline constexpr uintptr_t RenderViewToDevice = 0x48;
        inline constexpr uintptr_t SwapChain = 0xC8;
    }

    namespace Script
    {
        const uintptr_t ScriptContextResume = REBASE(0x1D79260);

        namespace LocalScript
        {
            inline constexpr uintptr_t ByteCode = 0x1A8;
            inline constexpr uintptr_t GUID = 0xE8;
            inline constexpr uintptr_t Hash = 0x1B8;
        }

        namespace ModuleScript
        {
            inline constexpr uintptr_t ByteCode = 0x150;
            inline constexpr uintptr_t GUID = 0xE8;
            inline constexpr uintptr_t Hash = 0x160;
        }

        namespace ByteCode
        {
            inline constexpr uintptr_t Pointer = 0x10;
            inline constexpr uintptr_t Size = 0x20;
        }

        namespace Environment
        {
            inline constexpr uintptr_t Node = 0x180; // Unable to update
            inline constexpr uintptr_t WeakThreadRef = 0x08; // Unable to update
            inline constexpr uintptr_t LiveThreadRef = 0x20; // Unable to update
            inline constexpr uintptr_t Thread = 0x08; // Unable to update
        }
    }

    namespace ExtraSpace
    {
        const uintptr_t RequireBypass = 0x851;
        
        const uintptr_t ScriptContextToResume = 0x7F8;
    }

    namespace Signal
    {
        const uintptr_t ConnectionDisconnect = REBASE(0x2FA7DC0);
    }

    namespace Connection
    {
        inline constexpr uintptr_t Next = 0x10;
        inline constexpr uintptr_t Enabled = 0x20;
        inline constexpr uintptr_t SlotWrapper = 0x30;
    }

    namespace CallbackValue
    {
        inline constexpr uintptr_t Offset = 0x78; // Unable to update
        inline constexpr uintptr_t HasCallback = 0x38; // Unable to update
        inline constexpr uintptr_t Structure = 0x18; // Unable to update
        inline constexpr uintptr_t ObjectRefs = 0x38; // Unable to update
        inline constexpr uintptr_t ObjectRef = 0x28; // Unable to update
        inline constexpr uintptr_t ReferenceId = 0x14; // Unable to update
    }

    namespace SlotWrapper
    {
        inline constexpr uintptr_t LiveThreadRef = 0x60;
    }

    namespace LiveThreadRef
    {
        inline constexpr uintptr_t Thread = 0x28;
        inline constexpr uintptr_t ThreadId = 0x10; // Unused
        inline constexpr uintptr_t FuncId = 0x14;
    }

    namespace PropertyDescriptor
    {
        inline constexpr uintptr_t Name = 0x8; // Unused
        inline constexpr uintptr_t PropertyDescriptorBitFlags = 0x8C;
        inline constexpr uintptr_t ScriptableMask = 0x10;
        inline constexpr uintptr_t GetSet = 0x90; // Unused
        inline constexpr uintptr_t Type = 0x60; // Unused
        inline constexpr uintptr_t TypeNumber = 0x30; // Unused
        inline constexpr uintptr_t Getter = 0x18; // Unused
    }

    namespace Hyperion
    {
        const uintptr_t ControlFlowGuardian = HYPREBASE(0xF9BCE0); // Unable to update
        const uintptr_t ValidateCandidate = HYPREBASE(0x4200D0); // Unable to update
    }
}

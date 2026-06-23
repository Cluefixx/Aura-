#pragma once

#include <Exploit/Classes/DataModel/DataModel.hpp>
#include <Roblox/Offsets.hpp>
#include "Structs.hpp"
#include <cstdarg>
#include <cstdint>
#include <excpt.h>
#include <lstate.h>

struct lua_State;

namespace Roblox
{
    inline auto Print = reinterpret_cast<uintptr_t(*)(int, const char*, ...)>(Offsets::Print);
    inline auto Luau_Execute = reinterpret_cast<void(__fastcall*)(lua_State*)>(Offsets::Luau::Luau_Execute);
    inline auto TaskDefer = reinterpret_cast<int(__fastcall*)(lua_State*)>(Offsets::TaskDefer);
    inline auto ScriptContextResume = (uint64_t(__fastcall*)(uint64_t, YieldState*, YieldingLuaThread**, uint32_t, uint8_t, uint64_t))Offsets::Script::ScriptContextResume;
    inline auto GetLuaStateForInstance = reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t, uintptr_t*, uintptr_t*)>(Offsets::GetLuaStateForInstance);
    inline auto GetIdentityStruct = reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(Offsets::Identity::GetIdentityStruct);
    inline auto PushInstance = reinterpret_cast<void(__fastcall*)(lua_State*, void**)>(Offsets::Reflection::PushInstance);
    inline auto PushInstance2 = (uintptr_t * (__fastcall*)(lua_State*, std::shared_ptr<uintptr_t*>))Offsets::Reflection::PushInstance2;
    inline auto CastArgs = reinterpret_cast<uintptr_t(__fastcall*)(lua_State*, int, void*, bool, int)>(Offsets::Reflection::CastArgs);
    inline auto GetPropertyDataRaw = reinterpret_cast<void* (__fastcall*)(uintptr_t, uintptr_t*, uintptr_t*, int)>(Offsets::Reflection::GetPropertyData);

    inline void* GetPropertyData(uintptr_t classDescriptorPropertyMap, uintptr_t* outputValue, uintptr_t* propertyDescriptorAtom, int argumentFour)
    {
        return GetPropertyDataRaw(classDescriptorPropertyMap, outputValue, propertyDescriptorAtom, argumentFour);
    }

    inline uintptr_t* GetPropertyData(uintptr_t classDescriptorPropertyMap, uintptr_t* propertyDescriptorAtom)
    {
        if (!classDescriptorPropertyMap || !propertyDescriptorAtom)
        {
            return nullptr;
        }

        uintptr_t outputValue = 0;
        __try
        {
            GetPropertyDataRaw(classDescriptorPropertyMap, &outputValue, propertyDescriptorAtom, 0);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }

        if (*reinterpret_cast<uint8_t*>((uintptr_t)&outputValue + 4))
        {
            return nullptr;
        }

        uintptr_t classDescriptor = classDescriptorPropertyMap - 0x250;
        uintptr_t bucketMask = *reinterpret_cast<uintptr_t*>(classDescriptor + 728);
        uintptr_t bucketIndices = *reinterpret_cast<uintptr_t*>(classDescriptor + 712);
        uintptr_t propertyTable = *reinterpret_cast<uintptr_t*>(classDescriptor + 720);

        if (!bucketIndices || !propertyTable)
        {
            return nullptr;
        }

        uint32_t index = *reinterpret_cast<uint32_t*>(bucketIndices + 4 * outputValue);
        uintptr_t entry = propertyTable + 16ULL * (bucketMask & index);
        if (!entry)
        {
            return nullptr;
        }

        return reinterpret_cast<uintptr_t*>(entry);
    }

    inline auto KTable = reinterpret_cast<uintptr_t*>(Offsets::Reflection::KTable);
    inline auto LuaVMLoad = reinterpret_cast<int(__fastcall*)(lua_State*, const char*, const char*, size_t, int)>(Offsets::Luau::LuaVM_Load);
    inline auto ConnectionDisconnect = reinterpret_cast<void(__fastcall*)(uintptr_t)>(Offsets::Signal::ConnectionDisconnect);
    inline auto GetFastFlag = reinterpret_cast<bool(__fastcall*)(const char*, bool*)>(Offsets::FastFlags::GetFFlag);
    inline auto SetFastFlag = reinterpret_cast<bool(__fastcall*)(const char*, const char*)>(Offsets::FastFlags::SetFFlag);

    namespace Signals {
        inline auto FireProximityPrompt = (uintptr_t * (__thiscall*)(uintptr_t))Offsets::Input::FireProximityPrompt;
        inline auto FireMouseClick = (void(__fastcall*)(__int64, float, __int64))Offsets::Input::FireMouseClick;
        inline auto FireRightMouseClick = (void(__fastcall*)(__int64, float, __int64))Offsets::Input::FireRightMouseClick;
        inline auto FireMouseHoverEnter = (void(__fastcall*)(__int64, __int64))Offsets::Input::FireMouseHoverEnter;
        inline auto FireMouseHoverLeave = (void(__fastcall*)(__int64, __int64))Offsets::Input::FireMouseHoverLeave;
        inline auto FireTouchInterest = (void(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, bool, bool))Offsets::Input::FireTouchInterest;
    }

    inline DataModel GetDataModel()
    {
        uintptr_t FakeDataModel = *reinterpret_cast<uintptr_t*>(Offsets::DataModel::FakeDataModelPointer);
        if (!FakeDataModel) return DataModel(0);

        uintptr_t DataModelAddress = *reinterpret_cast<uintptr_t*>(FakeDataModel + Offsets::DataModel::FakeDataModelToDataModel);
        return DataModel(DataModelAddress);
    }
}

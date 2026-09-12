#pragma once

template <typename ReturnType, typename ThisType, typename... Args> inline ReturnType CallVFunc(int index, ThisType* thisPtr, Args... args)
{
    using Function = ReturnType(__fastcall*)(ThisType*, Args...);
    return (*reinterpret_cast<Function const* const*>(thisPtr))[index](thisPtr, args...);
}

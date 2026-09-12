#pragma once

#include <cstdint>

class CBaseHandle
{
  public:
    constexpr CBaseHandle() = default;
    explicit constexpr CBaseHandle(std::uint32_t value) : m_Index(value)
    {
    }

    constexpr std::uint32_t ToInt() const
    {
        return m_Index;
    }

  protected:
    std::uint32_t m_Index = 0xFFFFFFFFu;
};

static_assert(sizeof(CBaseHandle) == 0x4);

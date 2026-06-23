#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <zstd/zstd.h>
#include <zstd/common/xxhash.h>

namespace Bytecode
{
    inline std::string Decompress(const std::string& Source)
    {
        if (Source.size() <= 8)
        {
            return "";
        }

        static const char Magic[4] = { 'R', 'S', 'B', '1' };

        std::string Input = Source;

        uint8_t Key[4];
        std::memcpy(Key, Input.data(), 4);
        for (uint32_t Index = 0; Index < 4; ++Index)
        {
            Key[Index] ^= Magic[Index];
            Key[Index] -= Index * 41;
        }

        for (size_t Index = 0; Index < Input.size(); ++Index)
        {
            Input[Index] ^= Key[Index % 4] + Index * 41;
        }

        uint32_t DecompressedSize = 0;
        std::memcpy(&DecompressedSize, Input.data() + 4, 4);

        std::vector<uint8_t> Output(DecompressedSize);
        const size_t Result = ZSTD_decompress(Output.data(), DecompressedSize, Input.data() + 8, Input.size() - 8);

        if (ZSTD_isError(Result))
        {
            return "";
        }

        return std::string(reinterpret_cast<const char*>(Output.data()), DecompressedSize);
    }
}

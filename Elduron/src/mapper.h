#pragma once
#include "common.h"

uintptr_t oraclemap(const rbxctx* ctx,
                     const std::vector<char>& dll,
                     uintptr_t& out_base);

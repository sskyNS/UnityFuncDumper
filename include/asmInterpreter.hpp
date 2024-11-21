#pragma once
#include "dmntcht.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
extern "C"
{
#include "armadillo.h"
#include "strext.h"
}

void dumpPointers(const std::vector<std::string> &UnityNames,
                  const std::vector<uint32_t> &UnityOffsets,
                  const DmntCheatProcessMetadata &cheatMetadata,
                  const std::string &unity_sdk);

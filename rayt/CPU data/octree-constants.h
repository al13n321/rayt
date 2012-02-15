#pragma once

#include "common.h"

const char * const kNodeLinkChannelName = "node links";
const int kNodeLinkSize = 4; // size of node in node links channel
const int kBlockHeaderSize = 52; // bytes
const uint kRootBlockParent = (1 << 24) - 1;
const int kMaxBlocksCount = (1 << 23) - 1;

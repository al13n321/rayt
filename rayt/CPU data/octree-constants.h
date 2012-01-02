#pragma once

#include "common.h"

const char * const kNodeLinkChannelName = "node links";
const int kNodeLinkSize = 4; // size of node in node links channel
const uint kFaultNode = (1 << 24) - 1;
const uint kRootBlockParent = (1 << 24) - 1;

#pragma once

#include "common.h"

namespace rayt {

	const char * const kNodeLinkChannelName = "node links";
	const int kNodeLinkSize = 4; // size of node in node links channel
	const int kBlockHeaderSize = 76; // bytes
	const uint kRootBlockParent = (1 << 24) - 1;
	const int kMaxBlocksCount = (1 << 23) - 1;

	inline uint PackStoredNodeLink(uchar children_mask, bool fault_flag, bool duplication_flag, uint ptr) {
		assert(ptr < (1U << 21));
		return (ptr << 11) | ((duplication_flag ? 1U: 0U) << 9) | ((fault_flag ? 1U: 0U) << 8) | children_mask;
	}

	inline void UnpackStoredNodeLink(uint link, uchar &children_mask, bool &fault_flag, bool &duplication_flag, uint &ptr) {
		children_mask = link & 255;
		fault_flag = !!(link & (1 << 8));
		duplication_flag = !!(link & (1 << 9));
		ptr = link >> 11;
	}

	inline uchar UnpackStoredChildrenMask(uint link) {
		return link & 255;
	}

	inline bool UnpackStoredFaultFlag(uint link) {
		return !!(link & (1 << 8));
	}

	inline bool NeedFarPointer(int pointer_index, int pointed_index) {
		return abs(pointer_index - pointed_index) >= (1 << 20);
	}

	inline uint NearPointerValue(int pointer_index, int pointed_index) {
		return (1 << 20) + (pointed_index - pointer_index);
	}

	inline uint PackCacheNodeLink(uchar children_mask, bool fault_flag, bool duplication_flag, bool far_flag, uint ptr) {
		assert(ptr < (1U << 21));
		return (ptr << 11) | ((far_flag ? 1U: 0U) << 10) | ((duplication_flag ? 1U: 0U) << 9) | ((fault_flag ? 1U: 0U) << 8) | children_mask;
	}

	// just for debug
	inline void UnpackCacheNodeLink(uint link, uchar &children_mask, bool &fault_flag, bool &duplication_flag, bool &far_flag, uint &ptr) {
		children_mask = link & 255;
		fault_flag = !!(link & (1 << 8));
		duplication_flag = !!(link & (1 << 9));
		far_flag = !!(link & (1 << 10));
		ptr = link >> 11;
	}

}

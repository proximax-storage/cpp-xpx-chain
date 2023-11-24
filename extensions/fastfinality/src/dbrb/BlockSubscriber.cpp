/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BlockSubscriber.h"

namespace catapult { namespace dbrb {

	void BlockSubscriber::notifyBlock(const model::BlockElement& blockElement) {
		auto pBlockHandler = m_pWeakBlockHandler.lock();
		if (pBlockHandler)
			pBlockHandler->handleBlock(blockElement);
	}

	void BlockSubscriber::notifyDropBlocksAfter(Height height) {
		// No block rollbacks in fast finality.
	}
}}

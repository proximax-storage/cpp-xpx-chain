/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/io/BlockChangeSubscriber.h"

namespace catapult { namespace dbrb {

    class BlockHandler {
	public:
		virtual void handleBlock(const model::BlockElement& blockElement) = 0;
    };

    class BlockSubscriber : public io::BlockChangeSubscriber {
	public:
		explicit BlockSubscriber(std::weak_ptr<BlockHandler> pWeakBlockHandler)
			: m_pWeakBlockHandler(std::move(pWeakBlockHandler))
		{}

	public:
		void notifyBlock(const model::BlockElement& blockElement) override;
		void notifyDropBlocksAfter(Height height) override;

    private:
		std::weak_ptr<BlockHandler> m_pWeakBlockHandler;
    };
}}

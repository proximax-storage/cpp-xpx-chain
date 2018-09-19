/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once

#include "catapult/cache/CatapultCache.h"
#include "catapult/chain/BlockScorer.h"
#include "catapult/chain/ChainUtils.h"
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/extensions/LocalNodeStateRef.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/observers/ObserverContext.h"
#include "InputUtils.h"

namespace catapult {
	namespace cache { class CatapultCache; }
	namespace chain { struct ObserverState; }
}

namespace catapult { namespace consumers {

	/// A tuple composed of a block, a hash and a generation hash.
	class WeakBlockInfo : public model::WeakEntityInfoT<model::Block> {
	public:
		/// Creates a block info.
		constexpr WeakBlockInfo() : m_pGenerationHash(nullptr)
		{}

		/// Creates a block info around \a blockElement.
		constexpr explicit WeakBlockInfo(const model::BlockElement& blockElement)
				: WeakEntityInfoT(blockElement.Block, blockElement.EntityHash)
				, m_pGenerationHash(&blockElement.GenerationHash)
		{}

	public:
		/// Gets the generation hash.
		constexpr const Hash256& generationHash() const {
			return *m_pGenerationHash;
		}

	private:
		const Hash256* m_pGenerationHash;
	};

	struct SyncState {
	public:
		SyncState() = default;

		explicit SyncState(const extensions::LocalNodeStateRef& localNodeState)
			: EffectiveBalanceHeight(localNodeState.Config.BlockChain.EffectiveBalanceRange)
			, m_pOriginalCurrentCache(&localNodeState.CurrentCache)
			, m_pOriginalPreviousCache(&localNodeState.PreviousCache)
			, m_pOriginalState(&localNodeState.State)
			, m_pCacheCurrentDelta(std::make_unique<cache::CatapultCacheDelta>(m_pOriginalCurrentCache->createDelta()))
			, m_pCachePreviousDelta(std::make_unique<cache::CatapultCacheDelta>(m_pOriginalPreviousCache->createDelta()))
			, m_stateCopy(localNodeState.State)
			, m_storage(&localNodeState.Storage)
		{}

	public:
		Height EffectiveBalanceHeight;

	public:
		WeakBlockInfo commonBlockInfo() const {
			return WeakBlockInfo(*m_pCommonBlockElement);
		}

		Height commonBlockHeight() const {
			return m_pCommonBlockElement->Block.Height;
		}

		const model::ChainScore& scoreDelta() const {
			return m_scoreDelta;
		}

		const cache::CatapultCacheDelta& currentCacheDelta() const {
			return *m_pCacheCurrentDelta;
		}

		const cache::CatapultCacheDelta& previousCacheDelta() const {
			return *m_pCacheCurrentDelta;
		}

	public:
		consumers::TransactionInfos detachRemovedTransactionInfos() {
			return std::move(m_removedTransactionInfos);
		}

		observers::ObserverState currentObserverState() {
			return observers::ObserverState(*m_pCacheCurrentDelta, m_stateCopy);
		}

		observers::ObserverState preivousObserverState() {
			return observers::ObserverState(*m_pCachePreviousDelta, m_stateCopy);
		}

		io::BlockStorageView storage() const {
			return m_storage->view();
		}

		void update(
				std::shared_ptr<const model::BlockElement>&& pCommonBlockElement,
				model::ChainScore&& scoreDelta,
				consumers::TransactionInfos&& removedTransactionInfos) {
			m_pCommonBlockElement = std::move(pCommonBlockElement);
			m_scoreDelta = std::move(scoreDelta);
			m_removedTransactionInfos = std::move(removedTransactionInfos);
		}

		void commit(Height height) {
			m_pOriginalCurrentCache->commit(height);
			m_pOriginalPreviousCache->commit(height);
			m_pCacheCurrentDelta.reset(); // release the delta after commit so that the UT updater can acquire a lock
			m_pCachePreviousDelta.reset(); // release the delta after commit so that the UT updater can acquire a lock

			*m_pOriginalState = m_stateCopy;
		}

	private:
		cache::CatapultCache* m_pOriginalCurrentCache;
		cache::CatapultCache* m_pOriginalPreviousCache;
		state::CatapultState* m_pOriginalState;
		std::unique_ptr<cache::CatapultCacheDelta> m_pCacheCurrentDelta; // unique_ptr to allow explicit release of lock in commit
		std::unique_ptr<cache::CatapultCacheDelta> m_pCachePreviousDelta; // unique_ptr to allow explicit release of lock in commit
		state::CatapultState m_stateCopy;
		std::shared_ptr<const model::BlockElement> m_pCommonBlockElement;
		model::ChainScore m_scoreDelta;
		consumers::TransactionInfos m_removedTransactionInfos;
		io::BlockStorageCache* m_storage;
	};
}}

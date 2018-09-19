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

#include <src/catapult/config/LocalNodeConfiguration.h>
#include "Harvester.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/chain/BlockScorer.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/model/BlockUtils.h"

namespace catapult { namespace harvesting {

	namespace {
		struct NextBlockContext {
		public:
			explicit NextBlockContext(const model::BlockElement& parentBlockElement, Timestamp nextTimestamp)
					: ParentBlock(parentBlockElement.Block)
					, ParentContext(parentBlockElement)
					, Timestamp(nextTimestamp)
					, Height(ParentBlock.Height + catapult::Height(1))
					, BlockTime(utils::TimeSpan::FromDifference(Timestamp, ParentBlock.Timestamp))
			{}

		public:
			const model::Block& ParentBlock;
			model::PreviousBlockContext ParentContext;
			catapult::Timestamp Timestamp;
			catapult::Height Height;
			utils::TimeSpan BlockTime;
			catapult::Difficulty Difficulty;

		public:
			bool tryCalculateDifficulty(const cache::BlockDifficultyCache& cache, const model::BlockChainConfiguration& config) {
				return chain::TryCalculateDifficulty(cache, ParentBlock.Height, config, Difficulty);
			}
		};

		auto CreateBlock(
				const NextBlockContext& context,
				model::NetworkIdentifier networkIdentifier,
				const crypto::KeyPair& keyPair,
				const TransactionsInfo& info) {
			auto pBlock = model::CreateBlock(context.ParentContext, networkIdentifier, keyPair.publicKey(), info.Transactions);
			pBlock->Difficulty = context.Difficulty;
			pBlock->Timestamp = context.Timestamp;
			pBlock->BlockTransactionsHash = info.TransactionsHash;
			SignBlockHeader(keyPair, *pBlock);
			return pBlock;
		}
	}

	Harvester::Harvester(
			const extensions::LocalNodeStateRef& localNodeState,
			const UnlockedAccounts& unlockedAccounts,
			const TransactionsInfoSupplier& transactionsInfoSupplier)
			: m_localNodeState(localNodeState)
			, m_unlockedAccounts(unlockedAccounts)
			, m_transactionsInfoSupplier(transactionsInfoSupplier)
	{}

	std::unique_ptr<model::Block> Harvester::harvest(const model::BlockElement& lastBlockElement, Timestamp timestamp) {
		NextBlockContext context(lastBlockElement, timestamp);
		auto& cache = m_localNodeState.CurrentCache;
		auto& config = m_localNodeState.Config.BlockChain;
		if (!context.tryCalculateDifficulty(cache.sub<cache::BlockDifficultyCache>(), config)) {
			CATAPULT_LOG(debug) << "skipping harvest attempt due to error calculating difficulty";
			return nullptr;
		}

		chain::BlockHitContext hitContext;
		hitContext.ElapsedTime = context.BlockTime;
		hitContext.Difficulty = context.Difficulty;
		hitContext.Height = context.Height;

		const auto& accountStateCache = cache.sub<cache::AccountStateCache>();
		chain::BlockHitPredicate hitPredicate(config, [&accountStateCache](const auto& key, auto height) {
			auto lockedCacheView = accountStateCache.createView();
			cache::ReadOnlyAccountStateCache readOnlyCache(*lockedCacheView);
			cache::ImportanceView view(readOnlyCache);
			return view.getAccountImportanceOrDefault(key, height);
		});

		auto unlockedAccountsView = m_unlockedAccounts.view();
		const crypto::KeyPair* pHarvesterKeyPair = nullptr;
		for (const auto& keyPair : unlockedAccountsView) {
			hitContext.Signer = keyPair.publicKey();
			hitContext.GenerationHash = model::CalculateGenerationHash(context.ParentContext.GenerationHash, hitContext.Signer);

			if (hitPredicate(hitContext)) {
				pHarvesterKeyPair = &keyPair;
				break;
			}
		}

		if (!pHarvesterKeyPair)
			return nullptr;

		auto transactionsInfo = m_transactionsInfoSupplier(config.MaxTransactionsPerBlock);
		return CreateBlock(context, config.Network.Identifier, *pHarvesterKeyPair, transactionsInfo);
	}
}}

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

#include "catapult/cache_core/BalanceView.h"
#include "catapult/chain/BlockScorer.h"
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/model/BlockUtils.h"
#include "Harvester.h"

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
		};

		auto CreateBlock(
				const NextBlockContext& nextBlockContext,
				const model::BlockHitContext& hitContext,
				model::NetworkIdentifier networkIdentifier,
				const crypto::KeyPair& keyPair,
				const TransactionsInfo& info) {
			auto pBlock = model::CreateBlock(nextBlockContext.ParentContext, hitContext, networkIdentifier, keyPair.publicKey(), info.Transactions);
			pBlock->Timestamp = nextBlockContext.Timestamp;
			pBlock->BlockTransactionsHash = info.TransactionsHash;
			SignBlockHeader(keyPair, *pBlock);
			return pBlock;
		}
	}

	Harvester::Harvester(
			extensions::LocalNodeStateRef localNodeState,
			const UnlockedAccounts& unlockedAccounts,
			const TransactionsInfoSupplier& transactionsInfoSupplier)
			: m_localNodeState(localNodeState)
			, m_unlockedAccounts(unlockedAccounts)
			, m_transactionsInfoSupplier(transactionsInfoSupplier)
	{}

	std::unique_ptr<model::Block> Harvester::harvest(const model::BlockElement& lastBlockElement, Timestamp timestamp) {
		NextBlockContext nextBlockContext(lastBlockElement, timestamp);
		model::BlockHitContext hitContext;
		auto& config = m_localNodeState.Config.BlockChain;
		auto& storage = m_localNodeState.Storage;

		hitContext.ElapsedTime = utils::TimeSpan::FromDifference(timestamp, lastBlockElement.Block.Timestamp);
		utils::TimeSpan averageBlockTime{};
		if (lastBlockElement.Block.Height < Height(Block_Timestamp_History_Size + 2)) {
			averageBlockTime = lastBlockElement.Block.Timestamp / lastBlockElement.Block.Height.unwrap();
		} else {
			auto storageView = storage.view();
			auto pBlock = storageView.loadBlock(Height(lastBlockElement.Block.Height.unwrap() - Block_Timestamp_History_Size));
			averageBlockTime = (lastBlockElement.Block.Timestamp - pBlock->Timestamp) / Block_Timestamp_History_Size;
		}
		hitContext.BaseTarget = chain::CalculateBaseTarget(lastBlockElement.Block.BaseTarget, averageBlockTime, config);
		auto cacheView = m_localNodeState.Cache.createView();
		cache::BalanceView balanceView(cache::ReadOnlyAccountStateCache(cacheView.sub<cache::AccountStateCache>()));

		auto unlockedAccountsView = m_unlockedAccounts.view();
		const crypto::KeyPair* pHarvesterKeyPair = nullptr;
		for (const auto& keyPair : unlockedAccountsView) {
			hitContext.Signer = keyPair.publicKey();
			hitContext.GenerationHash = model::CalculateGenerationHash(
				nextBlockContext.ParentContext.GenerationHash,
				hitContext.Signer
			);

			hitContext.EffectiveBalance = balanceView.getEffectiveBalance(hitContext.Signer);

			chain::BlockHitPredicate hitPredicate;
			if (hitPredicate(hitContext)) {
				pHarvesterKeyPair = &keyPair;
				break;
			}
		}

		if (!pHarvesterKeyPair)
			return nullptr;

		auto transactionsInfo = m_transactionsInfoSupplier(config.MaxTransactionsPerBlock);
		return CreateBlock(nextBlockContext, hitContext, config.Network.Identifier, *pHarvesterKeyPair, transactionsInfo);
	}
}}

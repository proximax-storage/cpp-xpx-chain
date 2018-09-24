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
#include "catapult/cache_core/BalanceView.h"
#include "catapult/chain/BlockScorer.h"
#include "catapult/crypto/KeyPair.h"
#include "catapult/model/BlockUtils.h"

namespace catapult { namespace harvesting {

	namespace {
		auto CreateBlock(
				const model::PreviousBlockContext& previousBlockContext,
				const model::BlockHitContext& hitContext,
				model::NetworkIdentifier networkIdentifier,
				const crypto::KeyPair& keyPair,
				const TransactionsInfo& info) {
			auto pBlock = model::CreateBlock(previousBlockContext, hitContext, networkIdentifier, keyPair.publicKey(), info.Transactions);
			pBlock->Timestamp = previousBlockContext.Timestamp;
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
		model::PreviousBlockContext previousBlockContext(lastBlockElement);
		model::BlockHitContext hitContext;
		auto& config = m_localNodeState.Config.BlockChain;
		auto& storage = m_localNodeState.Storage;

		hitContext.ElapsedTime = utils::TimeSpan::FromDifference(timestamp, lastBlockElement.Block.Timestamp);
		utils::TimeSpan averageBlockTime{};
		if (lastBlockElement.Block.Height < Height(Block_Timestamp_History_Size + 1)) {
			averageBlockTime = lastBlockElement.Block.Timestamp / lastBlockElement.Block.Height.unwrap();
		} else {
			auto storageView = storage.view();
			auto pBlock = storageView.loadBlock(Height(lastBlockElement.Block.Height.unwrap() - Block_Timestamp_History_Size));
			averageBlockTime = (lastBlockElement.Block.Timestamp - pBlock->Timestamp) / Block_Timestamp_History_Size;
		}
		hitContext.BaseTarget = chain::CalculateBaseTarget(lastBlockElement.Block.BaseTarget, averageBlockTime);

		auto currentCacheView = m_localNodeState.CurrentCache.createView();
		auto previousCacheView = m_localNodeState.PreviousCache.createView();

		cache::BalanceView balanceView(
				cache::ReadOnlyAccountStateCache(currentCacheView.sub<cache::AccountStateCache>()),
				cache::ReadOnlyAccountStateCache(previousCacheView.sub<cache::AccountStateCache>()),
				Height(config.EffectiveBalanceRange)
		);
		const auto& currentHeight = storage.view().chainHeight();

		auto unlockedAccountsView = m_unlockedAccounts.view();
		const crypto::KeyPair* pHarvesterKeyPair = nullptr;
		for (const auto& keyPair : unlockedAccountsView) {
			hitContext.Signer = keyPair.publicKey();
			hitContext.GenerationHash = model::CalculateGenerationHash(previousBlockContext.GenerationHash, hitContext.Signer);
			hitContext.EffectiveBalance = balanceView.getEffectiveBalance(hitContext.Signer, currentHeight);

			chain::BlockHitPredicate hitPredicate;
			if (hitPredicate(hitContext)) {
				pHarvesterKeyPair = &keyPair;
				break;
			}
		}

		if (!pHarvesterKeyPair)
			return nullptr;

		auto transactionsInfo = m_transactionsInfoSupplier(config.MaxTransactionsPerBlock);
		return CreateBlock(previousBlockContext, hitContext, config.Network.Identifier, *pHarvesterKeyPair, transactionsInfo);
	}
}}

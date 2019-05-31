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

#include "HarvestingUtFacadeFactory.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/chain/ProcessingNotificationSubscriber.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/model/FeeUtils.h"

namespace catapult { namespace harvesting {

	// region HarvestingUtFacade::Impl

	class HarvestingUtFacade::Impl {
	public:
		Impl(
				Timestamp blockTime,
				const cache::CatapultCache& cache,
				const model::BlockChainConfiguration& blockChainConfig,
				const chain::ExecutionConfiguration& executionConfig)
				: m_blockTime(blockTime)
				, m_blockChainConfig(blockChainConfig)
				, m_executionConfig(executionConfig)
				, m_cacheDetachableDelta(cache.createDetachableDelta())
				, m_cacheDetachedDelta(m_cacheDetachableDelta.detach())
				, m_pCacheDelta(m_cacheDetachedDelta.tryLock())
		{}

	public:
		Height height() const {
			return m_cacheDetachableDelta.height() + Height(1);
		}

	public:
		bool validate(const model::TransactionInfo& transactionInfo) {
			return validate(model::WeakEntityInfo(*transactionInfo.pEntity, transactionInfo.EntityHash));
		}

		std::unique_ptr<model::Block> commit(const model::BlockHeader& blockHeader, const std::vector<model::TransactionInfo>& transactionInfos) {
			// 1. stitch block
			model::Transactions transactions;
			for (const auto& transactionInfo : transactionInfos)
				transactions.push_back(transactionInfo.pEntity);
			auto pBlock = model::StitchBlock(blockHeader, transactions);

			// 2. validate block (using zero hash)
			if (!validate(model::WeakEntityInfo(*pBlock, Hash256())))
				return nullptr;

			// 3. commit transactions
			for (const auto& transactionInfo : transactionInfos)
				commit(model::WeakEntityInfo(*transactionInfo.pEntity, transactionInfo.EntityHash));

			// 4. add back fee surpluses to accounts (skip cache lookup if no surplus)
			auto& accountStateCache = m_pCacheDelta->sub<cache::AccountStateCache>();
			for (const auto& transaction : pBlock->Transactions()) {
				auto surplus = transaction.MaxFee - model::CalculateTransactionFee(blockHeader.FeeMultiplier, transaction);
				if (Amount(0) != surplus)
					accountStateCache.find(transaction.Signer).get().Balances.credit(m_blockChainConfig.CurrencyMosaicId, surplus, blockHeader.Height);
			}

			// 5. execute block (using zero hash)
			commit(model::WeakEntityInfo(*pBlock, Hash256()));

			// 6. update block fields
			pBlock->StateHash = m_blockChainConfig.ShouldEnableVerifiableState
					? m_pCacheDelta->calculateStateHash(height()).StateHash
					: Hash256();

			pBlock->BlockReceiptsHash = m_blockChainConfig.ShouldEnableVerifiableReceipts
					? model::CalculateMerkleHash(*m_blockStatementBuilder.build())
					: Hash256();

			return pBlock;
		}

	private:
		using Processor = predicate<
			const validators::stateful::NotificationValidator&,
			const validators::ValidatorContext&,
			const observers::NotificationObserver&,
			observers::ObserverContext&>;

		bool process(const Processor& processor) {
			// 1. prepare state
			auto catapultState = state::CatapultState();
			catapultState.LastRecalculationHeight = model::ConvertToImportanceHeight(height(), m_blockChainConfig.ImportanceGrouping);
			auto observerState = m_blockChainConfig.ShouldEnableVerifiableReceipts
					? observers::ObserverState(*m_pCacheDelta, catapultState, m_blockStatementBuilder)
					: observers::ObserverState(*m_pCacheDelta, catapultState);

			// 2. prepare contexts
			auto network = m_executionConfig.Network;
			auto readOnlyCache = m_pCacheDelta->toReadOnly();
			auto notifyMode = observers::NotifyMode::Commit;

			auto resolverContext = m_executionConfig.ResolverContextFactory(readOnlyCache);
			auto validatorContext = validators::ValidatorContext(height(), m_blockTime, network, resolverContext, readOnlyCache);
			auto observerContext = observers::ObserverContext(observerState, height(), notifyMode, resolverContext);

			const auto& validator = *m_executionConfig.pValidator;
			const auto& observer = *m_executionConfig.pObserver;
			return processor(validator, validatorContext, observer, observerContext);
		}

		bool validate(const model::WeakEntityInfo& weakEntityInfo) {
			const auto& publisher = *m_executionConfig.pNotificationPublisher;
			return process([&weakEntityInfo, &publisher](
					const auto& validator,
					const auto& validatorContext,
					const auto& observer,
					auto& observerContext) {
				chain::ProcessingNotificationSubscriber sub(validator, validatorContext, observer, observerContext, true, false);

				// execute entity
				publisher.publish(weakEntityInfo, sub);

				return validators::IsValidationResultSuccess(sub.result());
			});
		}

		void commit(const model::WeakEntityInfo& weakEntityInfo) {
			const auto& publisher = *m_executionConfig.pNotificationPublisher;
			process([&weakEntityInfo, &publisher](
					const auto& validator,
					const auto& validatorContext,
					const auto& observer,
					auto& observerContext) {
				chain::ProcessingNotificationSubscriber sub(validator, validatorContext, observer, observerContext, false, true);

				// execute entity
				publisher.publish(weakEntityInfo, sub);

				return true;
			});
		}

	private:
		Timestamp m_blockTime;
		model::BlockChainConfiguration m_blockChainConfig;
		chain::ExecutionConfiguration m_executionConfig;
		cache::CatapultCacheDetachableDelta m_cacheDetachableDelta;
		cache::CatapultCacheDetachedDelta m_cacheDetachedDelta;
		std::unique_ptr<cache::CatapultCacheDelta> m_pCacheDelta;
		model::BlockStatementBuilder m_blockStatementBuilder;
	};

	// endregion

	// region HarvestingUtFacade

	HarvestingUtFacade::HarvestingUtFacade(
			Timestamp blockTime,
			const cache::CatapultCache& cache,
			const model::BlockChainConfiguration& blockChainConfig,
			const chain::ExecutionConfiguration& executionConfig)
			: m_pImpl(std::make_unique<Impl>(blockTime, cache, blockChainConfig, executionConfig))
	{}

	HarvestingUtFacade::~HarvestingUtFacade() = default;

	Height HarvestingUtFacade::height() const {
		return m_pImpl->height();
	}

	size_t HarvestingUtFacade::size() const {
		return m_transactionInfos.size();
	}

	const std::vector<model::TransactionInfo>& HarvestingUtFacade::transactionInfos() const {
		return m_transactionInfos;
	}

	bool HarvestingUtFacade::apply(const model::TransactionInfo& transactionInfo) {
		if (!m_pImpl->validate(transactionInfo))
			return false;

		m_transactionInfos.push_back(transactionInfo.copy());
		return true;
	}

	void HarvestingUtFacade::unapply() {
		if (m_transactionInfos.empty())
			CATAPULT_THROW_OUT_OF_RANGE("cannot call unapply when no transactions have been applied");

		m_transactionInfos.pop_back();
	}

	std::unique_ptr<model::Block> HarvestingUtFacade::commit(const model::BlockHeader& blockHeader) {
		if (height() != blockHeader.Height)
			CATAPULT_THROW_RUNTIME_ERROR("commit block header is inconsistent with facade state");

		auto pBlock = m_pImpl->commit(blockHeader, m_transactionInfos);
		m_transactionInfos.clear();
		return pBlock;
	}

	// endregion

	// region HarvestingUtFacadeFactory

	HarvestingUtFacadeFactory::HarvestingUtFacadeFactory(
			const cache::CatapultCache& cache,
			const model::BlockChainConfiguration& blockChainConfig,
			const chain::ExecutionConfiguration& executionConfig)
			: m_cache(cache)
			, m_blockChainConfig(blockChainConfig)
			, m_executionConfig(executionConfig)
	{}

	std::shared_ptr<HarvestingUtFacade> HarvestingUtFacadeFactory::create(Timestamp blockTime) const {
		return std::make_unique<HarvestingUtFacade>(blockTime, m_cache, m_blockChainConfig, m_executionConfig);
	}

	// endregion
}}

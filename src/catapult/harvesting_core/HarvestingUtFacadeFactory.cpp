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
#include "catapult/model/BlockUtils.h"
#include "catapult/model/FeeUtils.h"

namespace catapult { namespace harvesting {

	// region HarvestingUtFacade::Impl

	class HarvestingUtFacade::Impl {
	public:
		Impl(
			Timestamp blockTime,
			const cache::CatapultCache& cache,
			const chain::ExecutionConfiguration& executionConfig)
			: m_blockTime(blockTime)
			, m_executionConfig(executionConfig)
			, m_cacheDetachableDelta(cache.createDetachableDelta(Height(1)))
			, m_cacheDetachedDelta(m_cacheDetachableDelta.detach())
			, m_pCacheDelta(m_cacheDetachedDelta.tryLock())
		{}

	public:
		Height height() const {
			return m_cacheDetachableDelta.height();
		}

	public:
		bool apply(const model::TransactionInfo& transactionInfo) {
			auto originalSource = m_blockStatementBuilder.source();

			if (apply(model::WeakEntityInfo(*transactionInfo.pEntity, transactionInfo.EntityHash, height())))
				return true;

			auto finalSource = m_blockStatementBuilder.source();
			if (originalSource.PrimaryId != finalSource.PrimaryId)
				m_blockStatementBuilder.popSource();

			return false;
		}

		void unapply() {
			m_pCacheDelta->restoreChanges();
			m_blockStatementBuilder.popSource();
		}

		model::UniqueEntityPtr<model::Block> commit(const model::Block& blockHeader, const model::Transactions& transactions) {
			// 1. stitch block
			auto pBlock = model::StitchBlock(blockHeader, transactions);
			const auto& config = m_executionConfig.ConfigSupplier(blockHeader.Height);
			pBlock->FeeInterest = config.Node.FeeInterest;
			pBlock->FeeInterestDenominator = config.Node.FeeInterestDenominator;

			// 2. add back fee surpluses to accounts (skip cache lookup if no surplus)
			auto& accountStateCache = m_pCacheDelta->sub<cache::AccountStateCache>();
			for (const auto& transaction : pBlock->Transactions()) {
				auto surplus = transaction.MaxFee - m_executionConfig.pTransactionFeeCalculator->calculateTransactionFee(
					blockHeader.FeeMultiplier,
					transaction,
					pBlock->FeeInterest,
					pBlock->FeeInterestDenominator,
					blockHeader.Height);
				if (Amount(0) != surplus)
					accountStateCache.find(transaction.Signer).get().Balances.credit(config.Immutable.CurrencyMosaicId, surplus, blockHeader.Height);
			}

			// 3. execute block (using zero hash)
			if (!apply(model::WeakEntityInfo(*pBlock, Hash256(), blockHeader.Height)))
				return nullptr;

			// 4. update block fields
			pBlock->StateHash = config.Immutable.ShouldEnableVerifiableState
					? m_pCacheDelta->calculateStateHash(blockHeader.Height).StateHash
					: Hash256();

			pBlock->BlockReceiptsHash = config.Immutable.ShouldEnableVerifiableReceipts
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
			const auto& config = m_executionConfig.ConfigSupplier(height());
			catapultState.LastRecalculationHeight = model::ConvertToImportanceHeight(height(), config.Network.ImportanceGrouping);
			std::vector<std::unique_ptr<model::Notification>> notifications;
			auto observerState = config.Immutable.ShouldEnableVerifiableReceipts
					? observers::ObserverState(*m_pCacheDelta, catapultState, m_blockStatementBuilder, notifications)
					: observers::ObserverState(*m_pCacheDelta, catapultState, notifications);

			// 2. prepare contexts
			auto readOnlyCache = m_pCacheDelta->toReadOnly();
			auto notifyMode = observers::NotifyMode::Commit;

			auto resolverContext = m_executionConfig.ResolverContextFactory(readOnlyCache);
			auto validatorContext = validators::ValidatorContext(config, height(), m_blockTime, resolverContext, readOnlyCache);
			auto observerContext = observers::ObserverContext(observerState, config, height(), m_blockTime, notifyMode, resolverContext);

			const auto& validator = *m_executionConfig.pValidator;
			const auto& observer = *m_executionConfig.pObserver;
			return processor(validator, validatorContext, observer, observerContext);
		}

		bool apply(const model::WeakEntityInfo& weakEntityInfo) {
			const auto& publisher = *m_executionConfig.pNotificationPublisher;
			return process([&weakEntityInfo, &publisher](
					const auto& validator,
					const auto& validatorContext,
					const auto& observer,
					auto& observerContext) {
				chain::ProcessingNotificationSubscriber sub(validator, validatorContext, observer, observerContext);
				observerContext.Cache.backupChanges(false);

				// execute entity
				publisher.publish(weakEntityInfo, sub);
				if (validators::IsValidationResultSuccess(sub.result()))
					return true;

				observerContext.Cache.restoreChanges();
				return false;
			});
		}

	private:
		Timestamp m_blockTime;
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
			const chain::ExecutionConfiguration& executionConfig)
			: m_pImpl(std::make_unique<Impl>(blockTime, cache, executionConfig))
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
		if (!m_pImpl->apply(transactionInfo))
			return false;

		m_transactionInfos.push_back(transactionInfo.copy());
		return true;
	}

	void HarvestingUtFacade::unapply() {
		if (m_transactionInfos.empty())
			CATAPULT_THROW_OUT_OF_RANGE("cannot call unapply when no transactions have been applied");

		m_pImpl->unapply();
		m_transactionInfos.pop_back();
	}

	model::UniqueEntityPtr<model::Block> HarvestingUtFacade::commit(const model::Block& blockHeader) {
		if (height() != blockHeader.Height)
			CATAPULT_THROW_RUNTIME_ERROR("commit block header is inconsistent with facade state");

		model::Transactions transactions;
		for (const auto& transactionInfo : m_transactionInfos)
			transactions.push_back(transactionInfo.pEntity);

		auto pBlock = m_pImpl->commit(blockHeader, transactions);
		m_transactionInfos.clear();
		return pBlock;
	}

	// endregion

	// region HarvestingUtFacadeFactory

	HarvestingUtFacadeFactory::HarvestingUtFacadeFactory(
			const cache::CatapultCache& cache,
			const chain::ExecutionConfiguration& executionConfig)
			: m_cache(cache)
			, m_executionConfig(executionConfig)
	{}

	std::unique_ptr<HarvestingUtFacade> HarvestingUtFacadeFactory::create(Timestamp blockTime) const {
		return std::make_unique<HarvestingUtFacade>(blockTime, m_cache, m_executionConfig);
	}

	// endregion
}}

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

#include "PtUpdater.h"
#include "PtValidator.h"
#include "partialtransaction/src/PtUtils.h"
#include "plugins/txes/aggregate/src/model/AggregateTransaction.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/cache_tx/MemoryPtCache.h"
#include "catapult/crypto/Signer.h"
#include "catapult/thread/FutureUtils.h"
#include "catapult/thread/IoThreadPool.h"
#include "catapult/utils/ArraySet.h"
#include <boost/asio.hpp>

namespace catapult { namespace chain {

	namespace {
		using DetachedCosignatures = std::vector<model::DetachedCosignature>;

		std::shared_ptr<const model::AggregateTransaction> RemoveCosignatures(
				const std::shared_ptr<const model::AggregateTransaction>& pAggregateTransaction) {
			// if there are no cosignatures, no need to copy
			if (0 == pAggregateTransaction->CosignaturesCount())
				return pAggregateTransaction;

			// copy the transaction data without cosignatures
			uint32_t truncatedSize = sizeof(model::AggregateTransaction) + pAggregateTransaction->PayloadSize;
			auto pTransactionWithoutCosignatures = utils::MakeSharedWithSize<model::AggregateTransaction>(truncatedSize);
			std::memcpy(static_cast<void*>(pTransactionWithoutCosignatures.get()), pAggregateTransaction.get(), truncatedSize);
			pTransactionWithoutCosignatures->Size = truncatedSize;
			return std::move(pTransactionWithoutCosignatures);
		}

		DetachedCosignatures ExtractCosignatures(
				const model::AggregateTransaction& aggregateTransaction,
				const Hash256& aggregateHash,
				const model::WeakCosignedTransactionInfo& transactionInfoFromCache) {
			utils::KeySet cosigners;
			DetachedCosignatures cosignatures;
			const auto* pCosignature = aggregateTransaction.CosignaturesPtr();
			for (auto i = 0u; i < aggregateTransaction.CosignaturesCount(); ++i) {
				const auto& cosigner = pCosignature->Signer;
				if (cosigners.emplace(cosigner).second && (!transactionInfoFromCache || !transactionInfoFromCache.hasCosigner(cosigner)))
					cosignatures.emplace_back(pCosignature->Signer, pCosignature->Signature, aggregateHash);

				++pCosignature;
			}

			return cosignatures;
		}
	}

	struct StaleTransactionInfo {
		Hash256 AggregateHash;
		std::vector<model::Cosignature> EligibleCosignatures;
	};

	struct CheckEligibilityResult {
	public:
		explicit CheckEligibilityResult(CosignatureUpdateResult updateResult)
				: m_updateResult(updateResult)
				, m_validationResult(CosignersValidationResult::Ineligible)
		{}

		explicit CheckEligibilityResult(CosignersValidationResult validationResult)
				: m_updateResult(CosignatureUpdateResult::Ineligible)
				, m_validationResult(validationResult)
		{}

	public:
		CosignatureUpdateResult updateResult() const {
			return CosignersValidationResult::Failure == m_validationResult ? CosignatureUpdateResult::Error : m_updateResult;
		}

		bool isEligibile() const {
			return CosignersValidationResult::Missing == m_validationResult || CosignersValidationResult::Success == m_validationResult;
		}

		bool isPurgeRequired() const {
			return CosignersValidationResult::Failure == m_validationResult;
		}

		bool isCacheStale() const {
			return !!m_pStaleTransactionInfo;
		}

		const StaleTransactionInfo& staleTransactionInfo() const {
			return *m_pStaleTransactionInfo;
		}

	public:
		void setStaleTransactionInfo(StaleTransactionInfo&& staleTransactionInfo) {
			m_pStaleTransactionInfo = std::make_unique<StaleTransactionInfo>(std::move(staleTransactionInfo));
		}

	private:
		CosignatureUpdateResult m_updateResult;
		CosignersValidationResult m_validationResult;
		std::unique_ptr<StaleTransactionInfo> m_pStaleTransactionInfo; // unique_ptr used as optional
	};

	class PtUpdater::Impl final : public std::enable_shared_from_this<PtUpdater::Impl> {
	public:
		Impl(
				const cache::CatapultCache& cache,
				cache::MemoryPtCacheProxy& transactionsCache,
				std::unique_ptr<const PtValidator>&& pValidator,
				const CompletedTransactionSink& completedTransactionSink,
				const FailedTransactionSink& failedTransactionSink,
				const std::shared_ptr<thread::IoThreadPool>& pPool)
				: m_catapultCache(cache)
				, m_transactionsCache(transactionsCache)
				, m_pValidator(std::move(pValidator))
				, m_completedTransactionSink(completedTransactionSink)
				, m_failedTransactionSink(failedTransactionSink)
				, m_pPool(pPool)
		{}

	private:
		struct TransactionUpdateContext {
			std::shared_ptr<const model::AggregateTransaction> pAggregateTransaction;
			Hash256 AggregateHash;
			DetachedCosignatures Cosignatures;
			std::shared_ptr<const model::UnresolvedAddressSet> pExtractedAddresses;
		};

	public:
		thread::future<TransactionUpdateResult> update(const model::TransactionInfo& transactionInfo) {
			if (model::Entity_Type_Aggregate_Bonded != transactionInfo.pEntity->Type)
				CATAPULT_THROW_INVALID_ARGUMENT("PtUpdater only supports bonded aggregate transactions");

			const auto& aggregateHash = transactionInfo.EntityHash;
			DetachedCosignatures cosignatures;
			auto pAggregateTransaction = std::static_pointer_cast<const model::AggregateTransaction>(transactionInfo.pEntity);
			{
				auto view = m_transactionsCache.view();
				auto transactionInfoFromCache = view.find(aggregateHash);

				cosignatures = ExtractCosignatures(*pAggregateTransaction, aggregateHash, transactionInfoFromCache);

				if (transactionInfoFromCache)
					return update(cosignatures, TransactionUpdateResult::UpdateType::Existing);
			}

			auto pPromise = std::make_shared<thread::promise<TransactionUpdateResult>>(); // needs to be copyable to pass to post
			auto updateFuture = pPromise->get_future();

			TransactionUpdateContext updateContext{
				pAggregateTransaction,
				aggregateHash,
				std::move(cosignatures),
				transactionInfo.OptionalExtractedAddresses
			};
			boost::asio::post(m_pPool->ioContext(), [pThis = shared_from_this(), updateContext, pPromise{std::move(pPromise)}]() {
				pThis->updateImpl(updateContext).then([pPromise](auto&& resultFuture) {
					pPromise->set_value(resultFuture.get());
				});
			});

			return updateFuture;
		}

	private:
		thread::future<TransactionUpdateResult> updateImpl(const TransactionUpdateContext& updateContext) {
			const auto& aggregateHash = updateContext.AggregateHash;
			auto pAggregateTransactionWithoutCosignatures = RemoveCosignatures(updateContext.pAggregateTransaction);
			if (!isValid(*pAggregateTransactionWithoutCosignatures, aggregateHash))
				return thread::make_ready_future(TransactionUpdateResult{ TransactionUpdateResult::UpdateType::Invalid, 0 });

			// notice that the merkle component hash is not stored in the pt cache
			auto transactionInfo = model::DetachedTransactionInfo(pAggregateTransactionWithoutCosignatures, aggregateHash);
			transactionInfo.OptionalExtractedAddresses = updateContext.pExtractedAddresses;
			m_transactionsCache.modifier().add(transactionInfo);

			// if no cosignatures are present, check if the aggregate doesn't require any cosignatures (e.g. 1-of-1)
			if (updateContext.Cosignatures.empty())
				checkCompleteness(aggregateHash);

			return update(updateContext.Cosignatures, TransactionUpdateResult::UpdateType::New);
		}

	public:
		thread::future<CosignatureUpdateResult> update(const model::DetachedCosignature& cosignature) {
			auto pPromise = std::make_shared<thread::promise<CosignatureUpdateResult>>(); // needs to be copyable to pass to post
			auto updateFuture = pPromise->get_future();

			boost::asio::post(m_pPool->ioContext(), [pThis = shared_from_this(), cosignature, pPromise{std::move(pPromise)}]() {
				auto result = pThis->updateImpl(cosignature);
				pPromise->set_value(std::move(result));
			});

			return updateFuture;
		}

	private:
		CosignatureUpdateResult updateImpl(const model::DetachedCosignature& cosignature) {
			auto eligiblityResult = checkEligibility(cosignature);

			// proactively refresh the cache even if the new cosignature is invalid
			if (eligiblityResult.isCacheStale() && !eligiblityResult.isPurgeRequired())
				refreshStaleCacheEntry(eligiblityResult.staleTransactionInfo());

			if (!eligiblityResult.isEligibile()) {
				if (eligiblityResult.isPurgeRequired())
					remove(cosignature.ParentHash);

				return eligiblityResult.updateResult();
			}

			if (!crypto::Verify(cosignature.Signer, cosignature.ParentHash, cosignature.Signature)) {
				CATAPULT_LOG(debug)
						<< "ignoring unverifiable cosignature (signer = " << cosignature.Signer
						<< ", parentHash = " << cosignature.ParentHash << ")";
				return CosignatureUpdateResult::Unverifiable;
			}

			return addCosignature(cosignature);
		}

		thread::future<TransactionUpdateResult> update(
				const DetachedCosignatures& cosignatures,
				TransactionUpdateResult::UpdateType updateType) {
			if (cosignatures.empty())
				return thread::make_ready_future(TransactionUpdateResult{ updateType, 0u });

			std::vector<thread::future<CosignatureUpdateResult>> futures;
			for (const auto& cosignature : cosignatures)
				futures.emplace_back(update(cosignature));

			return thread::when_all(std::move(futures)).then([updateType](auto&& resultsFuture) {
				auto results = resultsFuture.get();
				auto numCosignaturesAdded = std::count_if(results.begin(), results.end(), [](auto& resultFuture) {
					auto result = resultFuture.get();
					return CosignatureUpdateResult::Added_Incomplete == result || CosignatureUpdateResult::Added_Complete == result;
				});

				return TransactionUpdateResult{ updateType, static_cast<size_t>(numCosignaturesAdded) };
			});
		}

		CosignatureUpdateResult addCosignature(const model::DetachedCosignature& cosignature) {
			{
				auto modifier = m_transactionsCache.modifier();
				if (!modifier.add(cosignature.ParentHash, cosignature.Signer, cosignature.Signature))
					return CosignatureUpdateResult::Redundant;
			}

			return checkCompleteness(cosignature.ParentHash);
		}

	private:
		CosignatureUpdateResult checkCompleteness(const Hash256& aggregateHash) {
			std::vector<model::Cosignature> completedCosignatures;
			{
				auto view = m_transactionsCache.view();
				auto transactionInfoFromCache = view.find(aggregateHash);
				if (!transactionInfoFromCache)
					return CosignatureUpdateResult::Redundant;

				if (!isComplete(transactionInfoFromCache))
					return CosignatureUpdateResult::Added_Incomplete;

				completedCosignatures = transactionInfoFromCache.cosignatures();
			}

			// if complete, remove from the cache
			auto removedInfo = remove(aggregateHash);
			if (removedInfo)
				handleComplete({ removedInfo.pEntity.get(), &completedCosignatures });

			return CosignatureUpdateResult::Added_Complete;
		}

		bool isValid(const model::Transaction& transaction, const Hash256& aggregateHash) const {
			auto height = m_catapultCache.height();
			auto result = m_pValidator->validatePartial(model::WeakEntityInfoT<model::Transaction>(transaction, aggregateHash, height));
			if (result.Normalized)
				return true;

			m_failedTransactionSink(transaction, height, aggregateHash, result.Raw);
			return false;
		}

		// checkEligibility has two responsibilities
		// 1. first pass to determine if cosignature is invalid before verifying signature (it could still be rejected later)
		// 2. detect if cache state for corresponding transaction is invalid and needs refreshing
		CheckEligibilityResult checkEligibility(const model::DetachedCosignature& cosignature) const {
			auto view = m_transactionsCache.view();
			auto transactionInfoFromCache = view.find(cosignature.ParentHash);
			if (!transactionInfoFromCache)
				return CheckEligibilityResult(CosignatureUpdateResult::Ineligible);

			if (transactionInfoFromCache.hasCosigner(cosignature.Signer))
				return CheckEligibilityResult(CosignatureUpdateResult::Redundant);

			// optimize for the most likely case that the new cosignature is valid and no existing cosignatures are stale
			auto cosignatures = transactionInfoFromCache.cosignatures();
			cosignatures.push_back({ cosignature.Signer, cosignature.Signature });
			auto validateAllResult = validateCosigners(transactionInfoFromCache, cosignatures);

			if (CosignersValidationResult::Ineligible != validateAllResult.Normalized) {
				// if there was an unexpected error, purge the entire transaction
				// failures are independent of cosignatures, so subsequent validateCosigners calls should never result in failures
				if (CosignersValidationResult::Failure == validateAllResult.Normalized)
					m_failedTransactionSink(transactionInfoFromCache.transaction(), m_catapultCache.height(), cosignature.ParentHash, validateAllResult.Raw);

				return CheckEligibilityResult(validateAllResult.Normalized);
			}

			// at this point, either the new cosignature or an existing cosignature is ineligible
			// 1. check the new cosignature and exit in the more likely case it is ineligible
			std::vector<model::Cosignature> singleElementCosignatures{ cosignature };
			auto validateNewResult = validateCosigners(transactionInfoFromCache, singleElementCosignatures);
			CheckEligibilityResult newCosignatureEligiblityResult(validateNewResult.Normalized);
			if (CosignersValidationResult::Ineligible == validateNewResult.Normalized)
				return newCosignatureEligiblityResult;

			// 2. a state change caused one of the previously accepted cosignatures to be invalid, so reprocess all of them
			CATAPULT_LOG(debug) << "detected stale cosignature for transaction " << cosignature.ParentHash;

			StaleTransactionInfo staleTransactionInfo;
			staleTransactionInfo.AggregateHash = cosignature.ParentHash;

			cosignatures.pop_back(); // only process original cosignatures
			for (const auto& existingCosignature : cosignatures) {
				singleElementCosignatures[0] = existingCosignature;
				auto validateSingleResult = validateCosigners(transactionInfoFromCache, singleElementCosignatures);
				if (CosignersValidationResult::Ineligible == validateSingleResult.Normalized) {
					CATAPULT_LOG(debug)
							<< "detected stale cosignature with signer " << cosignature.Signer
							<< " for transaction " << cosignature.ParentHash;
				} else {
					// cosignature is still valid
					staleTransactionInfo.EligibleCosignatures.push_back(existingCosignature);
				}
			}

			newCosignatureEligiblityResult.setStaleTransactionInfo(std::move(staleTransactionInfo));
			return newCosignatureEligiblityResult;
		}

		void refreshStaleCacheEntry(const StaleTransactionInfo& staleTransactionInfo) {
			// update the cache entry by removing it and then repopulating it
			auto modifier = m_transactionsCache.modifier();
			auto removedInfo = modifier.remove(staleTransactionInfo.AggregateHash);
			if (!removedInfo)
				return;

			modifier.add(removedInfo);
			for (const auto& cosignature : staleTransactionInfo.EligibleCosignatures)
				modifier.add(staleTransactionInfo.AggregateHash, cosignature.Signer, cosignature.Signature);
		}

		PtValidator::Result<CosignersValidationResult> validateCosigners(
				const model::WeakCosignedTransactionInfo& transactionInfo,
				const std::vector<model::Cosignature>& cosignatures) const {
			return m_pValidator->validateCosigners({ &transactionInfo.transaction(), &cosignatures });
		}

		bool isComplete(const model::WeakCosignedTransactionInfo& transactionInfo) const {
			return CosignersValidationResult::Success == m_pValidator->validateCosigners(transactionInfo).Normalized;
		}

		void handleComplete(const model::WeakCosignedTransactionInfo& transactionInfo) {
			m_completedTransactionSink(partialtransaction::StitchAggregate(transactionInfo));
		}

		model::DetachedTransactionInfo remove(const Hash256& aggregateHash) {
			auto modifier = m_transactionsCache.modifier();
			return modifier.remove(aggregateHash);
		}

	private:
		const cache::CatapultCache& m_catapultCache;
		cache::MemoryPtCacheProxy& m_transactionsCache;
		std::unique_ptr<const PtValidator> m_pValidator;
		CompletedTransactionSink m_completedTransactionSink;
		FailedTransactionSink m_failedTransactionSink;
		std::shared_ptr<thread::IoThreadPool> m_pPool;
	};

	PtUpdater::PtUpdater(
			const cache::CatapultCache& cache,
			cache::MemoryPtCacheProxy& transactionsCache,
			std::unique_ptr<const PtValidator>&& pValidator,
			const CompletedTransactionSink& completedTransactionSink,
			const FailedTransactionSink& failedTransactionSink,
			const std::shared_ptr<thread::IoThreadPool>& pPool)
			: m_pImpl(std::make_shared<Impl>(
					cache,
					transactionsCache,
					std::move(pValidator),
					completedTransactionSink,
					failedTransactionSink,
					pPool))
	{}

	PtUpdater::~PtUpdater() = default;

	thread::future<TransactionUpdateResult> PtUpdater::update(const model::TransactionInfo& transactionInfo) {
		return m_pImpl->update(transactionInfo);
	}

	thread::future<CosignatureUpdateResult> PtUpdater::update(const model::DetachedCosignature& cosignature) {
		return m_pImpl->update(cosignature);
	}
}}

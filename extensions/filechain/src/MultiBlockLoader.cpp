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

#include "catapult/cache/CatapultCache.h"
#include "catapult/cache/CacheUtils.h"
#include "catapult/chain/BlockExecutor.h"
#include "catapult/chain/BlockScorer.h"
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/extensions/LocalNodeStateRef.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/model/Block.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "catapult/model/Elements.h"
#include "catapult/utils/StackLogger.h"
#include "MultiBlockLoader.h"

namespace catapult { namespace filechain {

	// region CreateBlockDependentEntityObserverFactory

	namespace {
		class SkipTransientStatePredicate {
		public:
			SkipTransientStatePredicate(const model::Block& lastBlock, const model::BlockChainConfiguration& config)
					: m_inflectionTime(DifferenceOrZero(
							lastBlock.Timestamp.unwrap(),
							model::CalculateTransactionCacheDuration(config).millis()))
					, m_inflectionHeight(DifferenceOrZero(lastBlock.Height.unwrap(), config.MaxDifficultyBlocks) + 1)
			{}

		public:
			bool operator()(const model::Block& block) const {
				// only skip blocks before inflection points
				return m_inflectionTime > block.Timestamp && m_inflectionHeight > block.Height;
			}

		private:
			constexpr static uint64_t DifferenceOrZero(uint64_t lhs, uint64_t rhs) {
				return lhs > rhs ? lhs - rhs : 0;
			}

		private:
			Timestamp m_inflectionTime;
			Height m_inflectionHeight;
		};
	}

	BlockDependentEntityObserverFactory CreateBlockDependentEntityObserverFactory(
			const model::Block& lastBlock,
			const model::BlockChainConfiguration& config,
			const observers::EntityObserver& transientObserver,
			const observers::EntityObserver& permanentObserver) {
		auto predicate = SkipTransientStatePredicate(lastBlock, config);
		return [predicate, &transientObserver, &permanentObserver](const auto& block) -> const observers::EntityObserver& {
			return predicate(block) ? permanentObserver : transientObserver;
		};
	}

	// endregion

	// region LoadBlockChain

	namespace {
		class AnalyzeProgressLogger {
		private:
			static constexpr auto Log_Interval_Millis = 2000;

		public:
			explicit AnalyzeProgressLogger(const utils::StackLogger& stopwatch) : m_stopwatch(stopwatch), m_numLogs(0)
			{}

		public:
			void operator()(Height height, Height chainHeight) {
				auto currentMillis = m_stopwatch.millis();
				if (currentMillis < (m_numLogs + 1) * Log_Interval_Millis)
					return;

				CATAPULT_LOG(info) << "loaded " << height << " / " << chainHeight << " blocks in " << currentMillis << "ms";
				++m_numLogs;
			}

		private:
			const utils::StackLogger& m_stopwatch;
			size_t m_numLogs;
		};
	}

	class BlockChainLoader {
	private:
		using NotifyProgressFunc = consumer<Height, Height>;

	public:
		BlockChainLoader(
				const BlockDependentEntityObserverFactory& observerFactory,
				const extensions::LocalNodeStateRef& stateRef,
				Height startHeight)
				: m_observerFactory(observerFactory)
				, m_stateRef(stateRef)
				, m_startHeight(startHeight)
		{}

	public:
		void loadAll(const NotifyProgressFunc& notifyProgress) const {
			const auto& storage = m_stateRef.Storage.view();

			auto height = m_startHeight;
			auto pParentBlockElement = storage.loadBlockElement(height - Height(1));

			auto chainHeight = storage.chainHeight();
			auto effectiveBalanceHeight = Height(m_stateRef.Config.BlockChain.EffectiveBalanceRange);
			while (chainHeight >= height) {
				auto pBlockElement = storage.loadBlockElement(height);

				// Restore current cache
				executeForCurrentCache(*pBlockElement);

				// Restore cache effectiveBalanceHeight block below
				// We don't need to load nemesis block for previous cache,
				// because initial state of previous cache is state of nemesis block
				if (cache::canUpdatePreviousCache(height, effectiveBalanceHeight)) {
					auto pOldBlockElement = storage.loadBlockElement(height - effectiveBalanceHeight);
					executeForPreviousCache(*pOldBlockElement);
				}

				notifyProgress(height, chainHeight);

				pParentBlockElement = std::move(pBlockElement);
				height = height + Height(1);
			}
		}

	private:

		void executeForCurrentCache(const model::BlockElement& blockElement) const {
			execute(blockElement, m_stateRef.CurrentCache);
		}

		void executeForPreviousCache(const model::BlockElement& blockElement) const {
			execute(blockElement, m_stateRef.PreviousCache);
		}

		void execute(const model::BlockElement& blockElement, cache::CatapultCache& cache) const {
			auto cacheDelta = cache.createDelta();
			auto observerState = observers::ObserverState(cacheDelta);

			const auto& block = blockElement.Block;
			chain::ExecuteBlock(blockElement, m_observerFactory(block), observerState);
			cache.commit(block.Height);
		}

	private:
		const BlockDependentEntityObserverFactory& m_observerFactory;
		const extensions::LocalNodeStateRef& m_stateRef;
		Height m_startHeight;
	};

	void LoadBlockChain(
			const BlockDependentEntityObserverFactory& observerFactory,
			const extensions::LocalNodeStateRef& stateRef,
			Height startHeight) {
		BlockChainLoader loader(observerFactory, stateRef, startHeight);

		utils::StackLogger stopwatch("load block chain", utils::LogLevel::Warning);
		loader.loadAll(AnalyzeProgressLogger(stopwatch));
	}

	// endregion
}}

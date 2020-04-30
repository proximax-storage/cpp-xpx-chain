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

#include "ChainUtils.h"
#include "BlockDifficultyScorer.h"
#include "BlockScorer.h"
#include "catapult/model/BlockUtils.h"

namespace catapult { namespace chain {

	bool IsChainLink(const model::Block& parent, const Hash256& parentHash, const model::Block& child) {
		if (parent.Height + Height(1) != child.Height || parentHash != child.PreviousBlockHash)
			return false;

		return parent.Timestamp < child.Timestamp;
	}

	namespace {
		using DifficultySet = cache::BlockDifficultyCacheTypes::PrimaryTypes::BaseSetType::SetType::MemorySetType;

		void LoadDifficulties(
				DifficultySet& set,
				const cache::BlockDifficultyCache& cache,
				Height height,
				uint32_t numBlocks) {
			auto range = cache.createView(height)->difficultyInfos(height, numBlocks);
			set.insert(range.begin(), range.end());
		}

		auto FindConfig(const model::NetworkConfigurations& remoteConfigs, const Height& height) {
			auto iter = remoteConfigs.lower_bound(height);
			if (iter != remoteConfigs.end() && iter->first != height) {
				if (iter == remoteConfigs.begin())
					return remoteConfigs.end();
				--iter;
			} else if (iter == remoteConfigs.end() && !remoteConfigs.empty()) {
				--iter;
			}

			return iter;
		}
	}

	size_t CheckDifficulties(
			const cache::BlockDifficultyCache& cache,
			const std::vector<const model::Block*>& blocks,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const model::NetworkConfigurations& remoteConfigs) {
		if (blocks.empty())
			return 0;

		DifficultySet difficulties;
		size_t i = 0;
		for (const auto* pBlock : blocks) {
			auto iter = FindConfig(remoteConfigs, pBlock->Height);
			const auto& config = (remoteConfigs.end() != iter) ? iter->second : pConfigHolder->Config(pBlock->Height).Network;
			if (difficulties.size() < config.MaxDifficultyBlocks) {
				auto startHeight = difficulties.empty() ? blocks[0]->Height : difficulties.begin()->BlockHeight;
				if (startHeight > Height(1))
					LoadDifficulties(difficulties, cache, startHeight - Height(1), config.MaxDifficultyBlocks - difficulties.size());
			}

			auto startDifficultyIter = difficulties.cbegin();
			if (difficulties.size() > config.MaxDifficultyBlocks) {
				startDifficultyIter = difficulties.cend();
				for (auto k = 0u; k < config.MaxDifficultyBlocks; ++k)
					--startDifficultyIter;
			}

			auto difficulty = CalculateDifficulty(cache::DifficultyInfoRange(startDifficultyIter, difficulties.cend()), state::BlockDifficultyInfo(*pBlock), config);

			if (difficulty != pBlock->Difficulty)
				break;

			difficulties.insert(state::BlockDifficultyInfo(*pBlock));
			++i;
		}

		if (i != blocks.size()) {
			CATAPULT_LOG(warning)
					<< "difficulties diverge at " << i << " of " << blocks.size()
					<< " (height " << (blocks.front()->Height + Height(i)) << ")";
		}

		return i;
	}

	model::ChainScore CalculatePartialChainScore(const model::Block& parent, const std::vector<const model::Block*>& blocks) {
		model::ChainScore score;
		auto pPreviousBlock = &parent;
		for (const auto* pBlock : blocks) {
			score += model::ChainScore(CalculateScore(*pPreviousBlock, *pBlock));
			pPreviousBlock = pBlock;
		}

		return score;
	}
}}

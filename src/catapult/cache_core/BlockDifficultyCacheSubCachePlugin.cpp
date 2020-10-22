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

#include "BlockDifficultyCacheSubCachePlugin.h"

namespace catapult { namespace cache {

	void BlockDifficultyCacheSummaryCacheStorage::saveSummary(const CatapultCacheDelta& cacheDelta, io::OutputStream& output) const {
		const auto& delta = cacheDelta.sub<BlockDifficultyCache>();
		io::Write(output, delta.height());
		io::Write64(output, delta.size());

		auto pIterableView = delta.tryMakeIterableView();
		for (auto iter = pIterableView->begin(); iter != pIterableView->end();) {
			BlockDifficultyCacheStorage::Save(*iter, output);
			auto prevIter = iter;
			++iter;
			if (iter != pIterableView->end() && prevIter->BlockHeight >= iter->BlockHeight)
				CATAPULT_THROW_RUNTIME_ERROR("next block difficulty height can't be lower");
		}

		output.flush();
	}

	BlockDifficultyCacheSubCachePlugin::BlockDifficultyCacheSubCachePlugin(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder)
			: SubCachePluginAdapter<BlockDifficultyCache, BlockDifficultyCacheStorage>(
					std::make_unique<BlockDifficultyCache>(pConfigHolder))
	{}

	std::unique_ptr<CacheStorage> BlockDifficultyCacheSubCachePlugin::createStorage() {
		return std::make_unique<BlockDifficultyCacheSummaryCacheStorage>(cache());
	}
}}

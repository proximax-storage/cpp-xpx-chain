/**
*** Copyright (c) 2018-present,
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

#include "MockRemoteChainApi.h"
#include "catapult/io/BlockStorageCache.h"
#include "tests/test/core/EntityTestUtils.h"

namespace catapult { namespace mocks {

	namespace {
		catapult_runtime_error CreateHeightException(const char* message, Height height) {
			return catapult_runtime_error(message) << exception_detail::Make<Height>::From(height);
		}
	}

	MockRemoteChainApi::MockRemoteChainApi(const Key& remotePublicKey, const io::BlockStorageCache& storage, const extensions::LocalNodeChainScore& chainScore)
			: RemoteChainApi(remotePublicKey)
			, m_storage(storage)
			, m_chainScore(chainScore.get())
	{}

	thread::future<api::ChainInfo> MockRemoteChainApi::chainInfo() const {
		CATAPULT_LOG(info) << __FUNCTION__;
		auto info = api::ChainInfo();
		auto view = m_storage.view();
		info.Height = view.chainHeight();
		info.Score = m_chainScore.get();
		return thread::make_ready_future(std::move(info));
	}

	thread::future<model::HashRange> MockRemoteChainApi::hashesFrom(Height height, uint32_t maxHashes) const {
		CATAPULT_LOG(info) << __FUNCTION__;
		auto hashes = m_storage.view().loadHashesFrom(height, maxHashes);
		CATAPULT_LOG(info) << __FUNCTION__ << ": number of hashes = " << hashes.size();
		if (hashes.empty()) {
			auto exception = CreateHeightException("unable to get hashes from height", height);
			return thread::make_exceptional_future<model::HashRange>(exception);
		}

		return thread::make_ready_future(std::move(hashes));
	}

	thread::future<std::shared_ptr<const model::Block>> MockRemoteChainApi::blockLast() const {
		CATAPULT_LOG(info) << __FUNCTION__;
		auto view = m_storage.view();
		auto pBlock = view.loadBlock(view.chainHeight());
		return thread::make_ready_future(std::move(pBlock));
	}

	thread::future<std::shared_ptr<const model::Block>> MockRemoteChainApi::blockAt(Height height) const {
		CATAPULT_LOG(info) << __FUNCTION__;
		auto view = m_storage.view();
		auto pBlock = view.loadBlock(height);
		return thread::make_ready_future(std::move(pBlock));
	}

	thread::future<model::BlockRange> MockRemoteChainApi::blocksFrom(Height height, const api::BlocksFromOptions& options) const {
		CATAPULT_LOG(info) << __FUNCTION__;
		auto view = m_storage.view();

		uint32_t maxNumBlocks = view.chainHeight() >= height ? view.chainHeight().unwrap() - height.unwrap() + 1 : 0;
		auto numBlocks = std::min(maxNumBlocks, options.NumBlocks);

		std::vector<std::shared_ptr<const model::Block>> blocks;
		blocks.reserve(numBlocks);
		std::vector<const model::Block*> rawBlocks;
		rawBlocks.reserve(numBlocks);
		for (uint32_t i = 0; i < numBlocks; ++i)
		{
			blocks.push_back(view.loadBlock(Height{height.unwrap() + i}));
			rawBlocks.push_back(blocks[i].get());
		}

		auto range = test::CreateEntityRange(rawBlocks);
		return thread::make_ready_future(std::move(range));
	}

	thread::future<model::EntityRange<model::CacheEntryInfo<Height>>> MockRemoteChainApi::networkConfigs(model::EntityRange<Height>&&) const {
		return thread::make_ready_future(model::EntityRange<model::CacheEntryInfo<Height>>());
	}
}}

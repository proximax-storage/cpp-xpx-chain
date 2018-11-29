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

#pragma once

#include "catapult/api/RemoteChainApi.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/model/Block.h"
#include "catapult/model/RangeTypes.h"

namespace catapult { namespace io { class BlockStorageCache; } }

namespace catapult { namespace mocks {

	class MockRemoteChainApi : public api::RemoteChainApi {
	public:
		explicit MockRemoteChainApi(const io::BlockStorageCache& storage, const extensions::LocalNodeChainScore& chainScore);

	public:
		thread::future<api::ChainInfo> chainInfo() const override;
		thread::future<model::HashRange> hashesFrom(Height height, uint32_t maxHashes) const override;
		thread::future<std::shared_ptr<const model::Block>> blockLast() const override;
		thread::future<std::shared_ptr<const model::Block>> blockAt(Height height) const override;
		thread::future<model::BlockRange> blocksFrom(Height height, const api::BlocksFromOptions& options) const override;

	private:
		const io::BlockStorageCache& m_storage;
		extensions::LocalNodeChainScore m_chainScore;
	};
}}

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

#pragma once
#include "sdk/src/extensions/MemoryBlockStorage.h"
#include "catapult/io/BlockStorageCache.h"
#include "tests/test/nodeps/Nemesis.h"
#include "tests/test/core/BlockTestUtils.h"

namespace catapult { namespace mocks {

	/// A mock memory-based block storage that loads and saves blocks in memory.
	class MockMemoryBlockStorage : public extensions::MemoryBlockStorage {
	public:
		MockMemoryBlockStorage();
		~MockMemoryBlockStorage() override = default;

		/// Creates a mock memory-based block storage.
		template<typename BlockElementGenerator>
		MockMemoryBlockStorage(BlockElementGenerator generator) : MemoryBlockStorage(generator())
	{}
	};

	/// Creates a memory based block storage composed of \a numBlocks.
	std::unique_ptr<io::PrunableBlockStorage> CreateMemoryBlockStorage(uint32_t numBlocks);

	/// Creates a memory based block storage cache composed of \a numBlocks.
	std::unique_ptr<io::BlockStorageCache> CreateMemoryBlockStorageCache(uint32_t numBlocks);

	template<typename TNemesisBlockType>
	model::BlockElement CreateNemesisBlockElement(const TNemesisBlockType& data) {
		return test::BlockToBlockElement(test::GetNemesisBlock(data), test::GetNemesisGenerationHash());
	}
}}

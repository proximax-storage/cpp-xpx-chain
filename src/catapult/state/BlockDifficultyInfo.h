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
#include "catapult/types.h"
#include "catapult/model/Block.h"
#include "CacheDataEntry.h"

namespace catapult { namespace state {

	/// Represents detailed information about a block difficulty
	/// including the block height and the block timestamp.
	struct BlockDifficultyInfo : public CacheDataEntry<BlockDifficultyInfo> {
        static constexpr VersionType MaxVersion{1};

		/// Creates a default block difficulty info.
		BlockDifficultyInfo(VersionType version = 1)
				: BlockDifficultyInfo(Height(0), version)
		{}

		/// Creates a block difficulty info from a \a height.
		explicit BlockDifficultyInfo(Height height, VersionType version = 1)
				: BlockDifficultyInfo(height, Timestamp(0), Difficulty(0), version)
		{}

		/// Creates a block difficulty info from a \a height, a \a timestamp and a \a difficulty.
		explicit BlockDifficultyInfo(Height height, Timestamp timestamp, Difficulty difficulty, VersionType version = 1)
                : CacheDataEntry(version)
				, BlockHeight(height)
				, BlockTimestamp(timestamp)
				, BlockDifficulty(difficulty)
        {}

		/// Creates a block difficulty info from a \a block.
		explicit BlockDifficultyInfo(const model::Block& block, VersionType version = 1)
				: BlockDifficultyInfo(block.Height, block.Timestamp, block.Difficulty, version)
		{}

		/// Block height.
		Height BlockHeight;

		/// Block timestamp.
		Timestamp BlockTimestamp;

		/// Block difficulty.
		Difficulty BlockDifficulty;

		/// Returns \c true if this block difficulty info is less than \a rhs.
		constexpr bool operator<(const BlockDifficultyInfo& rhs) const {
			return BlockHeight < rhs.BlockHeight;
		}

		/// Returns \c true if this block difficulty info is equal to \a rhs.
		constexpr bool operator==(const BlockDifficultyInfo& rhs) const {
			return BlockHeight == rhs.BlockHeight;
		}

		/// Returns \c true if this block difficulty info is not equal to \a rhs.
		constexpr bool operator!=(const BlockDifficultyInfo& rhs) const {
			return BlockHeight != rhs.BlockHeight;
		}
	};
}}

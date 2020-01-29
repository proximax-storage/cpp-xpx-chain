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
#include "catapult/io/PodIoUtils.h"
#include "catapult/io/Stream.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	/// Policy for saving and loading lock info data.
	template<typename TLockInfo, typename TLockInfoSerializer, VersionType version>
	struct LockInfoSerializer;

	/// Policy for saving and loading lock info data.
	template<typename TLockInfo, typename TLockInfoSerializer>
	struct LockInfoSerializer<TLockInfo, TLockInfoSerializer, 1> {
	public:
		static constexpr VersionType Version = 1;

	public:
		/// Saves \a lockInfo to \a output.
		static void Save(const TLockInfo& lockInfo, io::OutputStream& output){
			// write version
			io::Write32(output, Version);

			io::Write(output, lockInfo.Account);
			io::Write(output, lockInfo.Mosaics[0].MosaicId);
			io::Write(output, lockInfo.Mosaics[0].Amount);
			io::Write(output, lockInfo.Height);
			io::Write8(output, utils::to_underlying_type(lockInfo.Status));
			TLockInfoSerializer::Save(lockInfo, output);
		}

		/// Loads a single value from \a input.
		static TLockInfo Load(io::InputStream& input){
			// read version
			VersionType version = io::Read32(input);
			if (version != Version)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid version of LockInfo (expected, actual)", Version, version);

			TLockInfo lockInfo;
			io::Read(input, lockInfo.Account);
			auto mosaicId = MosaicId{io::Read64(input)};
			auto amount = Amount{io::Read64(input)};
			lockInfo.Mosaics.push_back(model::Mosaic{mosaicId,  amount});
			io::Read(input, lockInfo.Height);
			lockInfo.Status = static_cast<LockStatus>(io::Read8(input));
			TLockInfoSerializer::Load(input, lockInfo);
			return lockInfo;
		}
	};

	/// Policy for saving and loading lock info data.
	template<typename TLockInfo, typename TLockInfoSerializer>
	struct LockInfoSerializer<TLockInfo, TLockInfoSerializer, 2> {
	public:
		static constexpr VersionType Version = 2;

	public:
		/// Saves \a lockInfo to \a output.
		static void Save(const TLockInfo& lockInfo, io::OutputStream& output) {
			// write version
			io::Write32(output, Version);

			io::Write(output, lockInfo.Account);
			io::Write(output, lockInfo.Height);
			io::Write8(output, utils::to_underlying_type(lockInfo.Status));
			io::Write8(output, utils::checked_cast<size_t, uint8_t>(lockInfo.Mosaics.size()));
			for (const auto& mosaic : lockInfo.Mosaics) {
				io::Write(output, mosaic.MosaicId);
				io::Write(output, mosaic.Amount);
			}

			TLockInfoSerializer::Save(lockInfo, output);
		}

		/// Loads a single value from \a input.
		static TLockInfo Load(io::InputStream& input) {
			// read version
			VersionType version = io::Read32(input);
			if (version != Version)
				CATAPULT_THROW_RUNTIME_ERROR_2("invalid version of LockInfo (expected, actual)", Version, version);

			TLockInfo lockInfo;
			io::Read(input, lockInfo.Account);
			io::Read(input, lockInfo.Height);
			lockInfo.Status = static_cast<LockStatus>(io::Read8(input));
			auto mosaicCount = io::Read8(input);
			lockInfo.Mosaics.reserve(mosaicCount);
			for (auto i = 0u; i < mosaicCount; ++i) {
				auto mosaicId = MosaicId{io::Read64(input)};
				auto amount = Amount{io::Read64(input)};
				lockInfo.Mosaics.push_back(model::Mosaic{mosaicId,  amount});
			}
			TLockInfoSerializer::Load(input, lockInfo);
			return lockInfo;
		}
	};
}}

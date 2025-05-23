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
#include "mongo/src/mappers/MapperInclude.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/lock_shared/src/state/LockInfo.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace mongo { namespace plugins {

	template<VersionType version>
	class LockInfoStreamer;

	template<>
	class LockInfoStreamer<1> {
	public:
		static void StreamLockInfo(
				mappers::bson_stream::document& builder,
				const state::LockInfo& lockInfo,
				const Address& accountAddress) {
			using namespace catapult::mongo::mappers;

			builder
					<< "account" << ToBinary(lockInfo.Account)
					<< "accountAddress" << ToBinary(accountAddress)
					<< "mosaicId" << ToInt64(lockInfo.Mosaics.begin()->first)
					<< "amount" << ToInt64(lockInfo.Mosaics.begin()->second)
					<< "height" << ToInt64(lockInfo.Height)
					<< "status" << utils::to_underlying_type(lockInfo.Status);
		}

		static void ReadLockInfo(state::LockInfo& lockInfo, const bsoncxx::document::element dbLockInfo) {
			using namespace catapult::mongo::mappers;

			DbBinaryToModelArray(lockInfo.Account, dbLockInfo["account"].get_binary());
			auto mosaicId = GetValue64<MosaicId>(dbLockInfo["mosaicId"]);
			auto amount = GetValue64<Amount>(dbLockInfo["amount"]);
			lockInfo.Mosaics.emplace(mosaicId, amount);
			lockInfo.Height = GetValue64<Height>(dbLockInfo["height"]);
			lockInfo.Status = static_cast<state::LockStatus>(ToUint8(dbLockInfo["status"].get_int32()));
		}
	};

	template<>
	class LockInfoStreamer<2> {
	public:
		static void StreamLockInfo(
				mappers::bson_stream::document& builder,
				const state::LockInfo& lockInfo,
				const Address& accountAddress) {
			using namespace catapult::mongo::mappers;

			builder
					<< "account" << ToBinary(lockInfo.Account)
					<< "accountAddress" << ToBinary(accountAddress)
					<< "height" << ToInt64(lockInfo.Height)
					<< "status" << utils::to_underlying_type(lockInfo.Status);

			auto array = builder << "mosaics" << bson_stream::open_array;
			for (const auto& pair : lockInfo.Mosaics) {
				array
						<< bson_stream::open_document
						<< "mosaicId" << ToInt64(pair.first)
						<< "amount" << ToInt64(pair.second)
						<< bson_stream::close_document;
			}
			array << bson_stream::close_array;
		}

		static void ReadLockInfo(state::LockInfo& lockInfo, const bsoncxx::document::element dbLockInfo) {
			using namespace catapult::mongo::mappers;

			DbBinaryToModelArray(lockInfo.Account, dbLockInfo["account"].get_binary());
			lockInfo.Height = GetValue64<Height>(dbLockInfo["height"]);
			lockInfo.Status = static_cast<state::LockStatus>(ToUint8(dbLockInfo["status"].get_int32()));

			for (const auto& dbMosaic : dbLockInfo["mosaics"].get_array().value) {
				auto mosaicId = GetValue64<MosaicId>(dbMosaic["mosaicId"]);
				auto amount = GetValue64<Amount>(dbMosaic["amount"]);
				lockInfo.Mosaics.emplace(mosaicId, amount);
			}
		}
	};

	/// Traits based lock info mapper.
	template<typename TTraits>
	class LockInfoMapper {
	private:
		using LockInfoType = typename TTraits::LockInfoType;

		// region ToDbModel

	private:
		static void StreamLockMetadata(mappers::bson_stream::document& builder) {
			builder
					<< "meta"
					<< mappers::bson_stream::open_document
					<< mappers::bson_stream::close_document;
		}

	public:
		static bsoncxx::document::value ToDbModel(const LockInfoType& lockInfo, const Address& accountAddress) {
			// lock metadata
			mappers::bson_stream::document builder;
			StreamLockMetadata(builder);

			// lock data
			auto doc = builder << TTraits::Doc_Name << mappers::bson_stream::open_document;
			LockInfoStreamer<TTraits::Version>::StreamLockInfo(builder, lockInfo, accountAddress);
			TTraits::StreamLockInfo(builder, lockInfo);
			return doc
					<< mappers::bson_stream::close_document
					<< mappers::bson_stream::finalize;
		}

		// endregion

		// region ToModel

	public:
		static void ToLockInfo(const bsoncxx::document::view& document, LockInfoType& lockInfo) {
			auto dbLockInfo = document[TTraits::Doc_Name];
			LockInfoStreamer<TTraits::Version>::ReadLockInfo(lockInfo, dbLockInfo);
			TTraits::ReadLockInfo(lockInfo, dbLockInfo);
		}

		// endregion
	};
}}}

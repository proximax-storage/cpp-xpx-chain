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

#include "ContractEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/utils/Casting.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamPublicKeys(bson_stream::document& builder, const std::string& keySetName, const utils::SortedKeySet& keys) {
			auto keyArray = builder << keySetName << bson_stream::open_array;
			for (const auto& key : keys)
				keyArray << ToBinary(key);

			keyArray << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::ContractEntry& entry, const Address& accountAddress) {
		bson_stream::document builder;
		auto doc = builder << "contract" << bson_stream::open_document
				<< "multisig" << ToBinary(entry.key())
				<< "multisigAddress" << ToBinary(accountAddress)
				<< "start" << ToInt64(entry.start())
				<< "duration" << ToInt64(entry.duration())
				<< "hash" << ToBinary(entry.hash());

		StreamPublicKeys(builder, "customers", entry.customers());
		StreamPublicKeys(builder, "executors", entry.executors());
		StreamPublicKeys(builder, "verifiers", entry.verifiers());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadKeySet(utils::SortedKeySet& keySet, const bsoncxx::array::view& dbKeys) {
			for (const auto& dbKey : dbKeys) {
				Key key;
				DbBinaryToModelArray(key, dbKey.get_binary());
				keySet.emplace(key);
			}
		}
	}

	state::ContractEntry ToContractEntry(const bsoncxx::document::view& document) {
		auto dbContractEntry = document["contract"];
		Key multisig;
		DbBinaryToModelArray(multisig, dbContractEntry["multisig"].get_binary());
		state::ContractEntry entry(multisig);

		auto start = static_cast<uint64_t>(dbContractEntry["start"].get_int64());
		entry.setStart(Height{start});
		auto duration = static_cast<uint64_t>(dbContractEntry["duration"].get_int64());
		entry.setDuration(BlockDuration{duration});
		Hash256 hash;
		DbBinaryToModelArray(hash, dbContractEntry["hash"].get_binary());
		entry.setHash(hash);

		ReadKeySet(entry.customers(), dbContractEntry["customers"].get_array().value);
		ReadKeySet(entry.executors(), dbContractEntry["executors"].get_array().value);
		ReadKeySet(entry.verifiers(), dbContractEntry["verifiers"].get_array().value);

		return entry;
	}

	// endregion
}}}

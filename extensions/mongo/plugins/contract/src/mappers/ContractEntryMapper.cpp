/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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

		void StreamHashes(bson_stream::document& builder, const std::vector<model::HashSnapshot>& hashes) {
			auto keyArray = builder << "hashes" << bson_stream::open_array;
			for (const auto& hashSnapshot : hashes) {
				keyArray
						<< bson_stream::open_document
						<< "hash" << ToBinary(hashSnapshot.Hash)
						<< "height" << ToInt64(hashSnapshot.HashHeight)
						<< bson_stream::close_document;
			}

			keyArray << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::ContractEntry& entry, const Address& accountAddress) {
		bson_stream::document builder;
		auto doc = builder << "contract" << bson_stream::open_document
                << "version" << static_cast<int32_t>(entry.getVersion())
				<< "multisig" << ToBinary(entry.key())
				<< "multisigAddress" << ToBinary(accountAddress)
				<< "start" << ToInt64(entry.start())
				<< "duration" << ToInt64(entry.duration())
				<< "hash" << ToBinary(entry.hash());

		StreamHashes(builder, entry.hashes());
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

		void ReadHashes(state::ContractEntry& entry, const bsoncxx::array::view& dbHashes) {
			for (const auto& dbHash : dbHashes) {
				auto snapshot = dbHash.get_document().view();
				Hash256 hash;
				DbBinaryToModelArray(hash, snapshot["hash"].get_binary());
				entry.pushHash(hash, GetValue64<Height>(snapshot["height"]));
			}
		}
	}

	state::ContractEntry ToContractEntry(const bsoncxx::document::view& document) {
		auto dbContractEntry = document["contract"];
        VersionType version = ToUint32(dbContractEntry["version"].get_int32());
		Key multisig;
		DbBinaryToModelArray(multisig, dbContractEntry["multisig"].get_binary());
		state::ContractEntry entry(multisig, version);

		auto start = static_cast<uint64_t>(dbContractEntry["start"].get_int64());
		entry.setStart(Height{start});
		auto duration = static_cast<uint64_t>(dbContractEntry["duration"].get_int64());
		entry.setDuration(BlockDuration{duration});
		Hash256 hash;
		DbBinaryToModelArray(hash, dbContractEntry["hash"].get_binary());

		ReadHashes(entry, dbContractEntry["hashes"].get_array().value);
		ReadKeySet(entry.customers(), dbContractEntry["customers"].get_array().value);
		ReadKeySet(entry.executors(), dbContractEntry["executors"].get_array().value);
		ReadKeySet(entry.verifiers(), dbContractEntry["verifiers"].get_array().value);

		return entry;
	}

	// endregion
}}}

/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BcDriveEntryMapper.h"
#include "catapult/utils/Casting.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamActiveDataModifications(bson_stream::document& builder, const std::vector<Hash256>& activeDataModifications) {
			auto array = builder << "activeDataModifications" << bson_stream::open_array;
			for (const auto& hash : activeDataModifications)
				array << ToBinary(hash);

			array << bson_stream::close_array;
		}

		void StreamCompletedDataModifications(bson_stream::document& builder, const std::vector<std::pair<Hash256, state::DataModificationState>>& completedDataModifications) {
			auto array = builder << "completedDataModifications" << bson_stream::open_array;
			for (const auto& pair : completedDataModifications) {
				array
						<< bson_stream::open_document
						<< "hash" << ToBinary(pair.first)
						<< "state" << utils::to_underlying_type(pair.second)
						<< bson_stream::close_document;
			}

			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::BcDriveEntry& entry, const Address& accountAddress) {
		bson_stream::document builder;
		auto doc = builder << "drive" << bson_stream::open_document
				<< "multisig" << ToBinary(entry.key())
				<< "multisigAddress" << ToBinary(accountAddress)
				<< "owner" << ToBinary(entry.owner())
				<< "rootHash" << ToBinary(entry.rootHash())
				<< "size" << static_cast<int64_t>(entry.size())
				<< "replicatorCount" << static_cast<int32_t>(entry.replicatorCount());

		StreamActiveDataModifications(builder, entry.activeDataModifications());
		StreamCompletedDataModifications(builder, entry.completedDataModifications());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadActiveDataModifications(std::vector<Hash256>& activeDataModifications, const bsoncxx::array::view& dbActiveDataModifications) {
			for (const auto& dbHash : dbActiveDataModifications) {
				auto doc = dbHash.get_binary();

				Hash256 hash;
				DbBinaryToModelArray(hash, doc);

				activeDataModifications.push_back(hash);
			}
		}

		void ReadCompletedDataModifications(std::vector<std::pair<Hash256, state::DataModificationState>>& completedDataModifications, const bsoncxx::array::view& dbCompletedDataModifications) {
			for (const auto& dbPair : dbCompletedDataModifications) {
				auto doc = dbPair.get_document().view();

				Hash256 hash;
				DbBinaryToModelArray(hash, doc["hash"].get_binary());
				auto state = static_cast<state::DataModificationState>(static_cast<uint8_t>(dbPair["state"].get_int32()));

				completedDataModifications.emplace_back(hash, state);
			}
		}
	}

	state::BcDriveEntry ToDriveEntry(const bsoncxx::document::view& document) {

		auto dbDriveEntry = document["drive"];

		Key multisig;
		DbBinaryToModelArray(multisig, dbDriveEntry["multisig"].get_binary());
		state::BcDriveEntry entry(multisig);

		Key owner;
		DbBinaryToModelArray(owner, dbDriveEntry["owner"].get_binary());
		entry.setOwner(owner);

		Hash256 rootHash;
		DbBinaryToModelArray(rootHash, dbDriveEntry["rootHash"].get_binary());
		entry.setRootHash(rootHash);

		entry.setSize(static_cast<uint64_t>(dbDriveEntry["size"].get_int64()));
		entry.setReplicatorCount(static_cast<uint16_t>(dbDriveEntry["replicatorCount"].get_int32()));

		ReadActiveDataModifications(entry.activeDataModifications(), dbDriveEntry["activeDataModifications"].get_array().value);
		ReadCompletedDataModifications(entry.completedDataModifications(), dbDriveEntry["completedDataModifications"].get_array().value);

		return entry;
	}

	// endregion
}}}

/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamContractorDepositMap(bson_stream::document& builder, const Key& key, const state::DepositMap& depositMap) {
			builder << "key" << ToBinary(key);
			auto array = builder << "depositMap" << bson_stream::open_array;
			for (const auto& pair : depositMap) {
				array
						<< "deposit" << bson_stream::open_document
						<< "hash" << ToBinary(pair.first)
						<< "mosaicId" << ToInt64(pair.second.MosaicId)
						<< "amount" << ToInt64(pair.second.Amount)
						<< bson_stream::close_document;
			}

			array << bson_stream::close_array;
		}

		void StreamContractor(bson_stream::document& builder, const std::string& keySetName, const state::ContractorDepositMap& contractorMap) {
			auto array = builder << keySetName << bson_stream::open_array;
			for (const auto& pair : contractorMap) {
				bson_stream::document depositBuilder;
				StreamContractorDepositMap(depositBuilder, pair.first, pair.second);
				array << depositBuilder;
			}

			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::DriveEntry& entry, const Address& accountAddress) {
		bson_stream::document builder;
		auto doc = builder << "drive" << bson_stream::open_document
				<< "multisig" << ToBinary(entry.key())
				<< "multisigAddress" << ToBinary(accountAddress)
				<< "start" << ToInt64(entry.start())
				<< "duration" << ToInt64(entry.duration())
				<< "size" << static_cast<int64_t>(entry.size())
				<< "replicas" << entry.replicas();

		StreamContractor(builder, "customers", entry.customers());
		StreamContractor(builder, "replicators", entry.replicators());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadDepositMap(state::DepositMap& depositMap, const bsoncxx::array::view& dbDepositMap) {
			for (const auto& dbDeposit : dbDepositMap) {
				auto doc = dbDeposit.get_document().view();
				Hash256 hash;
				DbBinaryToModelArray(hash, doc["hash"].get_binary());
				auto mosaicId = UnresolvedMosaicId{static_cast<uint64_t>(doc["mosaicId"].get_int64())};
				auto amount = Amount{static_cast<uint64_t>(doc["amount"].get_int64())};
				depositMap.emplace(hash, model::UnresolvedMosaic{mosaicId, amount});
			}
		}

		void ReadContractor(state::ContractorDepositMap& contractorDepositMap, const bsoncxx::array::view& dbContractorMap) {
			for (const auto& dbDepositMap : dbContractorMap) {
				auto doc = dbDepositMap.get_document().view();
				Key key;
				DbBinaryToModelArray(key, doc["key"].get_binary());
				state::DepositMap depositMap;
				ReadDepositMap(depositMap, doc["depositMap"].get_array().value);
				contractorDepositMap.emplace(key, depositMap);
			}
		}
	}

	state::DriveEntry ToDriveEntry(const bsoncxx::document::view& document) {
		auto dbDriveEntry = document["drive"];
		Key multisig;
		DbBinaryToModelArray(multisig, dbDriveEntry["multisig"].get_binary());
		state::DriveEntry entry(multisig);

		auto start = static_cast<uint64_t>(dbDriveEntry["start"].get_int64());
		entry.setStart(Height{start});
		auto duration = static_cast<uint64_t>(dbDriveEntry["duration"].get_int64());
		entry.setDuration(BlockDuration{duration});
		auto size = static_cast<uint64_t>(dbDriveEntry["size"].get_int64());
		entry.setSize(size);
		auto replicas = static_cast<uint64_t>(dbDriveEntry["replicas"].get_int64());
		entry.setReplicas(replicas);

		ReadContractor(entry.customers(), dbDriveEntry["customers"].get_array().value);
		ReadContractor(entry.replicators(), dbDriveEntry["replicators"].get_array().value);

		return entry;
	}

	// endregion
}}}

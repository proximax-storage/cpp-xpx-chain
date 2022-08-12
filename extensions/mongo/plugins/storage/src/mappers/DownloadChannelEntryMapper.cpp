/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadChannelEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {

		void StreamListOfPublicKeys(bson_stream::document& builder, const std::vector<Key>& listOfPublicKeys) {
			auto array = builder << "listOfPublicKeys" << bson_stream::open_array;
			for (const auto& key : listOfPublicKeys)
				array << ToBinary(key);

			array << bson_stream::close_array;
		}

		void StreamShardReplicators(bson_stream::document& builder, const utils::SortedKeySet& shardReplicators) {
			auto array = builder << "shardReplicators" << bson_stream::open_array;
			for (const auto& replicatorKey : shardReplicators)
				array << ToBinary(replicatorKey);
			array << bson_stream::close_array;
		}

		void StreamCumulativePayments(bson_stream::document& builder, const std::map<Key, Amount>& cumulativePayments) {
			auto array = builder << "cumulativePayments" << bson_stream::open_array;
			for (const auto& pair : cumulativePayments) {
				bson_stream::document paymentBuilder;
				paymentBuilder
						<< "replicator" << ToBinary(pair.first)
						<< "payment" << ToInt64(pair.second);
				array << paymentBuilder;
			}
			array << bson_stream::close_array;
		}

	}

	bsoncxx::document::value ToDbModel(const state::DownloadChannelEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "downloadChannelInfo" << bson_stream::open_document
				<< "id" << ToBinary(entry.id())
				<< "consumer" << ToBinary(entry.consumer())
				<< "drive" << ToBinary(entry.drive())
				<< "downloadSizeMegabytes" << static_cast<int64_t>(entry.downloadSize())
				<< "downloadApprovalCount" << static_cast<int16_t>(entry.downloadApprovalCountLeft())
				<< "finished" << entry.isFinishPublished();

		StreamListOfPublicKeys(builder, entry.listOfPublicKeys());
		StreamShardReplicators(builder, entry.shardReplicators());
		StreamCumulativePayments(builder, entry.cumulativePayments());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {

		void ReadListOfPublicKeys(std::vector<Key>& listOfPublicKeys, const bsoncxx::array::view& dbListOfPublicKeys) {
			for (const auto& dbKey : dbListOfPublicKeys) {
				auto doc = dbKey.get_binary();

				Key key;
				DbBinaryToModelArray(key, doc);

				listOfPublicKeys.push_back(key);
			}
		}

		void ReadShardReplicators(utils::SortedKeySet& shardReplicators, const bsoncxx::array::view& dbShardReplicators) {
			for (const auto& dbReplicator : dbShardReplicators) {
				Key replicatorKey;
				DbBinaryToModelArray(replicatorKey, dbReplicator.get_binary());
				shardReplicators.emplace(std::move(replicatorKey));
			}
		}

		void ReadCumulativePayments(std::map<Key, Amount>& cumulativePayments, const bsoncxx::array::view& dbCumulativePayments) {
			for (const auto& dbPair : dbCumulativePayments) {
				auto doc = dbPair.get_document().view();

				Key replicator;
				DbBinaryToModelArray(replicator, doc["replicator"].get_binary());

				cumulativePayments[replicator] = Amount(static_cast<uint64_t>(dbPair["payment"].get_int64()));
			}
		}

	}

	state::DownloadChannelEntry ToDownloadChannelEntry(const bsoncxx::document::view& document) {

		auto dbDownloadChannelEntry = document["downloadChannelInfo"];

		Hash256 id;
		DbBinaryToModelArray(id, dbDownloadChannelEntry["id"].get_binary());
		state::DownloadChannelEntry entry(id);

		Key consumer;
		DbBinaryToModelArray(consumer, dbDownloadChannelEntry["consumer"].get_binary());
		entry.setConsumer(consumer);

		Key drive;
		DbBinaryToModelArray(drive, dbDownloadChannelEntry["drive"].get_binary());
		entry.setDrive(drive);

		entry.setDownloadSize(static_cast<uint64_t>(dbDownloadChannelEntry["downloadSizeMegabytes"].get_int64()));
		entry.setDownloadApprovalCountLeft(static_cast<uint16_t>(dbDownloadChannelEntry["downloadApprovalCount"].get_int32()));
		entry.setFinishPublished(dbDownloadChannelEntry["finished"].get_bool().value);

		ReadListOfPublicKeys(entry.listOfPublicKeys(), dbDownloadChannelEntry["listOfPublicKeys"].get_array().value);
		ReadShardReplicators(entry.shardReplicators(), dbDownloadChannelEntry["shardReplicators"].get_array().value);
		ReadCumulativePayments(entry.cumulativePayments(), dbDownloadChannelEntry["cumulativePayments"].get_array().value);

		return entry;
	}

	// endregion
}}}

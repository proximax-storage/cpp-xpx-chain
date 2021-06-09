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

	}

	bsoncxx::document::value ToDbModel(const state::DownloadChannelEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "downloadChannelInfo" << bson_stream::open_document
				<< "id" << ToBinary(entry.id())
				<< "consumer" << ToBinary(entry.consumer())
				<< "feedbackFeeAmount" << ToInt64(entry.feedbackFeeAmount())
				<< "downloadSize" << static_cast<int64_t>(entry.downloadSize());

		StreamListOfPublicKeys(builder, entry.listOfPublicKeys());

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

	}

	state::DownloadChannelEntry ToDownloadChannelEntry(const bsoncxx::document::view& document) {

		auto dbDownloadChannelEntry = document["downloadChannelInfo"];

		Hash256 id;
		DbBinaryToModelArray(id, dbDownloadChannelEntry["id"].get_binary());
		state::DownloadChannelEntry entry(id);

		Key consumer;
		DbBinaryToModelArray(consumer, dbDownloadChannelEntry["consumer"].get_binary());
		entry.setConsumer(consumer);

		entry.setFeedbackFeeAmount(Amount(dbDownloadChannelEntry["feedbackFeeAmount"].get_int64()));
		entry.setDownloadSize(static_cast<uint64_t>(dbDownloadChannelEntry["downloadSize"].get_int64()));

		ReadListOfPublicKeys(entry.listOfPublicKeys(), dbDownloadChannelEntry["listOfPublicKeys"].get_array().value);

		return entry;
	}

	// endregion
}}}

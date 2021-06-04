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

		void StreamWhitelistedPublicKeys(bson_stream::document& builder, const std::vector<Key>& whitelistedPublicKeys) {
			auto array = builder << "whitelistedPublicKeys" << bson_stream::open_array;
			for (const auto& key : whitelistedPublicKeys)
				array << ToBinary(key);

			array << bson_stream::close_array;
		}

	}

	bsoncxx::document::value ToDbModel(const state::DownloadChannelEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "downloadChannelInfo" << bson_stream::open_document
				<< "downloadChannelId" << ToBinary(entry.id())
				<< "consumer" << ToBinary(entry.consumer())
				<< "feedbackFeeAmount" << ToInt64(entry.feedbackFeeAmount())
				<< "downloadSize" << static_cast<int64_t>(entry.downloadSize());

		StreamWhitelistedPublicKeys(builder, entry.whitelistedPublicKeys());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {

		void ReadWhitelistedPublicKeys(std::vector<Key>& whitelistedPublicKeys, const bsoncxx::array::view& dbWhitelistedPublicKeys) {
			for (const auto& dbKey : dbWhitelistedPublicKeys) {
				auto doc = dbKey.get_binary();

				Key key;
				DbBinaryToModelArray(key, doc);

				whitelistedPublicKeys.push_back(key);
			}
		}

	}

	state::DownloadChannelEntry ToDownloadChannelEntry(const bsoncxx::document::view& document) {

		auto dbDownloadChannelEntry = document["downloadChannelInfo"];

		Hash256 id;
		DbBinaryToModelArray(id, dbDownloadChannelEntry["downloadChannelId"].get_binary());
		state::DownloadChannelEntry entry(id);

		Key consumer;
		DbBinaryToModelArray(consumer, dbDownloadChannelEntry["consumer"].get_binary());
		entry.setConsumer(consumer);

		entry.setFeedbackFeeAmount(Amount(dbDownloadChannelEntry["feedbackFeeAmount"].get_int64()));
		entry.setDownloadSize(static_cast<uint64_t>(dbDownloadChannelEntry["downloadSize"].get_int64()));

		ReadWhitelistedPublicKeys(entry.whitelistedPublicKeys(), dbDownloadChannelEntry["whitelistedPublicKeys"].get_array().value);

		return entry;
	}

	// endregion
}}}

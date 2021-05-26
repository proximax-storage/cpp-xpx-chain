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

	bsoncxx::document::value ToDbModel(const state::DownloadChannelEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "downloadChannelInfo" << bson_stream::open_document
				<< "downloadChannelId" << ToBinary(entry.id())
				<< "consumer" << ToBinary(entry.consumer())
				<< "driveKey" << ToBinary(entry.drive())
				<< "transactionFee" << ToInt64(entry.transactionFee())
				<< "downloadSize" << static_cast<int64_t>(entry.downloadSize());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::DownloadChannelEntry ToDownloadChannelEntry(const bsoncxx::document::view& document) {
		auto dbDownloadChannelEntry = document["downloadChannelInfo"];
		Hash256 id;
		DbBinaryToModelArray(id, dbDownloadChannelEntry["downloadChannelId"].get_binary());
		state::DownloadChannelEntry entry(id);
		Key consumer;
		DbBinaryToModelArray(consumer, dbDownloadChannelEntry["consumer"].get_binary());
		entry.setConsumer(consumer);
		Key driveKey;
		DbBinaryToModelArray(driveKey, dbDownloadChannelEntry["driveKey"].get_binary());
		entry.setDrive(driveKey);
		entry.setTransactionFee(Amount(dbDownloadChannelEntry["transactionFee"].get_int64()));
		entry.setDownloadSize(static_cast<uint64_t>(dbDownloadChannelEntry["downloadSize"].get_int64()));

		return entry;
	}

	// endregion
}}}

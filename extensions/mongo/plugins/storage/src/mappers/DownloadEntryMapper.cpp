/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	bsoncxx::document::value ToDbModel(const state::DownloadChannelEntry& entry, const Hash256& id) {
		bson_stream::document builder;
		auto doc = builder << "downloadChannelInfo" << bson_stream::open_document
				<< "id" << ToBinary(entry.id())
				<< "consumer" << ToBinary(entry.consumer())
				<< "driveKey" << ToBinary(entry.drive())
				<< "transactionFee" << ToInt64(entry.transactionFee())
				<< "storageUnits" << ToInt64(entry.storageUnits());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	state::DownloadChannelEntry ToDownloadEntry(const bsoncxx::document::view& document) {
		auto dbDownloadEntry = document["downloadChannelInfo"];
		Hash256 id;
		DbBinaryToModelArray(id, dbDownloadEntry["id"].get_binary());
		state::DownloadChannelEntry entry(id);
		Key consumer;
		DbBinaryToModelArray(consumer, dbDownloadEntry["consumer"].get_binary());
		entry.setConsumer(consumer);
		Key driveKey;
		DbBinaryToModelArray(driveKey, dbDownloadEntry["driveKey"].get_binary());
		entry.setDrive(driveKey);
		entry.setTransactionFee(Amount(dbDownloadEntry["transactionFee"].get_int64()));
		entry.setStorageUnits(Amount(dbDownloadEntry["storageUnits"].get_int64()));

		return entry;
	}

	// endregion
}}}

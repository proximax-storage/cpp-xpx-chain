/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {
		void StreamFiles(bson_stream::document& builder, const std::map<Hash256, uint64_t>& files) {
			auto array = builder << "files" << bson_stream::open_array;
			for (const auto& pair : files) {
				bson_stream::document fileBuilder;
				fileBuilder
					<< "fileHash" << ToBinary(pair.first)
					<< "fileSize" << static_cast<int64_t>(pair.second);
				array << fileBuilder;
			}

			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::DownloadEntry& entry, const Address& driveAddress) {
		bson_stream::document builder;
		auto doc = builder << "downloadInfo" << bson_stream::open_document
				<< "operationToken" << ToBinary(entry.OperationToken)
				<< "driveKey" << ToBinary(entry.DriveKey)
				<< "driveAddress" << ToBinary(driveAddress)
				<< "fileRecipient" << ToBinary(entry.FileRecipient)
				<< "height" << ToInt64(entry.Height);

		StreamFiles(builder, entry.Files);

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadFiles(std::map<Hash256, uint64_t>& files, const bsoncxx::array::view& dbFiles) {
			for (const auto& dbFile : dbFiles) {
				auto doc = dbFile.get_document().view();
				Hash256 fileHash;
				DbBinaryToModelArray(fileHash, doc["fileHash"].get_binary());
				uint64_t fileSize = doc["fileSize"].get_int64();
				files.emplace(fileHash, fileSize);
			}
		}
	}

	state::DownloadEntry ToDownloadEntry(const bsoncxx::document::view& document) {
		auto dbDownloadEntry = document["downloadInfo"];
		state::DownloadEntry entry;
		DbBinaryToModelArray(entry.OperationToken, dbDownloadEntry["operationToken"].get_binary());
		DbBinaryToModelArray(entry.DriveKey, dbDownloadEntry["driveKey"].get_binary());
		DbBinaryToModelArray(entry.FileRecipient, dbDownloadEntry["fileRecipient"].get_binary());
		entry.Height = Height(dbDownloadEntry["height"].get_int64());

		ReadFiles(entry.Files, dbDownloadEntry["files"].get_array().value);

		return entry;
	}

	// endregion
}}}

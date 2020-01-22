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
		void StreamFiles(bson_stream::document& builder, const std::set<Hash256>& fileHashes) {
			auto array = builder << "files" << bson_stream::open_array;
			for (const auto& fileHash : fileHashes)
				array << ToBinary(fileHash);

			array << bson_stream::close_array;
		}

		void StreamDownloads(bson_stream::document& builder, const state::DownloadMap& downloads) {
			auto array = builder << "downloads" << bson_stream::open_array;
			for (const auto& downloadPair : downloads) {
				bson_stream::document downloadBuilder;
				downloadBuilder << "operationToken" << ToBinary(downloadPair.first);
				StreamFiles(downloadBuilder, downloadPair.second);
				array << downloadBuilder;
			}

			array << bson_stream::close_array;
		}

		void StreamFileRecipients(bson_stream::document& builder, const state::FileRecipientMap& fileRecipients) {
			auto array = builder << "fileRecipients" << bson_stream::open_array;
			for (const auto& fileRecipientPair : fileRecipients) {
				bson_stream::document fileRecipientBuilder;
				fileRecipientBuilder << "key" << ToBinary(fileRecipientPair.first);
				StreamDownloads(fileRecipientBuilder, fileRecipientPair.second);
				array << fileRecipientBuilder;
			}

			array << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(const state::DownloadEntry& entry, const Address& driveAddress) {
		bson_stream::document builder;
		auto doc = builder << "downloadInfo" << bson_stream::open_document
				<< "driveKey" << ToBinary(entry.driveKey())
				<< "driveAddress" << ToBinary(driveAddress);

		StreamFileRecipients(builder, entry.fileRecipients());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {
		void ReadFiles(std::set<Hash256>& fileHashes, const bsoncxx::array::view& dbFiles) {
			for (const auto& dbFile : dbFiles) {
				Hash256 fileHash;
				DbBinaryToModelArray(fileHash, dbFile.get_binary());
				fileHashes.insert(fileHash);
			}
		}

		void ReadDownloads(state::DownloadMap& downloads, const bsoncxx::array::view& dbDownloads) {
			for (const auto& dbDownload : dbDownloads) {
				auto doc = dbDownload.get_document().view();

                Hash256 operationToken;
                DbBinaryToModelArray(operationToken, doc["operationToken"].get_binary());
				std::set<Hash256> fileHashes;
				ReadFiles(fileHashes, doc["files"].get_array().value);
				downloads.emplace(operationToken, fileHashes);
			}
		}

		void ReadFileRecipients(state::FileRecipientMap& fileRecipients, const bsoncxx::array::view& dbFileRecipients) {
			for (const auto& dbFileRecipient : dbFileRecipients) {
				auto doc = dbFileRecipient.get_document().view();

				Key key;
				DbBinaryToModelArray(key, doc["key"].get_binary());
				state::DownloadMap downloads;
				ReadDownloads(downloads, doc["downloads"].get_array().value);
				fileRecipients.emplace(key, downloads);
			}
		}
	}

	state::DownloadEntry ToDownloadEntry(const bsoncxx::document::view& document) {
		auto dbDownloadEntry = document["downloadInfo"];
		Key driveKey;
		DbBinaryToModelArray(driveKey, dbDownloadEntry["driveKey"].get_binary());
		state::DownloadEntry entry(driveKey);

		ReadFileRecipients(entry.fileRecipients(), dbDownloadEntry["fileRecipients"].get_array().value);

		return entry;
	}

	// endregion
}}}

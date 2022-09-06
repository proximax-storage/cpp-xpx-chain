/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PublicKeyStatementMapper.h"
#include "ReceiptMapper.h"
#include "catapult/model/Statement.h"

namespace catapult { namespace mongo { namespace mappers {

	namespace {
		void StreamReceipts(
				bson_stream::document& builder,
				const model::BlockchainStateStatement& statement,
				const MongoReceiptRegistry& receiptRegistry) {
			auto receiptsArray = builder << "receipts" << bson_stream::open_array;
			for (auto i = 0u; i < statement.size(); ++i) {
				const auto& receipt = statement.receiptAt(i);
				bsoncxx::builder::stream::document receiptBuilder;
				StreamReceipt(receiptBuilder, receipt, receiptRegistry);
				receiptsArray << receiptBuilder;
			}

			receiptsArray << bson_stream::close_array;
		}
	}

	bsoncxx::document::value ToDbModel(
			Height height,
			const model::BlockchainStateStatement& statement,
			const MongoReceiptRegistry& receiptRegistry) {
		bson_stream::document builder;
		builder
				<< "height" << ToInt64(height)
				<< "source" << bson_stream::open_document
				<< "primaryId" << static_cast<int32_t>(statement.source().PrimaryId)
				<< "secondaryId" << static_cast<int32_t>(statement.source().SecondaryId)
				<< bson_stream::close_document;

		StreamReceipts(builder, statement, receiptRegistry);
		return builder << bson_stream::finalize;
	}
}}}

/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "InstallMessageMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "plugins/txes/dbrb/src/model/InstallMessageTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	template<typename TTransaction>
	void StreamInstallMessageTransaction(bson_stream::document& builder, const TTransaction& transaction) {
		builder << "messageHash" << ToBinary(transaction.MessageHash);

		auto pBuffer = transaction.PayloadPtr();
		auto sequence = dbrb::Read<dbrb::Sequence>(pBuffer);
		auto certificate = dbrb::Read<dbrb::CertificateType>(pBuffer);

		auto sequenceArray = builder << "sequence" << bson_stream::open_array;
		for (const auto& view : sequence.data()) {
			auto viewArray = sequenceArray << bson_stream::open_array;
			for (const auto& [processId, change] : view.Data) {
				viewArray << bson_stream::open_document
					<< "processId" << ToBinary(processId)
					<< "membershipChange" << static_cast<int32_t>(change)
					<< bson_stream::close_document;
			}
			viewArray << bson_stream::close_array;
		}
		sequenceArray << bson_stream::close_array;

		auto certificateArray = builder << "certificate" << bson_stream::open_array;
		for (const auto& [processId, signature] : certificate) {
			certificateArray << bson_stream::open_document
				  << "processId" << ToBinary(processId)
				  << "signature" << ToBinary(signature)
				  << bson_stream::close_document;
		}
		certificateArray << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(InstallMessage, StreamInstallMessageTransaction)
}}}

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
		builder << "viewsCount" << static_cast<int32_t>(transaction.ViewsCount);
		builder << "mostRecentViewSize" << static_cast<int32_t>(transaction.MostRecentViewSize);
		builder << "signaturesCount" << static_cast<int32_t>(transaction.SignaturesCount);

		// Streaming ViewSizes
		auto viewSizesArray = builder << "viewSizes" << bson_stream::open_array;
		auto pSize = transaction.ViewSizesPtr();
		for (auto i = 0; i < transaction.ViewsCount; ++i, ++pSize)
			viewSizesArray << static_cast<int32_t>(*pSize);
		viewSizesArray << bson_stream::close_array;

		// Streaming ViewProcessIds
		auto viewProcessIdsArray = builder << "viewProcessIds" << bson_stream::open_array;
		auto pViewProcessId = transaction.ViewProcessIdsPtr();
		for (auto i = 0; i < transaction.MostRecentViewSize; ++i, ++pViewProcessId)
			viewProcessIdsArray << ToBinary(*pViewProcessId);
		viewProcessIdsArray << bson_stream::close_array;

		// Streaming MembershipChanges
		auto membershipChangesArray = builder << "membershipChanges" << bson_stream::open_array;
		auto pChange = transaction.MembershipChangesPtr();
		for (auto i = 0; i < transaction.MostRecentViewSize; ++i, ++pChange)
			membershipChangesArray << *pChange;
		membershipChangesArray << bson_stream::close_array;

		// Streaming SignaturesProcessIds
		auto signatureProcessIdsArray = builder << "signatureProcessIds" << bson_stream::open_array;
		auto pSigProcessId = transaction.SignaturesProcessIdsPtr();
		for (auto i = 0; i < transaction.SignaturesCount; ++i, ++pSigProcessId)
			signatureProcessIdsArray << ToBinary(*pSigProcessId);
		signatureProcessIdsArray << bson_stream::close_array;

		// Streaming Signatures
		auto signaturesArray = builder << "signatures" << bson_stream::open_array;
		auto pSignature = transaction.SignaturesPtr();
		for (auto i = 0; i < transaction.SignaturesCount; ++i, ++pSignature)
			signaturesArray << ToBinary(*pSignature);
		signaturesArray << bson_stream::close_array;
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(InstallMessage, StreamInstallMessageTransaction)
}}}

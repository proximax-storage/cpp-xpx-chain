/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NamespaceExtendedMetadataMapper.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/metadata_v2/src/model/NamespaceExtendedMetadataTransaction.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Stream(bson_stream::document& builder, const TTransaction& transaction) {
			builder
					<< "targetKey" << ToBinary(transaction.TargetKey)
					<< "scopedMetadataKey" << static_cast<int64_t>(transaction.ScopedMetadataKey)
					<< "targetNamespaceId" << ToInt64(transaction.TargetNamespaceId)
					<< "valueSizeDelta" << transaction.ValueSizeDelta
					<< "valueSize" << transaction.ValueSize
					<< "isImmutable" << transaction.IsValueImmutable;

			if (0 < transaction.ValueSize)
				builder << "value" << ToBinary(transaction.ValuePtr(), transaction.ValueSize);
		}
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(NamespaceExtendedMetadata, Stream)
}}}

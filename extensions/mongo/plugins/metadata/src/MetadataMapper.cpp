/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MetadataMapper.h"
#include "mongo/src/MongoTransactionPluginFactory.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "plugins/txes/metadata/src/model/AddressMetadataTransaction.h"
#include "plugins/txes/metadata/src/model/MosaicMetadataTransaction.h"
#include "plugins/txes/metadata/src/model/NamespaceMetadataTransaction.h"
#include "plugins/txes/metadata/src/model/MetadataTypes.h"
#include "plugins/txes/metadata/src/state/MetadataUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		void StreamModification(
				bson_stream::array_context& context,
				const model::MetadataModification& modification) {
			context
					<< bson_stream::open_document
						<< "modificationType" << utils::to_underlying_type(modification.ModificationType)
						<< "key" << std::string(modification.KeyPtr(), modification.KeySize)
						<< "value" << std::string(modification.ValuePtr(), modification.ValueSize)
					<< bson_stream::close_document;
		}

		template<typename TModifications>
		void StreamModifications(
				bson_stream::document& builder, TModifications modifications) {
			auto modificationsArray = builder << "modifications" << bson_stream::open_array;
			for (const auto& modification : modifications)
				StreamModification(modificationsArray, modification);

			modificationsArray << bson_stream::close_array;
		}

		template<typename TMetadataId>
		struct MetadataTransactionStreamer {
			template<typename TTransaction>
			static void Stream(bson_stream::document& builder, const TTransaction& transaction) {
				auto vectorizedMetadataId = state::ToVector<TMetadataId>(transaction.MetadataId);
				builder << "metadataType" << utils::to_underlying_type(transaction.MetadataType);
				builder << "metadataId" << ToBinary(vectorizedMetadataId.data(), vectorizedMetadataId.size());
				StreamModifications(builder, transaction.Transactions());
			}
		};
	}

	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(AddressMetadata, MetadataTransactionStreamer<UnresolvedAddress>::Stream)
	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(MosaicMetadata, MetadataTransactionStreamer<UnresolvedMosaicId>::Stream)
	DEFINE_MONGO_TRANSACTION_PLUGIN_FACTORY(NamespaceMetadata, MetadataTransactionStreamer<NamespaceId>::Stream)
}}}

/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataEntityType.h"
#include "ExtendedMetadataSharedTransaction.h"
#include "catapult/model/ContainerTypes.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/config/BlockchainConfiguration.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Metadata transaction header with mosaic id target.
	template<typename THeader>
	struct MosaicExtendedMetadataTransactionHeader : public ExtendedMetadataTransactionHeader<THeader> {
		/// Target mosaic identifier.
		UnresolvedMosaicId TargetMosaicId;
	};

	/// Binary layout for a mosaic extended metadata transaction body.
	template<typename THeader>
	struct MosaicExtendedMetadataTransactionBody
			: public BasicExtendedMetadataTransactionBody<MosaicExtendedMetadataTransactionHeader<THeader>, Entity_Type_Mosaic_Extended_Metadata>
	{};

	DEFINE_EMBEDDABLE_TRANSACTION(MosaicExtendedMetadata)

#pragma pack(pop)

	/// Extracts addresses of additional accounts that must approve \a transaction.
	inline utils::KeySet ExtractAdditionalRequiredCosigners(const EmbeddedMosaicExtendedMetadataTransaction& transaction, const config::BlockchainConfiguration&) {
		return { transaction.TargetKey };
	}
}}


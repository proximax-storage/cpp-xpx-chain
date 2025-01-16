/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataEntityType.h"
#include "ExtendedMetadataSharedTransaction.h"
#include "plugins/txes/namespace/src/types.h"
#include "catapult/model/ContainerTypes.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/config/BlockchainConfiguration.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Extended metadata transaction header with namespace id target.
	template<typename THeader>
	struct NamespaceExtendedMetadataTransactionHeader : public ExtendedMetadataTransactionHeader<THeader> {
		/// Target namespace identifier.
		NamespaceId TargetNamespaceId;
	};

	/// Binary layout for a namespace extended metadata transaction body.
	template<typename THeader>
	struct NamespaceExtendedMetadataTransactionBody
			: public BasicExtendedMetadataTransactionBody<NamespaceExtendedMetadataTransactionHeader<THeader>, Entity_Type_Namespace_Extended_Metadata>
	{};

	DEFINE_EMBEDDABLE_TRANSACTION(NamespaceExtendedMetadata)

#pragma pack(pop)

	/// Extracts addresses of additional accounts that must approve \a transaction.
	inline utils::KeySet ExtractAdditionalRequiredCosigners(const EmbeddedNamespaceExtendedMetadataTransaction& transaction, const config::BlockchainConfiguration&) {
		return { transaction.TargetKey };
	}
}}


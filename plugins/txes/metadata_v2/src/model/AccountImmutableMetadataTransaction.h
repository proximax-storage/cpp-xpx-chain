/**
*** Copyright (c) 2016-2019, Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp.
*** Copyright (c) 2020-present, Jaguar0625, gimre, BloodyRookie.
*** All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "MetadataEntityType.h"
#include "ImmutableMetadataSharedTransaction.h"
#include "catapult/model/ContainerTypes.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/config/BlockchainConfiguration.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for an account immutable metadata transaction body.
	template<typename THeader>
	struct AccountImmutableMetadataTransactionBody
			: public BasicImmutableMetadataTransactionBody<ImmutableMetadataTransactionHeader<THeader>, Entity_Type_Account_Immutable_Metadata>
	{};

	DEFINE_EMBEDDABLE_TRANSACTION(AccountImmutableMetadata)

#pragma pack(pop)

	/// Extracts addresses of additional accounts that must approve \a transaction.
	inline utils::KeySet ExtractAdditionalRequiredCosigners(const EmbeddedAccountImmutableMetadataTransaction& transaction, const config::BlockchainConfiguration&) {
		return { transaction.TargetKey };
	}
}}


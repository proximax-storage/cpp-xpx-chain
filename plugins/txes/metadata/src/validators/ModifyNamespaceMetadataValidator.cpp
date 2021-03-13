/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "plugins/txes/namespace/src/cache/NamespaceCache.h"
#include "catapult/validators/StatefulValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::ModifyNamespaceMetadataNotification_v1;

	DEFINE_STATEFUL_VALIDATOR(ModifyNamespaceMetadata, [](const auto& notification, const StatefulValidatorContext& context) {
		constexpr uint64_t mask = 1ull << 63;
		if (!(notification.MetadataId.unwrap() & mask))
			return Failure_Metadata_NamespaceId_Malformed;

		const auto& namespaceCache = context.Cache.sub<cache::NamespaceCache>();
		auto it = namespaceCache.find(notification.MetadataId);

		if (!it.tryGet())
			return Failure_Metadata_Namespace_Not_Found;

		if (it.get().root().owner() != notification.Signer)
			return Failure_Metadata_Namespace_Modification_Not_Permitted;

		return ValidationResult::Success;
	});
}}

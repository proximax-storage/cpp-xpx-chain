/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <plugins/txes/metadata/src/config/MetadataConfiguration.h>
#include "Results.h"
#include "plugins/txes/metadata/src/model/MetadataNotifications.h"
#include "catapult/utils/UnresolvedAddress.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to metadata field modification and validates that
	DECLARE_STATELESS_VALIDATOR(MetadataType, model::MetadataTypeNotification<1>)();

	/// A validator implementation that applies to metadata field modification and validates that
	DECLARE_STATELESS_VALIDATOR(MetadataFieldModification, model::ModifyMetadataFieldNotification<1>)(uint8_t maxKeySize, uint16_t maxValueSize);

	/// A validator implementation that applies to metadata modifications check that modification is valid
	DECLARE_STATEFUL_VALIDATOR(MetadataModifications, model::MetadataModificationsNotification<1>)(const uint8_t& maxFields);

	/// A validator implementation that applies to metadata check that operation is permitted and address exists
	DECLARE_STATEFUL_VALIDATOR(ModifyAddressMetadata, model::ModifyAddressMetadataNotification)();

	/// A validator implementation that applies to metadata check that operation is permitted and mosaicId exists
	DECLARE_STATEFUL_VALIDATOR(ModifyMosaicMetadata, model::ModifyMosaicMetadataNotification)();

	/// A validator implementation that applies to metadata check that operation is permitted and namespaceId exists
	DECLARE_STATEFUL_VALIDATOR(ModifyNamespaceMetadata, model::ModifyNamespaceMetadataNotification)();
}}

/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "src/config/MetadataV1Configuration.h"
#include "Results.h"
#include "plugins/txes/metadata/src/model/MetadataV1Notifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to metadata field modification and validates that
	DECLARE_STATELESS_VALIDATOR(MetadataV1Type, model::MetadataV1TypeNotification<1>)();

	/// A validator implementation that applies to metadata field modification and validates that
	DECLARE_STATEFUL_VALIDATOR(MetadataV1FieldModification, model::ModifyMetadataV1FieldNotification<1>)();

	/// A validator implementation that applies to metadata modifications check that modification is valid
	DECLARE_STATEFUL_VALIDATOR(MetadataV1Modifications, model::MetadataV1ModificationsNotification<1>)();

	/// A validator implementation that applies to metadata check that operation is permitted and address exists
	DECLARE_STATEFUL_VALIDATOR(ModifyAddressMetadataV1, model::ModifyAddressMetadataNotification_v1)();

	/// A validator implementation that applies to metadata check that operation is permitted and mosaicId exists
	DECLARE_STATEFUL_VALIDATOR(ModifyMosaicMetadataV1, model::ModifyMosaicMetadataNotification_v1)();

	/// A validator implementation that applies to metadata check that operation is permitted and namespaceId exists
	DECLARE_STATEFUL_VALIDATOR(ModifyNamespaceMetadataV1, model::ModifyNamespaceMetadataNotification_v1)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(MetadataV1PluginConfig, model::PluginConfigNotification<1>)();
}}

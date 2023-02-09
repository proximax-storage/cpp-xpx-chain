/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
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
#include "Results.h"
#include "src/model/MosaicNotifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/model/Notifications.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include <unordered_set>

namespace catapult { namespace validators {

	// region MosaicChangeTransaction

	/// A validator implementation that applies to mosaic required notifications and validates that:
	/// - mosaic exists and is active
	/// - mosaic owner matches requesting signer
	DECLARE_STATEFUL_VALIDATOR(ProperMosaicV1, model::MosaicRequiredNotification<1>)();
	/// - necessary flags are set
	DECLARE_STATEFUL_VALIDATOR(ProperMosaicV2, model::MosaicRequiredNotification<2>)();

	// endregion

	// region MosaicDefinitionTransaction

	/// A validator implementation that applies to mosaic properties notifications and validates that:
	/// - definition has valid mosaic flags
	/// - definition has divisibility no greater than \a maxDivisibility
	/// - mosaic duration has a value not larger than \a maxMosaicDuration
	/// - optional mosaic properties are sorted, known and not duplicative
	DECLARE_STATEFUL_VALIDATOR(MosaicPropertiesV1, model::MosaicPropertiesNotification<1>)();
	DECLARE_STATEFUL_VALIDATOR(MosaicPropertiesV2, model::MosaicPropertiesNotification<2>)();

	/// A validator implementation that applies to mosaic nonce notifications and validates that:
	/// - mosaic id is the expected id generated from signer and nonce
	DECLARE_STATELESS_VALIDATOR(MosaicId, model::MosaicNonceNotification<1>)();

	/// A validator implementation that applies to mosaic definition notifications and validates that:
	/// - the mosaic is available and can be created or modified
	DECLARE_STATEFUL_VALIDATOR(MosaicAvailability, model::MosaicDefinitionNotification<1>)();

	/// A validator implementation that applies to mosaic definition notifications and validates that:
	/// - the resulting mosaic duration is not larger than \a maxMosaicDuration and there was no overflow
	DECLARE_STATEFUL_VALIDATOR(MosaicDuration, model::MosaicDefinitionNotification<1>)();

	// endregion

	// region MosaicSupplyChangeTransaction

	/// A validator implementation that applies to mosaic supply change notifications and validates that:
	/// - direction has a valid value
	/// - delta amount is non-zero
	DECLARE_STATELESS_VALIDATOR(MosaicSupplyChangeV1, model::MosaicSupplyChangeNotification<1>)();
	DECLARE_STATELESS_VALIDATOR(MosaicSupplyChangeV2, model::MosaicSupplyChangeNotification<2>)();

	/// A validator implementation that applies to all balance transfer notifications and validates that:
	/// - transferred mosaic is active and is transferable
	/// - as an optimization, special currency mosaic (\a currencyMosaicId) transfers are always allowed
	DECLARE_STATEFUL_VALIDATOR(MosaicTransfer, model::BalanceTransferNotification<1>)(UnresolvedMosaicId currencyMosaicId);

	/// A validator implementation that applies to mosaic supply change notifications and validates that:
	/// - the affected mosaic has mutable supply
	/// - decrease does not cause owner amount to become negative
	/// - increase does not cause total atomic units to exceed max atomic units
	/// \note This validator is dependent on MosaicChangeAllowedValidator.
	DECLARE_STATEFUL_VALIDATOR(MosaicSupplyChangeAllowedV1, model::MosaicSupplyChangeNotification<1>)();
	/// - supply forever immutable flag is not set
	DECLARE_STATEFUL_VALIDATOR(MosaicSupplyChangeAllowedV2, model::MosaicSupplyChangeNotification<2>)();

	/// A validator implementation that applies to mosaic supply change notifications and validates that:
	/// - the account changing the supply does not exceed the maximum number of mosaics (\a maxMosaics) an account is allowed to own
	DECLARE_STATEFUL_VALIDATOR(MaxMosaicsSupplyChangeV1, model::MosaicSupplyChangeNotification<1>)();
	DECLARE_STATEFUL_VALIDATOR(MaxMosaicsSupplyChangeV2, model::MosaicSupplyChangeNotification<2>)();

	// endregion

	// region TransferTransaction

	/// A validator implementation that applies to all balance transfer notifications and validates that:
	/// - the recipient does not exceed the maximum number of mosaics (\a maxMosaics) an account is allowed to own
	DECLARE_STATEFUL_VALIDATOR(MaxMosaicsBalanceTransfer, model::BalanceTransferNotification<1>)();

	// endregion

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(MosaicPluginConfig, model::PluginConfigNotification<1>)();
	
	/// A validator implementation that applies to during addition or modification of levy
	/// - check if signer is eligible
	/// - check if recipient address is valid
	/// - check if fee is valid
	/// - check mosaic Id is valid
	DECLARE_STATEFUL_VALIDATOR(ModifyLevy, model::MosaicModifyLevyNotification<1>)();
	
	/// A validator implementation that checks for valid removal of levy
	/// - check if signer is eligible
	/// - check if levy for this mosaicID exist
	/// - check if current levy is set
	DECLARE_STATEFUL_VALIDATOR(RemoveLevy, model::MosaicRemoveLevyNotification<1>)();
	
	/// A validator implementation that checks for levy configuration during transfer
	/// - check if levy config is enabled
	//  - check if transfered mosaic has levy
	/// - check if sender has enough balance for levy
	DECLARE_STATEFUL_VALIDATOR(LevyTransfer, model::BalanceTransferNotification<1>)();
}}

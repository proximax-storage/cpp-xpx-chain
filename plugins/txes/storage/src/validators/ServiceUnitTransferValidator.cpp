/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::BalanceTransferNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ServiceUnitTransfer, ([](const Notification& notification, const ValidatorContext& context) {
	  	const auto& mosaicId = context.Resolvers.resolve(notification.MosaicId);
		std::set<MosaicId> serviceMosaicIds({
			context.Config.Immutable.StorageMosaicId,
			context.Config.Immutable.StreamingMosaicId,
			context.Config.Immutable.ReviewMosaicId,
			context.Config.Immutable.SuperContractMosaicId,
		});

		if (serviceMosaicIds.count(mosaicId))
			return Failure_Storage_Service_Unit_Transfer;

		return ValidationResult::Success;
	}));

}}

/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::DebitMosaicNotification<1>;

	DEFINE_STATEFUL_VALIDATOR_WITH_LIQUIDITY_PROVIDER(DebitMosaic, [&liquidityProvider](const Notification& notification, const ValidatorContext& context) {
		return liquidityProvider->validateDebitMosaics(context, notification.MosaicDebtor, notification.MosaicId, notification.MosaicAmount);
	})
}}
/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(CreditMosaic, model::CreditMosaicNotification<1>, [&liquidityProvider](const model::CreditMosaicNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (CreditMosaic)");

		liquidityProvider.creditMosaics(context, notification.CurrencyDebtor, notification.MosaicCreditor, notification.MosaicId, notification.MosaicAmount);
	});
}} // namespace catapult::observers
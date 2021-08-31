/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SwapOperation.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/NotificationSubscriber.h"

namespace catapult { namespace utils {

	/// Swap mosaics between \a sender and \a receiver.
	void SwapMosaics(const Key&, const Key&, const std::vector<model::UnresolvedMosaic>&, model::NotificationSubscriber&, const config::ImmutableConfiguration&, SwapOperation);

	/// Swap unresolved amount of mosaics between \a sender and \a receiver.
	void SwapMosaics(const Key&, const Key&, const std::vector<std::pair<UnresolvedMosaicId, UnresolvedAmount>>&, model::NotificationSubscriber&, const config::ImmutableConfiguration&, SwapOperation);

	/// Swap mosaics on the \a account.
	void SwapMosaics(const Key&, const std::vector<model::UnresolvedMosaic>&, model::NotificationSubscriber&, const config::ImmutableConfiguration&, SwapOperation);

	/// Swap unresolved amount of mosaics on the \a account.
	void SwapMosaics(const Key&, const std::vector<std::pair<UnresolvedMosaicId, UnresolvedAmount>>&, model::NotificationSubscriber&, const config::ImmutableConfiguration&, SwapOperation);

	/// Writes \a data to \a ptr one byte at a time. When done, \a ptr points to the past-the-last byte.
	template<typename TData>
	void WriteToByteArray(uint8_t*& ptr, const TData& data);
}}

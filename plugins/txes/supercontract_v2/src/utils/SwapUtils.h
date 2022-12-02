/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <cstdint>
#include <iosfwd>
#include <catapult/types.h>
#include <catapult/model/Mosaic.h>
#include <catapult/model/NotificationSubscriber.h>
#include <catapult/config/ImmutableConfiguration.h>

namespace catapult { namespace utils {

#define SWAP_OPERATION_LIST     \
	/* Buy service mosaics. */  \
	ENUM_VALUE(Buy)             \
                                \
	/* Sell service mosaics. */ \
	ENUM_VALUE(Sell)

#define ENUM_VALUE(LABEL) LABEL,
	/// Enumeration of possible swap operations.
	enum class SwapOperation : uint8_t { SWAP_OPERATION_LIST };
#undef ENUM_VALUE

/// Insertion operator for outputting \a value to \a out.
std::ostream& operator<<(std::ostream& out, SwapOperation value);

/// Swap mosaics between \a sender and \a receiver.
void SwapMosaics(
		const Key& sender,
		const Key& receiver,
		const std::vector<model::UnresolvedMosaic>&,
		model::NotificationSubscriber&,
		const config::ImmutableConfiguration&,
		SwapOperation);

/// Swap unresolved amount of mosaics between \a sender and \a receiver.
void SwapMosaics(
		const Key& sender,
		const Key& receiver,
		const std::vector<std::pair<UnresolvedMosaicId, UnresolvedAmount>>&,
		model::NotificationSubscriber&,
		const config::ImmutableConfiguration&,
		SwapOperation);

/// Swap mosaics on the \a account.
void SwapMosaics(
		const Key& account,
		const std::vector<model::UnresolvedMosaic>&,
		model::NotificationSubscriber&,
		const config::ImmutableConfiguration&,
		SwapOperation);

/// Swap unresolved amount of mosaics on the \a account.
void SwapMosaics(
		const Key& account,
		const std::vector<std::pair<UnresolvedMosaicId, UnresolvedAmount>>&,
		model::NotificationSubscriber&,
		const config::ImmutableConfiguration&,
		SwapOperation);

}} // namespace catapult::utils
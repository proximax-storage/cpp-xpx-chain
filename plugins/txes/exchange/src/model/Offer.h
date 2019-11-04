/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Mosaic.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Offer type
	enum class OfferType : uint8_t {
		/// Buy offer.
		Buy = 1,

		/// Sell offer.
		Sell
	};

	struct Offer {
	public:
		/// Mosaic for exchange.
		UnresolvedMosaic Mosaic;

		/// Sum of XPX suggested to be paid for mosaic.
		Amount Cost;

		/// Offer type.
		OfferType Type;
	};

	struct MatchedOffer : public Offer {
	public:
		/// The owner of the matched offer.
		Key Owner;
	};

	struct OfferWithDuration : public Offer {
	public:
		/// The duration of the offer.
		BlockDuration Duration;
	};

	struct OfferMosaic {
		/// Mosaic id of the offer.
		UnresolvedMosaicId MosaicId;

		/// Offer type.
		model::OfferType OfferType;
	};

#pragma pack(pop)
}}

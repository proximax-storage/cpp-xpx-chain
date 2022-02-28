/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Mosaic.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Offer type
	enum class SdaOfferType : uint8_t {
        /// Sell offer.
        Sell,

		/// Buy offer.
		Buy,

		Begin = Sell,
		End = Buy,
	};

	/// Insertion operator for outputting \a type to \a out.
	std::ostream& operator<<(std::ostream& out, SdaOfferType type);

	struct SdaOffer {
	public:
		/// Mosaic for exchange.
		UnresolvedMosaic Mosaic;

		/// Sum of mosaic suggested to be paid in exchange with another mosaic.
		UnresolvedMosaic Cost;

		/// Offer type.
		SdaOfferType Type;
	};

	struct MatchedSdaOffer : public SdaOffer {
	public:
		/// The owner of the matched offer.
		Key Owner;
	};

	struct SdaOfferWithDuration : public SdaOffer {
	public:
		/// The duration of the offer.
		BlockDuration Duration;
	};

	struct SdaOfferMosaic {
		/// Mosaic id of the offer.
		UnresolvedMosaicId MosaicId;

		/// Offer type.
		model::SdaOfferType SdaOfferType;
	};

#pragma pack(pop)
}}

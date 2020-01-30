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
        /// Sell offer.
        Sell,

		/// Buy offer.
		Buy,
	};

	/// Insertion operator for outputting \a type to \a out.
	std::ostream& operator<<(std::ostream& out, OfferType type);

	struct Offer {
	public:
		/// Mosaic for exchange.
		UnresolvedMosaic Mosaic;

		/// Sum of XPX suggested to be paid for mosaic.
		Amount Cost;

		/// Offer type.
		OfferType Type;

	public:
		bool operator==(const Offer& other) const {
			return other.Mosaic == this->Mosaic && other.Cost == this->Cost && other.Type == this->Type;
		}
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

	public:
		bool operator==(const OfferWithDuration& other) const {
			return other.Duration == this->Duration && static_cast<const Offer&>(*this) == static_cast<const Offer&>(other);
		}
	};

	struct OfferMosaic {
		/// Mosaic id of the offer.
		UnresolvedMosaicId MosaicId;

		/// Offer type.
		model::OfferType OfferType;

	public:
		bool operator==(const OfferMosaic& other) const {
			return other.MosaicId == this->MosaicId && other.OfferType == this->OfferType;
		}
	};

#pragma pack(pop)
}}

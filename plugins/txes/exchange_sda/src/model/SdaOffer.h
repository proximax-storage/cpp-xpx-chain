/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/Mosaic.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	struct SdaOffer {
	public:
		/// Mosaic for exchange.
		UnresolvedMosaic MosaicGive;

		/// Sum of mosaic suggested to be exchanged with another mosaic.
		UnresolvedMosaic MosaicGet;
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
	};

#pragma pack(pop)
}}

/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaOffer.h"

namespace catapult { namespace model {

	std::ostream& operator<<(std::ostream& out, SdaOfferType type) {
		out << ((SdaOfferType::Buy == type) ? "Buy" : "Sell");
		return out;
	}
}}

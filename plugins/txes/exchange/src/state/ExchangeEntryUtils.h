/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/io/Stream.h"
#include "src/model/Offer.h"
#include <map>

namespace catapult { namespace state {

	using OfferMap = std::map<UnresolvedMosaicId, model::Offer>;

	void WriteOffers(const OfferMap& offers, io::OutputStream& output);
	void ReadOffers(OfferMap& offers, io::InputStream& input);
}}

/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeEntryUtils.h"
#include "catapult/io/PodIoUtils.h"

namespace catapult { namespace state {

	void WriteOffers(const OfferMap& offers, io::OutputStream& output) {
		io::Write8(output, static_cast<uint8_t>(offers.size()));
		for (const auto& pair : offers) {
			io::Write(output, pair.second.Mosaic.MosaicId);
			io::Write(output, pair.second.Mosaic.Amount);
			io::Write(output, pair.second.Cost);
		}
	}

	void ReadOffers(OfferMap& offers, io::InputStream& input) {
		auto offerCount = io::Read8(input);
		for (uint8_t i = 0; i < offerCount; ++i) {
			model::Offer offer;
			offer.Mosaic.MosaicId = io::Read<UnresolvedMosaicId>(input);
			offer.Mosaic.Amount = io::Read<Amount>(input);
			offer.Cost = io::Read<Amount>(input);
			offers.emplace(offer.Mosaic.MosaicId, offer);
		}
	}
}}

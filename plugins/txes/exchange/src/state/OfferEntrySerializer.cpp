/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OfferEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {
		void WriteOffers(const OfferMap& offers, io::OutputStream& output) {
			io::Write8(output, static_cast<uint8_t>(offers.size()));
			for (const auto& pair : offers) {
				io::Write(output, pair.second.Mosaic.MosaicId);
				io::Write(output, pair.second.Mosaic.Amount);
				io::Write(output, pair.second.Cost);
			}
		}
	}

	void OfferEntrySerializer::Save(const OfferEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write(output, entry.transactionHash());
		io::Write(output, entry.transactionSigner());
		io::Write8(output, utils::to_underlying_type(entry.offerType()));
		io::Write(output, entry.deadline());
		io::Write(output, entry.expiryHeight());

		WriteOffers(entry.offers(), output);
		WriteOffers(entry.initialOffers(), output);
	}

	namespace {
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
	}

	OfferEntry OfferEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of OfferEntry", version);

		auto transactionHash = io::Read<utils::ShortHash>(input);
		Key transactionSigner;
		io::Read(input, transactionSigner);
		auto offerType = static_cast<model::OfferType>(io::Read8(input));
		auto deadline = io::Read<Height>(input);
		state::OfferEntry entry(transactionHash, transactionSigner, offerType, deadline);

		auto expiryHeight = io::Read<Height>(input);
		entry.setExpiryHeight(expiryHeight);

		ReadOffers(entry.offers(), input);
		ReadOffers(entry.initialOffers(), input);

		return entry;
	}
}}

/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {
		void WriteOffer(const MosaicId& mosaicId, const OfferBase& offer, io::OutputStream& output) {
			io::Write(output, mosaicId);
			io::Write(output, offer.Amount);
			io::Write(output, offer.InitialAmount);
			io::Write(output, offer.InitialCost);
			io::Write(output, offer.Deadline);
			io::Write(output, offer.ExpiryHeight);
			io::Write8(output, offer.Expired);
		}

		void WriteSellOffers(const SellOfferMap& offers, io::OutputStream& output) {
			io::Write8(output, utils::checked_cast<size_t, uint8_t>(offers.size()));
			for (const auto& pair : offers) {
				WriteOffer(pair.first, pair.second, output);
			}
		}

		void WriteBuyOffers(const BuyOfferMap& offers, io::OutputStream& output) {
			io::Write8(output, utils::checked_cast<size_t, uint8_t>(offers.size()));
			for (const auto& pair : offers) {
				WriteOffer(pair.first, pair.second, output);
				io::Write(output, pair.second.ResidualCost);
			}
		}
	}

	void ExchangeEntrySerializer::Save(const ExchangeEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write(output, entry.owner());

		WriteSellOffers(entry.sellOffers(), output);
		WriteBuyOffers(entry.buyOffers(), output);
	}

	namespace {
		void ReadOffer(MosaicId& mosaicId, OfferBase& offer, io::InputStream& input) {
			mosaicId = io::Read<MosaicId>(input);
			offer.Amount = io::Read<Amount>(input);
			offer.InitialAmount = io::Read<Amount>(input);
			offer.InitialCost = io::Read<Amount>(input);
			offer.Deadline = io::Read<Height>(input);
			offer.ExpiryHeight = io::Read<Height>(input);
			offer.Expired = io::Read8(input);
		}

		void ReadSellOffers(SellOfferMap& offers, io::InputStream& input) {
			auto offerCount = io::Read8(input);
			for (uint8_t i = 0; i < offerCount; ++i) {
				MosaicId mosaicId;
				SellOffer offer;
				ReadOffer(mosaicId, offer, input);
				offers.emplace(mosaicId, offer);
			}
		}

		void ReadBuyOffers(BuyOfferMap& offers, io::InputStream& input) {
			auto offerCount = io::Read8(input);
			for (uint8_t i = 0; i < offerCount; ++i) {
				MosaicId mosaicId;
				BuyOffer offer;
				ReadOffer(mosaicId, offer, input);
				offer.ResidualCost = io::Read<Amount>(input);
				offers.emplace(mosaicId, offer);
			}
		}
	}

	ExchangeEntry ExchangeEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of ExchangeEntry", version);

		Key owner;
		io::Read(input, owner);
		state::ExchangeEntry entry(owner);

		ReadSellOffers(entry.sellOffers(), input);
		ReadBuyOffers(entry.buyOffers(), input);

		return entry;
	}
}}

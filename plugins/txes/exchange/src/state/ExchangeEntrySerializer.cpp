/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {
		void WriteOffer(const MosaicId& mosaicId, const OfferBase& offer, io::OutputStream& output) {
			io::Write(output, mosaicId);
			io::Write(output, offer.Amount);
			io::Write(output, offer.InitialAmount);
			io::Write(output, offer.InitialCost);
			io::Write(output, offer.Deadline);
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

		void WriteExpiredSellOffers(const ExpiredSellOfferMap& offers, io::OutputStream& output) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(offers.size()));
			for (const auto& pair : offers) {
				io::Write(output, pair.first);
				WriteSellOffers(pair.second, output);
			}
		}

		void WriteExpiredBuyOffers(const ExpiredBuyOfferMap& offers, io::OutputStream& output) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(offers.size()));
			for (const auto& pair : offers) {
				io::Write(output, pair.first);
				WriteBuyOffers(pair.second, output);
			}
		}
	}

	void ExchangeEntryNonHistoricalSerializer::Save(const ExchangeEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write(output, entry.owner());

		WriteSellOffers(entry.sellOffers(), output);
		WriteBuyOffers(entry.buyOffers(), output);
	}

	void ExchangeEntrySerializer::Save(const ExchangeEntry& entry, io::OutputStream& output) {
		ExchangeEntryNonHistoricalSerializer::Save(entry, output);

		WriteExpiredSellOffers(entry.expiredSellOffers(), output);
		WriteExpiredBuyOffers(entry.expiredBuyOffers(), output);
	}

	namespace {
		void ReadOffer(MosaicId& mosaicId, OfferBase& offer, io::InputStream& input) {
			mosaicId = io::Read<MosaicId>(input);
			offer.Amount = io::Read<Amount>(input);
			offer.InitialAmount = io::Read<Amount>(input);
			offer.InitialCost = io::Read<Amount>(input);
			offer.Deadline = io::Read<Height>(input);
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

		void ReadExpiredSellOffers(ExpiredSellOfferMap& offers, io::InputStream& input) {
			auto offerCount = io::Read16(input);
			for (uint16_t i = 0; i < offerCount; ++i) {
				auto height = io::Read<Height>(input);
				SellOfferMap expiredOffers;
				ReadSellOffers(expiredOffers, input);
				offers.emplace(height, expiredOffers);
			}
		}

		void ReadExpiredBuyOffers(ExpiredBuyOfferMap& offers, io::InputStream& input) {
			auto offerCount = io::Read16(input);
			for (uint16_t i = 0; i < offerCount; ++i) {
				auto height = io::Read<Height>(input);
				BuyOfferMap expiredOffers;
				ReadBuyOffers(expiredOffers, input);
				offers.emplace(height, expiredOffers);
			}
		}
	}

	ExchangeEntry ExchangeEntryNonHistoricalSerializer::Load(io::InputStream& input) {
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

	ExchangeEntry ExchangeEntrySerializer::Load(io::InputStream& input) {
		auto entry = ExchangeEntryNonHistoricalSerializer::Load(input);

		ReadExpiredSellOffers(entry.expiredSellOffers(), input);
		ReadExpiredBuyOffers(entry.expiredBuyOffers(), input);

		return entry;
	}
}}

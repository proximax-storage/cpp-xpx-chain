/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaExchangeEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {
		void WriteOffer(const MosaicId& mosaicId, const SdaOfferBase& offer, io::OutputStream& output) {
			io::Write(output, mosaicId);
			io::Write(output, offer.CurrentMosaicGive);
			io::Write(output, offer.InitialMosaicGive);
			io::Write(output, offer.InitialMosaicGet);
			io::Write(output, offer.Deadline);
		}

		void WriteSwapOffers(const SwapOfferMap& offers, io::OutputStream& output) {
			io::Write8(output, utils::checked_cast<size_t, uint8_t>(offers.size()));
			for (const auto& pair : offers) {
				WriteOffer(pair.first, pair.second, output);
			}
		}

		void WriteExpiredSwapOffers(const ExpiredSwapOfferMap& offers, io::OutputStream& output) {
			io::Write16(output, utils::checked_cast<size_t, uint16_t>(offers.size()));
			for (const auto& pair : offers) {
				io::Write(output, pair.first);
				WriteSwapOffers(pair.second, output);
			}
		}

		void SaveSdaExchangeEntry(const SdaExchangeEntry& entry, io::OutputStream& output, bool saveExpiredOffers = true) {
			// write version
			io::Write32(output, entry.version());

			io::Write(output, entry.owner());

			WriteSwapOffers(entry.swapOffers(), output);

			if (saveExpiredOffers) {
				WriteExpiredSwapOffers(entry.expiredSwapOffers(), output);
			}
		}
	}

	void SdaExchangeEntrySerializer::Save(const SdaExchangeEntry& entry, io::OutputStream& output) {
		SaveSdaExchangeEntry(entry, output);
	}

	namespace {
		void ReadOffer(MosaicId& mosaicId, SdaOfferBase& offer, io::InputStream& input) {
			mosaicId = io::Read<MosaicId>(input);
			offer.CurrentMosaicGive = io::Read<Amount>(input);
			offer.InitialMosaicGive = io::Read<Amount>(input);
			offer.InitialMosaicGet = io::Read<Amount>(input);
			offer.Deadline = io::Read<Height>(input);
		}

		void ReadSwapOffers(SwapOfferMap& offers, io::InputStream& input) {
			auto sdaOfferCount = io::Read8(input);
			for (uint8_t i = 0; i < sdaOfferCount; ++i) {
				MosaicId mosaicId;
				SwapOffer offer;
				ReadOffer(mosaicId, offer, input);
				offers.emplace(mosaicId, offer);
			}
		}

		void ReadExpiredSwapOffers(ExpiredSwapOfferMap& offers, io::InputStream& input) {
			auto sdaOfferCount = io::Read16(input);
			for (uint16_t i = 0; i < sdaOfferCount; ++i) {
				auto height = io::Read<Height>(input);
				SwapOfferMap expiredOffers;
				ReadSwapOffers(expiredOffers, input);
				offers.emplace(height, expiredOffers);
			}
		}

		SdaExchangeEntry LoadSdaExchangeEntry(io::InputStream& input, const predicate<VersionType>& loadExpiredOffers) {
			// read version
			VersionType version = io::Read32(input);
			Key owner;
			io::Read(input, owner);
			state::SdaExchangeEntry entry(owner, version);

			ReadSwapOffers(entry.swapOffers(), input);

			if (loadExpiredOffers(version)) {
				ReadExpiredSwapOffers(entry.expiredSwapOffers(), input);
			}

			return entry;
		}
	}

	SdaExchangeEntry SdaExchangeEntryNonHistoricalSerializer::Load(io::InputStream& input) {
		return LoadSdaExchangeEntry(input, [](auto version) { return (1 == version); });
	}

	SdaExchangeEntry SdaExchangeEntrySerializer::Load(io::InputStream& input) {
		return LoadSdaExchangeEntry(input, [](auto) { return true; });
	}
}}

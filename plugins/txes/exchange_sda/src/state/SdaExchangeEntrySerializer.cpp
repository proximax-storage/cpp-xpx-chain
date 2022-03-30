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
        void WriteSdaOffer(const MosaicsPair& pair, const SdaOfferBalance& offer, io::OutputStream& output) {
            io::Write(output, pair.first);
            io::Write(output, pair.second);
            io::Write(output, offer.CurrentMosaicGive);
            io::Write(output, offer.CurrentMosaicGet);
            io::Write(output, offer.InitialMosaicGive);
            io::Write(output, offer.InitialMosaicGet);
            io::Write(output, offer.Deadline);
        }

        void WriteSdaOffers(const SdaOfferBalanceMap& offers, io::OutputStream& output) {
            io::Write8(output, utils::checked_cast<size_t, uint8_t>(offers.size()));
            for (const auto& pair : offers) {
                WriteSdaOffer(pair.first, pair.second, output);
            }
        }

        void WriteExpiredSdaOffers(const ExpiredSdaOfferBalanceMap& offers, io::OutputStream& output) {
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(offers.size()));
            for (const auto& pair : offers) {
                io::Write(output, pair.first);
                WriteSdaOffers(pair.second, output);
            }
        }

        void SaveSdaExchangeEntry(const SdaExchangeEntry& entry, io::OutputStream& output) {
            // write version
            io::Write32(output, entry.version());

            io::Write(output, entry.owner());

            WriteSdaOffers(entry.sdaOfferBalances(), output);
            WriteExpiredSdaOffers(entry.expiredSdaOfferBalances(), output);
        }
    }

    void SdaExchangeEntrySerializer::Save(const SdaExchangeEntry& entry, io::OutputStream& output) {
        SaveSdaExchangeEntry(entry, output);
    }

    namespace {
        void ReadSdaOffer(MosaicsPair& pair, SdaOfferBalance& offer, io::InputStream& input) {
            pair.first = io::Read<MosaicId>(input);
            pair.second = io::Read<MosaicId>(input);
            offer.CurrentMosaicGive = io::Read<Amount>(input);
            offer.InitialMosaicGive = io::Read<Amount>(input);
            offer.InitialMosaicGet = io::Read<Amount>(input);
            offer.Deadline = io::Read<Height>(input);
        }

        void ReadSdaOffers(SdaOfferBalanceMap& offers, io::InputStream& input) {
            auto sdaOfferCount = io::Read8(input);
            for (uint8_t i = 0; i < sdaOfferCount; ++i) {
                MosaicsPair pair;
                SdaOfferBalance offer;
                ReadSdaOffer(pair, offer, input);
                offers.emplace(pair, offer);
            }
        }

        void ReadExpiredSdaOffers(ExpiredSdaOfferBalanceMap& offers, io::InputStream& input) {
            auto sdaOfferCount = io::Read16(input);
            for (uint16_t i = 0; i < sdaOfferCount; ++i) {
                auto height = io::Read<Height>(input);
                SdaOfferBalanceMap expiredOffers;
                ReadSdaOffers(expiredOffers, input);
                offers.emplace(height, expiredOffers);
            }
        }

        SdaExchangeEntry LoadSdaExchangeEntry(io::InputStream& input) {
            // read version
            VersionType version = io::Read32(input);
            Key owner;
            io::Read(input, owner);
            state::SdaExchangeEntry entry(owner, version);

            ReadSdaOffers(entry.sdaOfferBalances(), input);
            ReadExpiredSdaOffers(entry.expiredSdaOfferBalances(), input);

            return entry;
        }
    }

    SdaExchangeEntry SdaExchangeEntrySerializer::Load(io::InputStream& input) {
        return LoadSdaExchangeEntry(input);
    }
}}

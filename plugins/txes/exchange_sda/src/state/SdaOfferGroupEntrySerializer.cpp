/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaOfferGroupEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

    namespace {
        void SaveSdaOfferGroup(io::OutputStream& output, const std::vector<SdaOffer>& offers) {
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(offers.size()));
            for (const auto& offer : offers) {
                io::Write(output, offer.Owner);
                io::Write(output, offer.MosaicGive);
                io::Write(output, offer.MosaicGet);
                io::Write(output, offer.Deadline);
            }
        }

        auto LoadSdaOfferGroup(io::InputStream& input, std::vector<SdaOffer>& offers) {
            auto count = io::Read16(input);
            while (count--) {
                SdaOffer offer;
                io::Read(input, offer.Owner);
                io::Read(input, offer.MosaicGive);
                io::Read(input, offer.MosaicGet);
                io::Read(input, offer.Deadline);
                offers.emplace_back(offer);
            }
        }
    }

    void SdaOfferGroupEntrySerializer::Save(const SdaOfferGroupEntry& sdaOfferGroupEntry, io::OutputStream& output) {
        io::Write(output, sdaOfferGroupEntry.groupHash());
        SaveSdaOfferGroup(output, sdaOfferGroupEntry.sdaOfferGroup());
    }

	SdaOfferGroupEntry SdaOfferGroupEntrySerializer::Load(io::InputStream& input) {
        Hash256 groupHash;
        state::SdaOfferGroupEntry entry(groupHash);
        input.read(groupHash);
		LoadSdaOfferGroup(input, entry.sdaOfferGroup());

        return entry;
	}
}}
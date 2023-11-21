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
        void SaveSdaOfferGroupEntry(const SdaOfferGroupEntry& entry, io::OutputStream& output) {
            io::Write(output, entry.groupHash());
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(entry.sdaOfferGroup().size()));
            for (const auto& offer : entry.sdaOfferGroup()) {
                io::Write(output, offer.Owner);
                io::Write(output, offer.MosaicGive);
                io::Write(output, offer.Deadline);
            }
        }

        SdaOfferGroupEntry LoadSdaOfferGroupEntry(io::InputStream& input) {
            Hash256 groupHash;
            io::Read(input, groupHash);
            state::SdaOfferGroupEntry entry(groupHash);
            auto count = io::Read16(input);
            while (count--) {
                SdaOfferBasicInfo offer;
                io::Read(input, offer.Owner);
                io::Read(input, offer.MosaicGive);
                io::Read(input, offer.Deadline);
                entry.sdaOfferGroup().emplace_back(offer);
            }

            return entry;
        }
    }

    void SdaOfferGroupEntrySerializer::Save(const SdaOfferGroupEntry& entry, io::OutputStream& output) {
        SaveSdaOfferGroupEntry(entry, output);
    }

	SdaOfferGroupEntry SdaOfferGroupEntrySerializer::Load(io::InputStream& input) {
		return LoadSdaOfferGroupEntry(input);
	}
}}
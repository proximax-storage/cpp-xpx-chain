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
        void SaveSdaOfferInfo(io::OutputStream& output, const std::vector<SdaOfferBasicInfo>& offers) {
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(offers.size()));
            for (const auto& offer : offers) {
                io::Write(output, offer.Owner);
                io::Write(output, offer.MosaicGive);
                io::Write(output, offer.Deadline);
            }
        }

        void SaveSdaOfferGroup(io::OutputStream& output, const SdaOfferGroupMap& offers) {
            io::Write16(output, utils::checked_cast<size_t, uint16_t>(offers.size()));
            for (const auto& pair : offers) {
                io::Write(output, pair.first);
                SaveSdaOfferInfo(output, pair.second);
            }
        }

        auto LoadSdaOfferGroup(io::InputStream& input, std::vector<SdaOfferBasicInfo>& offers) {
            auto count = io::Read16(input);
            while (count--) {
                SdaOfferBasicInfo offer;
                io::Read(input, offer.Owner);
                io::Read(input, offer.MosaicGive);
                io::Read(input, offer.Deadline);
                offers.emplace_back(offer);
            }
        }

        auto LoadSdaOfferGroup(io::InputStream& input, SdaOfferGroupMap& offers) {
            auto offerCount = io::Read16(input);
            for (uint16_t i = 0; i < offerCount; ++i) {
                Hash256 groupHash;
				io::Read(input, groupHash);
                std::vector<SdaOfferBasicInfo> info;
                LoadSdaOfferGroup(input, info);
                offers.emplace(groupHash, info);
            }
        }
    }

    void SdaOfferGroupEntrySerializer::Save(const SdaOfferGroupEntry& sdaOfferGroupEntry, io::OutputStream& output) {
        io::Write(output, sdaOfferGroupEntry.groupHash());
        SaveSdaOfferGroup(output, sdaOfferGroupEntry.sdaOfferGroup());
    }

	SdaOfferGroupEntry SdaOfferGroupEntrySerializer::Load(io::InputStream& input) {
        Hash256 groupHash;
        io::Read(input, groupHash);
        state::SdaOfferGroupEntry entry(groupHash);
		LoadSdaOfferGroup(input, entry.sdaOfferGroup());

        return entry;
	}
}}
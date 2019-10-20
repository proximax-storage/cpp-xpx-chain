/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OfferDeadlineEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/exceptions.h"

namespace catapult { namespace state {

	namespace {
		void WriteOfferDeadlines(const OfferDeadlineEntry::OfferDeadlineMap& deadlines, io::OutputStream& output) {
			io::Write64(output, static_cast<uint8_t>(deadlines.size()));
			for (const auto& pair : deadlines) {
				io::Write(output, pair.first);
				io::Write(output, pair.second);
			}
		}

		void WriteOfferHeights(const OfferDeadlineEntry::OfferHeightMap& heights, io::OutputStream& output) {
			io::Write64(output, static_cast<uint8_t>(heights.size()));
			for (const auto& pair : heights) {
				io::Write(output, pair.first);
				io::Write(output, pair.second);
			}
		}
	}

	void OfferDeadlineEntrySerializer::Save(const OfferDeadlineEntry& entry, io::OutputStream& output) {
		// write version
		io::Write32(output, 1);

		io::Write(output, entry.height());

		WriteOfferDeadlines(entry.buyOfferDeadlines(), output);
		WriteOfferHeights(entry.buyOfferHeights(), output);
		WriteOfferDeadlines(entry.sellOfferDeadlines(), output);
		WriteOfferHeights(entry.sellOfferHeights(), output);
	}

	namespace {
		void ReadOfferDeadlines(OfferDeadlineEntry::OfferDeadlineMap& deadlines, io::InputStream& input) {
			auto deadlineCount = io::Read64(input);
			for (uint8_t i = 0; i < deadlineCount; ++i) {
				auto deadline = io::Read<Timestamp>(input);
				auto hash = io::Read<utils::ShortHash>(input);
				deadlines.emplace(deadline, hash);
			}
		}

		void ReadOfferHeights(OfferDeadlineEntry::OfferHeightMap& heights, io::InputStream& input) {
			auto heightCount = io::Read64(input);
			for (uint8_t i = 0; i < heightCount; ++i) {
				auto height = io::Read<Height>(input);
				auto hash = io::Read<utils::ShortHash>(input);
				heights.emplace(height, hash);
			}
		}
	}

	OfferDeadlineEntry OfferDeadlineEntrySerializer::Load(io::InputStream& input) {
		// read version
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of OfferDeadlineEntry", version);

		auto height = io::Read<Height>(input);
		state::OfferDeadlineEntry entry(height);

		ReadOfferDeadlines(entry.buyOfferDeadlines(), input);
		ReadOfferHeights(entry.buyOfferHeights(), input);
		ReadOfferDeadlines(entry.sellOfferDeadlines(), input);
		ReadOfferHeights(entry.sellOfferHeights(), input);

		return entry;
	}
}}

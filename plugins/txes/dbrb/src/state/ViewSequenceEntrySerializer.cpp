/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ViewSequenceEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {

		void SaveProcessId(io::OutputStream& output, const dbrb::ProcessId& processId) {
			io::Write(output, processId);
		}

		void LoadProcessId(io::InputStream& input, dbrb::ProcessId& processId) {
			io::Read(input, processId);
		}

		void SaveView(io::OutputStream& output, const dbrb::View& view) {
			const auto& data = view.Data;

			io::Write16(output, utils::checked_cast<size_t, uint16_t>(data.size()));
			for (const auto& [processId, membershipChange] : data) {
				SaveProcessId(output, processId);
				io::Write8(output, utils::to_underlying_type(membershipChange));
			}
		}

		void LoadView(io::InputStream& input, dbrb::View& view) {
			auto& data = view.Data;

			auto numPairs = io::Read16(input);
			while (numPairs--) {
				dbrb::ProcessId processId;
				LoadProcessId(input, processId);
				const auto membershipChange = static_cast<dbrb::MembershipChange>(io::Read8(input));

				data.emplace(processId, membershipChange);
			}
		}
	}

	void ViewSequenceEntrySerializer::Save(const ViewSequenceEntry& entry, io::OutputStream& output) {
		io::Write32(output, entry.version());
		io::Write(output, entry.hash());

		const auto& sequenceData = entry.sequence().data();
		io::Write16(output, utils::checked_cast<size_t, uint16_t>(sequenceData.size()));
		for (const auto& view : sequenceData) {
			SaveView(output, view);
		}
	}

	ViewSequenceEntry ViewSequenceEntrySerializer::Load(io::InputStream& input) {
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of ViewSequenceEntry", version);

		Hash256 hash;
		io::Read(input, hash);
		state::ViewSequenceEntry entry(hash);
		entry.setVersion(version);

		std::vector<dbrb::View> sequenceData;
		auto numViews = io::Read16(input);
		while (numViews--) {
			dbrb::View view;
			LoadView(input, view);
			sequenceData.emplace_back(std::move(view));
		}

		const auto pSequence = dbrb::Sequence::fromViews(sequenceData);
		if (!pSequence.has_value())
			CATAPULT_THROW_RUNTIME_ERROR("invalid sequence data");
		entry.sequence() = *pSequence;

		return entry;
	}
}}

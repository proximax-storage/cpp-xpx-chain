/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbTestUtils.h"

namespace catapult { namespace test {

	dbrb::View GenerateRandomView(const size_t& length) {
		dbrb::ViewData viewData;
		std::set<dbrb::ProcessId> members;

		for (auto i = 0u; i < length; ++i) {
			if (members.empty() || (test::RandomByte() % 2)) {
				const auto newProcessId = test::GenerateRandomByteArray<dbrb::ProcessId>();
				viewData.emplace(newProcessId, dbrb::MembershipChange::Join);
				members.emplace(newProcessId);
			} else {
				const auto pos = test::RandomInRange(0ul, members.size() - 1);
				auto iter = members.begin();
				std::advance(iter, pos);

				const auto removedProcessId = *iter;
				viewData.emplace(removedProcessId, dbrb::MembershipChange::Leave);
				members.erase(removedProcessId);
			}
		}

		return dbrb::View{ viewData };
	}

	dbrb::Sequence GenerateRandomSequence(const dbrb::View& initialView, const size_t& length, const uint8_t& maxStep) {
		if (length == 0u)
			return dbrb::Sequence();

		dbrb::View view = initialView.Data.empty() ? GenerateRandomView() : initialView;
		std::set<dbrb::ProcessId> members = view.members();
		std::vector<dbrb::View> sequenceData{ view };

		for (auto i = 1u; i < length; ++i) {
			// Step is a difference in lengths of two subsequent views.
			const auto step = test::RandomInRange<uint8_t>(1, maxStep);
			for (auto j = 0u; j < step; ++j) {
				if (members.empty() || (test::RandomByte() % 2)) {
					const auto newProcessId = test::GenerateRandomByteArray<dbrb::ProcessId>();
					view.Data.emplace(newProcessId, dbrb::MembershipChange::Join);
					members.emplace(newProcessId);
				} else {
					const auto pos = test::RandomInRange(0ul, members.size() - 1);
					auto iter = members.begin();
					std::advance(iter, pos);

					const auto removedProcessId = *iter;
					view.Data.emplace(removedProcessId, dbrb::MembershipChange::Leave);
					members.erase(removedProcessId);
				}
			}
			sequenceData.emplace_back(view);
		}

		return *dbrb::Sequence::fromViews(sequenceData);
	}

	state::ViewSequenceEntry CreateViewSequenceEntry(const Hash256& hash, const dbrb::Sequence& sequence) {
		state::ViewSequenceEntry entry(hash);
		entry.sequence() = sequence;
		return entry;
	}

	state::MessageHashEntry CreateMessageHashEntry(const Hash256& hash) {
		return state::MessageHashEntry{ 0u, hash };
	}

}}

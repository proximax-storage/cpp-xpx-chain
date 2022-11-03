/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "catapult/ionet/Node.h"
#include "catapult/ionet/Packet.h"
#include "catapult/types.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <optional>
#include <cstdint>


namespace catapult { namespace fastfinality {

		using ProcessId = Key;
		using Payload = std::shared_ptr<ionet::Packet>;


		/// Changes of processes' membership in a view.
		enum MembershipChanges {
			Join,
			Leave,
		};

		/// State of the process' life cycle.
		enum ProcessState {
			NotJoined,
			Joining,
			Joined,
			Leaving,
			Left,
		};

		/// Message certificate.
		struct Certificate {};	// TODO: Stub


		/// View of the system.
		struct View {
			/// History of processes' membership changes in the system.
			std::vector<std::pair<ionet::Node, MembershipChanges>> Data;

			/// Get IDs of current members of the view.
			std::set<ionet::Node> members() const;

			/// Check if \a processId is a member of this view.
			bool hasChange(const ProcessId& processId, MembershipChanges change) const;

			/// Check if \a processId is a member of this view.
			bool isMember(const ProcessId& processId) const;

			/// Calculate quorum size of this view.
			size_t quorumSize() const;

			/// Comparison operators; if view A is less recent than view B, then A < B.
			bool operator==(const View& other) const;
			bool operator!=(const View& other) const;
			bool operator<(const View& other) const;
			bool operator>(const View& other) const;
			bool operator<=(const View& other) const;
			bool operator>=(const View& other) const;

			/// Check if two views are comparable.
			static bool areComparable(const View& a, const View& b) {
				return (a == b || a < b || a > b);
			}
		};

		/// Sequence of views of the system.
		struct Sequence {
			/// Sequence of mutually comparable views. Greater index means more recent view.
			std::vector<View> Data;

			/// Try to get the least recent view from the \a Data, if \a Data is not empty.
			std::optional<View> maybeLeastRecent() const;

			/// Try to get the most recent view from the \a Data, if \a Data is not empty.
			std::optional<View> maybeMostRecent() const;

			/// Get \a testedView's potential index on insertion into \a Data, if it is comparable with every view in \a Data.
			/// If not comparable with at least one view from Data, returns SIZE_MAX.
			size_t canInsert(const View&) const;

			/// Check whether \a testedView is the most recent among views in \a Data, and therefore can be appended.
			bool canAppend(const View&) const;

			/// Check whether all views in \a testedSequence are more recent than ones in \a Data, and therefore can be appended.
			bool canAppend(const Sequence&) const;

			/// Try to insert \a newView into \a Data.
			/// Returns a new sequence on success.
			std::optional<Sequence> tryInsert(const View&) const;

			/// Try to append \a newView to \a Data.
			/// Returns a new sequence on success.
			std::optional<Sequence> tryAppend(const View&) const;

			/// Try to append \a newSequence to \a Data.
			/// Returns a new sequence on success.
			std::optional<Sequence> tryAppend(const Sequence&) const;

			/// Check whether all views in \a sequence are mutually comparable and sorted in strictly ascending order.
			static bool isValidSequence(const std::vector<View>& sequenceData) {
				if (sequenceData.size() <= 1)
					return true;

				auto ptrA = sequenceData.begin();
				auto ptrB = sequenceData.begin() + 1;
				for (; ptrB != sequenceData.end(); ++ptrA, ++ptrB)
					if (!(*ptrA < *ptrB))
						return false;

				return true;
			}
		};

}}
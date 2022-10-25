#pragma  once
#include "DbrbUtils.h"


namespace catapult { namespace fastfinality {

		std::set<ProcessId> View::members() const {
			std::set<ProcessId> joined;
			std::set<ProcessId> left;
			std::set<ProcessId> current;

			for (const auto& [processId, changeType] : Data) {
				if (changeType == MembershipChanges::Join)
					joined.insert(processId);
				else // MembershipChanges::Leave
					left.insert(processId);
			}

			std::set_difference(
					joined.begin(),
					joined.end(),
					left.begin(),
					left.end(),
					std::inserter(current, current.begin()));

			return current;
		};

		bool View::isMember(const ProcessId& processId) const {
			return (bool)(members().count(processId));
		};

		size_t View::quorumSize() const {
			unsigned size = members().size();
			return size - (size - 1) / 3;
		}

		bool View::operator==(const View& other) const {
			return Data == other.Data;
		}

		bool View::operator!=(const View& other) const {
			return Data != other.Data;
		}

		bool View::operator<(const View& other) const {
			// If other is not longer, it cannot be more recent.
			if (other.Data.size() <= Data.size())
				return false;

			const std::vector<std::pair<ProcessId, MembershipChanges>> commonPart {
					other.Data.begin(), other.Data.begin() + Data.size()
			};

			// If other is longer, but common part differs, the views are not comparable.
			return Data == commonPart;
		}

		bool View::operator>(const View& other) const {
			// If *this is not longer, it cannot be more recent.
			if (Data.size() <= other.Data.size())
				return false;

			const std::vector<std::pair<ProcessId, MembershipChanges>> commonPart {
					Data.begin(), Data.begin() + other.Data.size()
			};

			// If *this is longer, but common part differs, the views are not comparable.
			return other.Data == commonPart;
		}

		bool View::operator<=(const View& other) const {
			return (*this < other) || (*this == other);
		}

		bool View::operator>=(const View& other) const {
			return (*this > other) || (*this == other);
		}

		std::optional<View> Sequence::maybeLeastRecent() const {
			if (!Data.empty())
				return Data.front();
			return {};
		}

		std::optional<View> Sequence::maybeMostRecent() const {
			if (!Data.empty())
				return Data.back();
			return {};
		}

		size_t Sequence::canInsert(const View& testedView) const {
			const auto pMostRecent = maybeMostRecent();
			if (!pMostRecent)
				return 0;	// Sequence is empty; any view can be inserted and will appear at index 0.

			// Tested view is comparable with every view in Data IFF it is comparable
			// with the biggest view in Data, i.e. the most recent one.
			if (!View::areComparable(testedView, *pMostRecent))
				return SIZE_MAX;

			// Since at this point we know that testedView is comparable with every view in Data,
			// we can now use comparison by size for faster processing.
			const auto testedViewSize = testedView.Data.size();
			auto pView = Data.begin();
			size_t pos = 0;
			for (; pView != Data.end(); ++pos, ++pView) {
				if (testedViewSize == pView->Data.size())
					return SIZE_MAX;	// Duplicate means non-strictly ascending order.
				if (testedViewSize < pView->Data.size())
					return pos;
			}

			// testedView is the most recent view and is appended to the end of Data.
			return pos;
		}

		bool Sequence::canAppend(const View& testedView) const {
			const auto pMostRecent = maybeMostRecent();
			if (!pMostRecent)
				return true;	// Sequence is empty; any view can be appended.

			// Tested view can be appended to Data IFF it is strictly greater than the most recent view from Data.
			return testedView > *pMostRecent;
		}

		bool Sequence::canAppend(const Sequence& testedSequence) const {
			const auto pThisMostRecent = maybeMostRecent();
			const auto pOtherLeastRecent = testedSequence.maybeLeastRecent();
			if (!pThisMostRecent || !pOtherLeastRecent)
				return true;	// If either sequence is empty, append is always legal.

			return *pThisMostRecent < *pOtherLeastRecent;
		}

		std::optional<Sequence> Sequence::tryInsert(const View& newView) const {
			const size_t pos = canInsert(newView);
			if (pos != SIZE_MAX) {
				auto sequenceData = Data;
				sequenceData.insert(sequenceData.begin() + pos, newView);
				return Sequence { sequenceData };
			}
			return {};
		}

		std::optional<Sequence> Sequence::tryAppend(const View& newView) const {
			if (canAppend(newView)) {
				auto sequenceData = Data;
				sequenceData.push_back(newView);
				return Sequence { sequenceData };
			}
			return {};
		}

		std::optional<Sequence> Sequence::tryAppend(const Sequence& newSequence) const {
			if (canAppend(newSequence)) {
				auto sequenceData = Data;
				sequenceData.insert(sequenceData.end(), newSequence.Data.begin(), newSequence.Data.end());
				return Sequence { sequenceData };
			}
			return {};
		}

}}
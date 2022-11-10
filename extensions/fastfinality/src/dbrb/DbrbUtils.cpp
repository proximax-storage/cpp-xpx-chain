/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbUtils.h"


namespace catapult { namespace dbrb {

	std::set<ProcessId> View::members() const {
		std::set<ProcessId> joined;
		std::set<ProcessId> left;
		std::set<ProcessId> current;

		for (const auto& [node, changeType] : Data) {
			if (changeType == MembershipChanges::Join)
				joined.insert(node);
			else // MembershipChanges::Leave
				left.insert(node);
		}

		std::set_difference(joined.begin(), joined.end(),
							left.begin(), left.end(),
							std::inserter(current, current.begin()));

		return current;
	}

	bool View::hasChange(const ProcessId& processId, MembershipChanges change) const {
		return std::find_if(Data.cbegin(), Data.cend(), [&processId, change](const auto& pair) { return pair.first == processId && pair.second == change; }) != Data.cend();
	}

	bool View::isMember(const ProcessId& processId) const {
		return std::find_if(Data.cbegin(), Data.cend(), [&processId](const auto& pair) { return pair.first == processId && pair.second == MembershipChanges::Join; }) != Data.cend();
	}

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

		const std::vector<std::pair<ProcessId, MembershipChanges>> commonPart{ other.Data.cbegin(), other.Data.cend() };

		// If other is longer, but common part differs, the views are not comparable.
		return Data == commonPart;
	}

	bool View::operator>(const View& other) const {
		// If *this is not longer, it cannot be more recent.
		if (Data.size() <= other.Data.size())
			return false;

		const std::vector<std::pair<ProcessId, MembershipChanges>> commonPart{ Data.cbegin(), Data.cend() };

		// If *this is longer, but common part differs, the views are not comparable.
		return other.Data == commonPart;
	}

	bool View::operator<=(const View& other) const {
		return (*this < other) || (*this == other);
	}

	bool View::operator>=(const View& other) const {
		return (*this > other) || (*this == other);
	}

	const std::vector<View>& Sequence::data() const {
		return m_data;
	}

	std::optional<View> Sequence::maybeLeastRecent() const {
		if (!m_data.empty())
			return m_data.front();
		return {};
	}

	std::optional<View> Sequence::maybeMostRecent() const {
		if (!m_data.empty())
			return m_data.back();
		return {};
	}

	int64_t Sequence::canInsert(const View& testedView) const {
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
		auto pView = m_data.begin();
		int64_t pos = 0;
		for (; pView != m_data.end(); ++pos, ++pView) {
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

	std::vector<View>::iterator Sequence::tryInsert(const View& newView) {
		const int64_t pos = canInsert(newView);
		if (pos != SIZE_MAX)
			return m_data.insert(m_data.begin() + pos, newView);
		else
			return m_data.end();
	}

	bool Sequence::tryAppend(const View& newView) {
		const bool appendable = canAppend(newView);
		if (appendable)
			m_data.push_back(newView);

		return appendable;
	}

	bool Sequence::tryAppend(const Sequence& newSequence) {
		const bool appendable = canAppend(newSequence);
		if (appendable)
			m_data.insert(m_data.end(), newSequence.data().begin(), newSequence.data().end());

		return appendable;
	}

	bool Sequence::tryErase(const View& view) {
		const auto iter = std::find(m_data.begin(), m_data.end(), view);
		const bool found = (iter != m_data.end());
		if (found)
			m_data.erase(iter);

		return found;
	}

	bool Sequence::operator<(const Sequence& rhs) const {
		return m_data.size() < rhs.m_data.size();
	}

	Sequence::Sequence(const std::vector<View>& sequenceData) {
		m_data = sequenceData;
	}
}}
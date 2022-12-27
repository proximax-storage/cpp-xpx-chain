/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "catapult/ionet/NetworkNode.h"
#include "catapult/ionet/Node.h"
#include "catapult/ionet/Packet.h"
#include "catapult/types.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <optional>
#include <cstdint>


namespace catapult { namespace dbrb {

	using ProcessId = ionet::Node;
	using Payload = std::shared_ptr<ionet::Packet>;
	using PackedView = std::vector<std::pair<model::UniqueEntityPtr<ionet::NetworkNode>, uint8_t>>;

	struct View;
	PackedView PackView(const View& view, uint32_t& payloadSize);
	void CopyView(uint8_t*& pData, const PackedView& nodes);
	View UnpackView(const uint8_t*& pData);
	void CopyProcessId(uint8_t*& pData, const model::UniqueEntityPtr<ionet::NetworkNode>& processId);
	ProcessId UnpackProcessId(const uint8_t*& pData);

	Hash256 CalculateHash(const std::vector<RawBuffer>& buffers);
	Hash256 CalculatePayloadHash(const Payload& payload);

	/// Changes of processes' membership in a view.
	enum class MembershipChanges : uint8_t {
		Join,
		Leave,
	};

	/// State of the process membership.
	enum class MembershipStates : uint8_t {
		NotJoined,
		Joining,
		Participating,
		Leaving,
		Left,
	};

	/// View of the system.
	struct View {
		/// Set of processes' membership changes in the system.
		std::set<std::pair<ProcessId, MembershipChanges>> Data;

		/// Get IDs of current members of the view.
		std::set<ProcessId> members() const;

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

		/// Insertion operator for outputting \a view to \a out.
		friend std::ostream& operator<<(std::ostream& out, const View& view) {
			bool leadingSpace = false;

			out << "[";
			for (const auto& [processId, change] : view.Data) {
				if (leadingSpace)
					out << " ";

				out << (change == MembershipChanges::Join ? "+" : "-");

				std::ostringstream processIdStringStream;
				processIdStringStream << processId;
				const std::string processIdString = processIdStringStream.str();

				const auto firstDigitPos = processIdStringStream.str().find_first_of("0123456789");
				out << processIdString.substr(0, 1) + processIdString.at(firstDigitPos);

				leadingSpace = true;
			}
			out << "]";

			return out;
		}

		/// Merge other view into this view.
		View& merge(const View& other);

		/// Remove from this view's \a Data all changes that appear in the other view.
		View& difference(const View& other);

		/// Merge two views into one.
		static View merge(const View& a, const View& b) {
			std::set<std::pair<ProcessId, MembershipChanges>> newData;
			for (const auto& change : a.Data)
				newData.insert(change);
			for (const auto& change : b.Data)
				newData.insert(change);
			return View{ std::move(newData) };
		}

		/// Check if two views are comparable.
		static bool areComparable(const View& a, const View& b) {
			return (a == b || a < b || a > b);
		}
	};

	/// Sequence of views of the system.
	class Sequence {
	public:
		/// Construct an empty \c Sequence.
		Sequence() = default;

		/// Get underlying sequence of views.
		const std::vector<View>& data() const;

		/// Try to get the least recent view from the \a m_data, if \a m_data is not empty.
		std::optional<View> maybeLeastRecent() const;

		/// Try to get the most recent view from the \a m_data, if \a m_data is not empty.
		std::optional<View> maybeMostRecent() const;

		/// Get \a testedView's potential index on insertion into \a m_data, if it is comparable with every view in \a m_data.
		/// If not comparable with at least one view from \a m_data, returns SIZE_MAX.
		int64_t canInsert(const View& testedView) const;

		/// Check whether \a testedView is the most recent among views in \a m_data, and therefore can be appended.
		bool canAppend(const View& testedView) const;

		/// Check whether all views in \a testedSequence are more recent than ones in \a m_data, and therefore can be appended.
		bool canAppend(const Sequence& testedSequence) const;

		/// Try to insert \a newView into \a m_data.
		/// Returns iterator to the inserted view on success, or iterator to the end of \a m_data otherwise.
		std::vector<View>::iterator tryInsert(const View& newView);

		/// Try to append \a newView to \a m_data.
		/// Returns \c true is appended successfully, \c false otherwise.
		bool tryAppend(const View& newView);

		/// Try to append \a newSequence to \a m_data.
		/// Returns \c true is appended successfully, \c false otherwise.
		bool tryAppend(const Sequence& newSequence);

		/// Try to erase \a view from \a m_data.
		/// Returns whether the view was found and erased.
		bool tryErase(const View& view);

		bool operator==(const Sequence& rhs) const;
		bool operator<(const Sequence& rhs) const;

		/// Check whether all views in \a sequenceData are mutually comparable and sorted in strictly ascending order.
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

		/// Create \c Sequence object from \a sequenceData, if \a sequenceData is a valid sequence.
		static std::optional<Sequence> fromViews(const std::vector<View>& sequenceData) {
			if (Sequence::isValidSequence(sequenceData))
				return Sequence { sequenceData };
			return {};
		}

		/// Insertion operator for outputting \a view to \a out.
		friend std::ostream& operator<<(std::ostream& out, const Sequence& sequence) {
			bool leadingComma = false;
			for (const auto& view : sequence.m_data) {
				if (leadingComma)
					out << ", ";

				out << view;
				leadingComma = true;
			}

			return out;
		}

	private:
		/// Construct a \c Sequence object using valid \a sequenceData.
		explicit Sequence(const std::vector<View>& sequenceData);

		/// Sequence of mutually comparable views. Greater index means more recent view.
		std::vector<View> m_data;
	};

	/// Struct that stores Install message data in unwrapped form.
	struct InstallMessageData {
		/// Least recent view.
		View LeastRecentView;

		/// Sequence that is converged on to replace the view.
		Sequence ConvergedSequence;

		/// View to be replaced.
		View ReplacedView;

		// ConvergedSignatures need no additional processing
		// and can be accessed directly from the Install message.
	};
}}
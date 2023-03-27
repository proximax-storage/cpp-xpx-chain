/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "DbrbViewFetcher.h"
#include "catapult/ionet/Packet.h"
#include "catapult/types.h"
#include <algorithm>
#include <map>
#include <set>
#include <vector>
#include <optional>
#include <cstdint>

namespace catapult { namespace dbrb {

	using Payload = std::shared_ptr<ionet::Packet>;
	using CertificateType = std::map<ProcessId, Signature>;

	/// State of the process membership.
	enum class MembershipState : uint8_t {
		NotJoined,
		Joining,
		Participating,
		Leaving,
		Left,
	};

	void Write(uint8_t*& pBuffer, const utils::RawBuffer& data);
	void Write(uint8_t*& pBuffer, uint8_t byte);
	struct View;
	void Write(uint8_t*& pBuffer, const View& view);
	struct Sequence;
	void Write(uint8_t*& pBuffer, const Sequence& sequence);
	void Write(uint8_t*& pBuffer, const Payload& payload);
	void Write(uint8_t*& pBuffer, const CertificateType& certificate);

	template<typename T>
	T Read(const uint8_t*& pBuffer);
	template<>
	ProcessId Read(const uint8_t*& pBuffer);
	template<>
	Signature Read(const uint8_t*& pBuffer);
	template<>
	Hash256 Read(const uint8_t*& pBuffer);
	template<>
	MembershipChange Read(const uint8_t*& pBuffer);
	template<>
	View Read(const uint8_t*& pBuffer);
	template<>
	Sequence Read(const uint8_t*& pBuffer);
	template<>
	Payload Read(const uint8_t*& pBuffer);
	template<>
	CertificateType Read(const uint8_t*& pBuffer);

	Hash256 CalculateHash(const std::vector<RawBuffer>& buffers);
	Hash256 CalculatePayloadHash(const Payload& payload);

	std::string MembershipChangeToString(const ProcessId& processId, const MembershipChange& change);

	/// View of the system.
	struct View {
		/// Set of processes' membership changes in the system.
		ViewData Data;

		/// Get IDs of current members of the view.
		std::set<ProcessId> members() const;

		/// Check if \a processId is a member of this view.
		bool hasChange(const ProcessId& processId, MembershipChange change) const;

		/// Check if \a processId is a member of this view.
		bool isMember(const ProcessId& processId) const;

		/// Calculate quorum size of this view.
		size_t quorumSize() const;

		/// Calculate the size in bytes required to serialize this view.
		size_t packedSize() const;

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

				out << MembershipChangeToString(processId, change);
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
			ViewData newData;
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

		/// Calculate the size in bytes required to serialize this view.
		size_t packedSize() const;

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

		/// Check whether every view in \a testedSequence can be inserted into current sequence without conflicts.
		bool canMerge(const Sequence& testedSequence) const;

		/// Try to insert \a newView into \a m_data.
		/// Returns iterator to the inserted view on success, or iterator to the end of \a m_data otherwise.
		std::vector<View>::iterator tryInsert(const View& newView);

		/// Try to append \a newView to \a m_data.
		/// Returns \c true is appended successfully, \c false otherwise.
		bool tryAppend(const View& newView);

		/// Try to append \a newSequence to \a m_data.
		/// Returns \c true is appended successfully, \c false otherwise.
		bool tryAppend(const Sequence& newSequence);

		/// Try to merge \a newSequence with \a m_data.
		/// Returns \c true is merged successfully, \c false otherwise.
		bool tryMerge(const Sequence& newSequence);

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

		/// Insertion operator for outputting \a sequence to \a out.
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
		/// Most recent view.
		View MostRecentView;

		/// Sequence that is converged on to replace the view.
		Sequence ConvergedSequence;

		/// View to be replaced.
		View ReplacedView;

		/// Map of processes and their signatures for appropriate Converged messages for Sequence.
		CertificateType ConvergedSignatures;
	};
}}
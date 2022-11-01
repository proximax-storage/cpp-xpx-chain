#pragma  once
#include <algorithm>
#include <map>
#include <set>
#include <vector>


namespace catapult { namespace fastfinality {

		using ProcessId = uint64_t; // Any kind of process identifier; can be Key
		using Payload = std::vector<uint8_t>;


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
			std::vector<std::pair<ProcessId, MembershipChanges>> Data;

			/// Get IDs of current members of the view.
			std::set<ProcessId> members() const;

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
			size_t canInsert(const View& testedView) const;

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

		private:
			/// Construct a \c Sequence object using valid \a sequenceData.
			explicit Sequence(const std::vector<View>& sequenceData);

			/// Sequence of mutually comparable views. Greater index means more recent view.
			std::vector<View> m_data;
		};

}}
/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma  once
#include "DbrbDefinitions.h"
#include "DbrbUtils.h"

namespace catapult { namespace dbrb {

	/// View of the system.
	struct View {
		/// Set of IDs of current members of the view.
		ViewData Data;

		/// Check if \a processId is a member of this view.
		bool isMember(const ProcessId& processId) const;

		/// Calculate quorum size of this view.
		size_t quorumSize() const;

		/// Calculate the size in bytes required to serialize this view.
		size_t packedSize() const;

		/// Merge other view into this view.
		View& merge(View& other);

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
			for (const auto& processId : view.Data) {
				if (leadingSpace)
					out << " ";

				out << ProcessIdToString(processId);
				leadingSpace = true;
			}
			out << "]";

			return out;
		}
	};
}}
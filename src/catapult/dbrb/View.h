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
		/// Calculates the maximum number of invalid processes that is allowed in a view of \a viewSize.
		static size_t maxInvalidProcesses(size_t viewSize);

		/// Calculates quorum size in a view of \a viewSize.
		static size_t quorumSize(size_t viewSize);

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
	};
}}
/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"
#include "extensions/fastfinality/src/dbrb/DbrbUtils.h"

namespace catapult { namespace state {

	// Mixin for storing view sequence details.
	class ViewSequenceMixin {
	public:
		ViewSequenceMixin()
				: m_sequence()
		{}

	public:
		/// Gets underlying sequence.
		const dbrb::Sequence& sequence() const {
			return m_sequence;
		}

		/// Gets underlying sequence.
		dbrb::Sequence& sequence() {
			return m_sequence;
		}

		/// Gets the most recent view in the underlying sequence, if it exists.
		/// Returns an empty view otherwise.
		const dbrb::View& mostRecentView() const {
			const auto pView = m_sequence.maybeMostRecent();
			return pView.value_or(dbrb::View());
		}

	private:
		dbrb::Sequence m_sequence;
	};

	// View sequence entry.
	class ViewSequenceEntry : public ViewSequenceMixin {
	public:
		// Creates a view sequence entry around \a hash.
		explicit ViewSequenceEntry(const Hash256& hash) : m_hash(hash), m_version(1)
		{}

	public:
		// Gets the hash of the corresponding Install message.
		const Hash256& hash() const {
			return m_hash;
		}

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

	private:
		Hash256 m_hash;
		VersionType m_version;
	};
}}

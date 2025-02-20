/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CommitteeEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading committee entry data.
	struct CommitteeEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const CommitteeEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static CommitteeEntry Load(io::InputStream& input);
	};

	/// Policy for saving and loading committee entry data for patricia tree.
	struct CommitteeEntryPatriciaTreeSerializer {
		/// Saves \a entry to \a output.
		static void Save(const CommitteeEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static CommitteeEntry Load(io::InputStream& input);
	};
}}

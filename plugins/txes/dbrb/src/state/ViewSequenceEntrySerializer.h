/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "ViewSequenceEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading view sequence entry data.
	struct ViewSequenceEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const ViewSequenceEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static ViewSequenceEntry Load(io::InputStream& input);
	};
}}

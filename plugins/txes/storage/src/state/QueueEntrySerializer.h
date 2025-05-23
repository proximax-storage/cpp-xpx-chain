/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "QueueEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {

	/// Policy for saving and loading drive entry data.
	struct QueueEntrySerializer {
		/// Saves \a entry to \a output.
		static void Save(const QueueEntry& entry, io::OutputStream& output);

		/// Loads a single value from \a input.
		static QueueEntry Load(io::InputStream& input);
	};
}}

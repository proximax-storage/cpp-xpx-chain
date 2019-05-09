/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "MetadataEntry.h"
#include "catapult/io/Stream.h"

namespace catapult { namespace state {
	/// Policy for saving and loading metadata.
	struct MetadataSerializer {
		/// Saves \a metadata to \a output.
		static void Save(const MetadataEntry& metadata, io::OutputStream& output);

		/// Loads a single value from \a input.
		static MetadataEntry Load(io::InputStream& input);
	};
}}

/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"

namespace catapult { namespace state {

	/// A catapult metadata field.
	struct MetadataField {
		/// Key of metadata field
		std::string MetadataKey;

		/// Value of metadata field
		std::string MetadataValue;

		/// If RemoveHeight is zero it means that field is not removed
		Height RemoveHeight;
	};
}}

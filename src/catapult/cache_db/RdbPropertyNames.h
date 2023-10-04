/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include <array>

namespace catapult { namespace cache { namespace property_names {

	constexpr auto SIZE_PROPERTY = "size";
	constexpr auto ROOT_PROPERTY = "root";

	constexpr std::array<const char*, 2> AllProperties = {
		SIZE_PROPERTY,
		ROOT_PROPERTY,
	};
}}}

/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <algorithm>
#include <vector>

namespace catapult::utils {

	template<class T>
	T median(std::vector<T> values) {
		std::sort(values.begin(), values.end());
		return values[values.size() / 2];
	}

}
/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/TestHarness.h"
#include "src/utils/MathUtils.h"

namespace catapult { namespace utils {

#define TEST_CLASS MathUtilsTests

	TEST(TEST_CLASS, Sqrt) {
		for (uint i = 0; i < 10000; i++) {
			auto v = test::Random();
			auto r = sqrt(v);
			ASSERT_LE(r * r, v);
			ASSERT_GT((r + 1) * (r + 1), v);
		}
	}
}}
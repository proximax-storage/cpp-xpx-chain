/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/mappers/OperationEntryMapper.h"
#include "mongo/plugins/lock_shared/tests/mappers/LockInfoMapperTests.h"
#include "tests/test/MongoOperationTestTraits.h"
#include "tests/test/OperationMapperTestUtils.h"
#include "tests/TestHarness.h"
#include <mongocxx/client.hpp>

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS OperationEntryMapperTests

	namespace {
		struct OperationEntryMapperTraits : public test::MongoOperationTestTraits {
			static constexpr auto AssertEqualLockInfoData = test::AssertEqualMongoOperationData;
			static constexpr char Doc_Name[] = "operation";
		};
	}

	DEFINE_LOCK_INFO_MAPPER_TESTS(OperationEntryMapperTraits)
}}}

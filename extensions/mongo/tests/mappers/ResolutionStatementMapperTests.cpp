/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "mongo/src/mappers/ResolutionStatementMapper.h"
#include "mongo/tests/test/MongoReceiptTestUtils.h"

namespace catapult { namespace mongo { namespace mappers {

#define TEST_CLASS ResolutionStatementMapperTests

	namespace {
		template<typename TTraits>
		void AssertCanMapResolutionStatement(size_t numResolutions) {
			// Arrange:
			auto unresolved = TTraits::CreateUnresolved(213);
			typename TTraits::ResolutionStatementType statement(unresolved);
			for (uint8_t i = 0; i < numResolutions; ++i)
				statement.addResolution(TTraits::CreateResolved(i), { 123u + i, 234u + i });

			// Act:
			auto document = ToDbModel(Height(567), statement);

			// Assert:
			TTraits::AssertResolutionStatement(statement, Height(567), document.view(), 3, 0);
		}
	}

#define RESOLUTION_STATEMENT_MAPPER_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_Address) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<test::AddressResolutionTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Mosaic) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<test::MosaicResolutionTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	RESOLUTION_STATEMENT_MAPPER_TEST(CanMapResolutionStatement_NoResolutions) {
		// Assert:
		AssertCanMapResolutionStatement<TTraits>(0);
	}

	RESOLUTION_STATEMENT_MAPPER_TEST(CanMapResolutionStatement_SingleResolution) {
		// Assert:
		AssertCanMapResolutionStatement<TTraits>(1);
	}

	RESOLUTION_STATEMENT_MAPPER_TEST(CanMapResolutionStatement_MultipleResolutions) {
		// Assert:
		AssertCanMapResolutionStatement<TTraits>(5);
	}
}}}

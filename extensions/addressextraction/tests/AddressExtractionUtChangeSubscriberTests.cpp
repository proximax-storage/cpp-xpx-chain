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

#include "addressextraction/src/AddressExtractionUtChangeSubscriber.h"
#include "addressextraction/tests/test/AddressExtractionSubscriberTestContext.h"

namespace catapult { namespace addressextraction {

#define TEST_CLASS AddressExtractionUtChangeSubscriberTests

	namespace {
		class TestContext : public test::AddressExtractionSubscriberTestContext<cache::UtChangeSubscriber> {
		public:
			TestContext() : AddressExtractionSubscriberTestContext(CreateAddressExtractionUtChangeSubscriber)
			{}
		};
	}

	TEST(TEST_CLASS, NotifyAddsExtractsTransactionAddresses) {
		// Act + Assert:
		TestContext().assertTransactionInfosExtractions([](auto& subscriber, const auto& transactionInfos) {
			subscriber.notifyAdds(transactionInfos);
		});
	}

	TEST(TEST_CLASS, NotifyRemovesExtractsTransactionAddresses) {
		// Act + Assert:
		TestContext().assertTransactionInfosExtractions([](auto& subscriber, const auto& transactionInfos) {
			subscriber.notifyRemoves(transactionInfos);
		});
	}

	TEST(TEST_CLASS, FlushDoesNotExtractTransactionAddresses) {
		// Act + Assert:
		TestContext().assertNoExtractions([](auto& subscriber) {
			subscriber.flush();
		});
	}
}}

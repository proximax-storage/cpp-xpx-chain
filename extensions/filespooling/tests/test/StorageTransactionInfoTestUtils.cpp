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

#include "StorageTransactionInfoTestUtils.h"
#include "catapult/io/TransactionInfoSerializer.h"
#include "tests/test/core/mocks/MockMemoryStream.h"

namespace catapult { namespace test {

	model::TransactionInfosSet CreateTransactionInfosSetWithOptionalAddresses(size_t count) {
		auto transactionInfos = test::CreateTransactionInfosWithOptionalAddresses(count);
		return test::CopyTransactionInfosToSet(transactionInfos);
	}

	std::vector<uint8_t> SerializeTransactionInfo(const model::TransactionInfo& transactionInfo) {
		std::vector<uint8_t> buffer;
		mocks::MockMemoryStream stream(buffer);

		io::WriteTransactionInfo(stream, transactionInfo);
		return buffer;
	}
}}

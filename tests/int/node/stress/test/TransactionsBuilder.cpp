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

#include "TransactionsBuilder.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "plugins/txes/namespace/src/model/NamespaceIdGenerator.h"
#include "tests/test/local/RealTransactionFactory.h"
#include "tests/test/nodeps/Nemesis.h"

namespace catapult { namespace test {



	// region ctor

	TransactionsBuilder::TransactionsBuilder(const Accounts& accounts) : m_accounts(accounts)
	{}

	// endregion

	// region TransactionsGenerator

	size_t TransactionsBuilder::size() const {
		return m_transactionDescriptorPairs.size();
	}

	model::UniqueEntityPtr<model::Transaction> TransactionsBuilder::generate(uint32_t descriptorType, const std::shared_ptr<const void>& pDescriptor, Timestamp deadline) const {
		return m_generators.find(descriptorType)->second(pDescriptor, deadline);
	}
	model::UniqueEntityPtr<model::Transaction> TransactionsBuilder::generateAt(size_t index, Timestamp deadline) const {
		const auto& pair = m_transactionDescriptorPairs[index];
		auto pTransaction = generate(pair.first, pair.second, deadline);
		if (pTransaction)
			return pTransaction;

		CATAPULT_THROW_INVALID_ARGUMENT_1("cannot generate unknown transaction type", pair.first);
	}

	// endregion



	// region protected

	const Accounts& TransactionsBuilder::accounts() const {
		return m_accounts;
	}

	const std::pair<uint32_t, std::shared_ptr<const void>>& TransactionsBuilder::getAt(size_t index) const {
		return m_transactionDescriptorPairs[index];
	}

	// endregion


}}

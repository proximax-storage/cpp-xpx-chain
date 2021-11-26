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

#pragma once
#include "Accounts.h"
#include "TransactionsGenerator.h"
#include <vector>

namespace catapult { namespace test {

	/// Basic transactions builder and generator for transfer transactions.
	using TransactionGenerator = std::function<model::UniqueEntityPtr<model::Transaction>(const std::shared_ptr<const void>&, Timestamp)>;
	class TransactionBuilderCapability;

	class TransactionsBuilder : public TransactionsGenerator {
		friend TransactionBuilderCapability;
	public:
		/// Creates a builder around \a accounts.
		explicit TransactionsBuilder(const Accounts& accounts);

	public:
		// TransactionsGenerator
		size_t size() const override;
		model::UniqueEntityPtr<model::Transaction> generateAt(size_t index, Timestamp deadline) const override;

	public:

		template<typename TDescriptorType>
		void registerDescriptor(TDescriptorType descriptorType, TransactionGenerator generator)
		{
			m_generators.emplace(utils::to_underlying_type(descriptorType), std::move(generator));
		}

		template<typename TCapability>
		std::shared_ptr<TCapability> getCapability()
		{
			auto capability = std::make_shared<TCapability>(*this);
			capability->registerHooks();
			return capability;
		}

	protected:
		/// Gets accounts.
		const Accounts& accounts() const;

		/// Gets transaction descriptor pair at \ index.
		const std::pair<uint32_t, std::shared_ptr<const void>>& getAt(size_t index) const;

		/// Adds transaction described by \a descriptor with descriptor type (\a descriptorType).
		template<typename TDescriptorType, typename TDescriptor>
		void add(TDescriptorType descriptorType, const TDescriptor& descriptor) {
			m_transactionDescriptorPairs.emplace_back(
					utils::to_underlying_type(descriptorType),
					std::make_shared<TDescriptor>(descriptor));
		}

	private:
		/// Generates transaction with specified \a deadline given descriptor (\a pDescriptor) and descriptor type (\a descriptorType).
		model::UniqueEntityPtr<model::Transaction> generate(
				uint32_t descriptorType,
				const std::shared_ptr<const void>& pDescriptor,
				Timestamp deadline) const;

	private:
		const Accounts& m_accounts;

		std::map<uint32_t, TransactionGenerator> m_generators;
		std::vector<std::pair<uint32_t, std::shared_ptr<const void>>> m_transactionDescriptorPairs;
	};
}}

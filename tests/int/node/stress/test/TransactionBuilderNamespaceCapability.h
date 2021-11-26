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
#include "TransactionsBuilder.h"
#include "TransactionBuilderCapability.h"
#include "TransactionsGenerator.h"


namespace catapult { namespace test {


	class TransactionBuilderNamespaceCapability : public TransactionBuilderCapability
	{

	private:
		struct NamespaceDescriptor {
			size_t OwnerId;
			std::string Name;
			BlockDuration Duration;
			size_t AddressAliasId; // optional
		};

		struct NetworkConfigDescriptor {
			std::string NetworkConfig;
			std::string SupportedEntities;
			BlockDuration BlocksBeforeActive;
		};

	public:
		/// Adds a root namespace registration for namespace \a name by \a ownerId for specified \a duration,
		/// optionally setting an alias for \a aliasId.
		void addNamespace(size_t ownerId, const std::string& name, BlockDuration duration, size_t aliasId = 0);

	public:
		TransactionBuilderNamespaceCapability(TransactionsBuilder& builder) : TransactionBuilderCapability(builder)
		{

		}
		void registerHooks() override;
	private:
		model::UniqueEntityPtr<model::Transaction> createRegisterNamespace(const NamespaceDescriptor& descriptor, Timestamp deadline);

		model::UniqueEntityPtr<model::Transaction> createAddressAlias(const NamespaceDescriptor& descriptor, Timestamp deadline);



	};
}}

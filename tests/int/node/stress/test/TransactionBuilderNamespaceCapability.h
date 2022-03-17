/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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

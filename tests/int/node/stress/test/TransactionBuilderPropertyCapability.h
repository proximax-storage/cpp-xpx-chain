/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionsBuilder.h"
#include "TransactionBuilderCapability.h"

namespace catapult { namespace test {


	/// Transactions builder capability for property transactions.
	class TransactionBuilderPropertyCapability : public TransactionBuilderCapability
	{

	private:
		struct PropertyAddressBlockDescriptor {
			size_t SenderId;
			size_t PartnerId;
			bool IsAdd;
		};
	public:
		/// Adds a property that blocks \a partnerId from sending to \a senderId.
		void addAddressBlockProperty(size_t senderId, size_t partnerId);

		/// Adds a property that unblocks \a partnerId from sending to \a senderId.
		void addAddressUnblockProperty(size_t senderId, size_t partnerId);

	public:
		TransactionBuilderPropertyCapability(TransactionsBuilder&  builder) : TransactionBuilderCapability(builder)
		{

		}
		void registerHooks() override;
	private:
		model::UniqueEntityPtr<model::Transaction> createAddressPropertyTransaction(
				const PropertyAddressBlockDescriptor& descriptor,
				Timestamp deadline);



	};
}}

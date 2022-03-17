/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "TransactionsBuilder.h"
#include "TransactionBuilderCapability.h"


namespace catapult { namespace test {


	class TransactionBuilderTransferCapability : public TransactionBuilderCapability
	{

	private:
		struct TransferDescriptor {
			size_t SenderId;
			size_t RecipientId;
			catapult::Amount Amount;
			std::string RecipientAlias; // optional
		};
	public:
		/// Adds a transfer from \a senderId to \a recipientId for amount \a transferAmount.
		void addTransfer(size_t senderId, size_t recipientId, Amount transferAmount);

		/// Adds a transfer from \a senderId to \a recipientAlias for amount \a transferAmount.
		void addTransfer(size_t senderId, const std::string& recipientAlias, Amount transferAmount);

	public:
		explicit TransactionBuilderTransferCapability(TransactionsBuilder& builder) : TransactionBuilderCapability(builder)
		{

		}
		void registerHooks() override;
	private:
		model::UniqueEntityPtr<model::Transaction> createTransfer(const TransferDescriptor& descriptor, Timestamp deadline);



	};

}}

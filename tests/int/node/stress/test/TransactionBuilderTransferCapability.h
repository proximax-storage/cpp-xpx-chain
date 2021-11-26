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

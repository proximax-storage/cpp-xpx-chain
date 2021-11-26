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

#include <sdk/src/extensions/ConversionExtensions.h>
#include <plugins/txes/namespace/src/model/NamespaceIdGenerator.h>
#include "TransactionBuilderCapability.h"
#include "TransactionBuilderTransferCapability.h"
#include "tests/test/local/RealTransactionFactory.h"

namespace catapult { namespace test {

		// region add


		namespace {
			constexpr auto Network_Identifier = model::NetworkIdentifier::Mijin_Test;
			UnresolvedAddress RootAliasToAddress(const std::string& namespaceName) {
				auto namespaceId = model::GenerateNamespaceId(NamespaceId(), namespaceName);

				UnresolvedAddress address{}; // force zero initialization
				address[0] = utils::to_underlying_type(Network_Identifier) | 0x01;
				std::memcpy(address.data() + 1, &namespaceId, sizeof(NamespaceId));
				return address;
			}
		}

		void TransactionBuilderTransferCapability::registerHooks() {
			m_builder.registerDescriptor(DescriptorTypes::Transfer, [self = ptr<TransactionBuilderTransferCapability>()](auto& pDescriptor, auto deadline){
			  return self->createTransfer(CastToDescriptor<TransferDescriptor>(pDescriptor), deadline);
			});
		}
		void TransactionBuilderTransferCapability::addTransfer(size_t senderId, const std::string& recipientAlias, Amount transferAmount) {
			auto descriptor = TransferDescriptor{ senderId, 0, transferAmount, recipientAlias };
			add(DescriptorTypes::Transfer, descriptor);
		}

		void TransactionBuilderTransferCapability::addTransfer(size_t senderId, size_t recipientId, Amount transferAmount) {
			auto descriptor = TransferDescriptor{ senderId, recipientId, transferAmount, "" };
			add(DescriptorTypes::Transfer, descriptor);
		}

		model::UniqueEntityPtr<model::Transaction> TransactionBuilderTransferCapability::createTransfer(
				const TransferDescriptor& descriptor,
				Timestamp deadline) {
			const auto& senderKeyPair = accounts().getKeyPair(descriptor.SenderId);
			auto recipientAddress = descriptor.RecipientAlias.empty()
									? extensions::CopyToUnresolvedAddress(accounts().getAddress(descriptor.RecipientId))
									: RootAliasToAddress(descriptor.RecipientAlias);

			auto pTransaction = CreateTransferTransaction(senderKeyPair, recipientAddress, descriptor.Amount);
			return SignWithDeadline(std::move(pTransaction), senderKeyPair, deadline);
		}


		// endregion
}}

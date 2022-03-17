/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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

/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "sdk/src/extensions/TransactionExtensions.h"
#include "TransactionsGenerator.h"
#include "TransactionsBuilder.h"
#include "tests/test/nodeps/Nemesis.h"

namespace catapult { namespace test {

		enum class DescriptorTypes : uint32_t{
			Transfer = 0,
			Property,
			Secret_Lock,
			Secret_Proof,
			Namespace,
			Alias,
			Network_Config
		};
		/// Dataless class that describes a capability
		class TransactionBuilderCapability : public std::enable_shared_from_this<TransactionBuilderCapability>
		{
		public:
			TransactionBuilderCapability(TransactionsBuilder& builder) :m_builder(builder)
			{

			}

			virtual void registerHooks() = 0;
		protected:
			template<typename TDescriptorType, typename TDescriptor>
			void add(TDescriptorType descriptorType, const TDescriptor& descriptor) {
				m_builder.add(descriptorType, descriptor);
			}

			/// Casts \a pVoid to a descriptor.
			template<typename TDescriptor>
			static const TDescriptor& CastToDescriptor(const std::shared_ptr<const void>& pVoid) {
				return *static_cast<const TDescriptor*>(pVoid.get());
			}

			const Accounts& accounts() const
			{
				return m_builder.accounts();
			}

			template<typename TChildType>
			std::shared_ptr<TChildType> ptr() {
				return std::static_pointer_cast<TChildType>(shared_from_this());
			}

			model::UniqueEntityPtr<model::Transaction> SignWithDeadline(
					model::UniqueEntityPtr<model::Transaction>&& pTransaction,
					const crypto::KeyPair& signerKeyPair,
					Timestamp deadline) {
				pTransaction->Deadline = deadline;
				pTransaction->MaxFee = Amount(pTransaction->Size);
				extensions::TransactionExtensions(GetNemesisGenerationHash()).sign(signerKeyPair, *pTransaction);
				return std::move(pTransaction);
			}

			TransactionsBuilder& m_builder;
		};
}}

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

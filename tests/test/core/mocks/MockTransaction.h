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
#include "catapult/model/Notifications.h"
#include "catapult/model/Transaction.h"
#include "catapult/model/TransactionPlugin.h"
#include "tests/test/core/BalanceTransfers.h"

namespace catapult { namespace mocks {

	// region mock notifications

/// Defines a mock notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_MOCK_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) \
	constexpr auto Mock_##DESCRIPTION##_Notification = model::MakeNotificationType( \
			(model::NotificationChannel::CHANNEL), \
			(static_cast<model::FacilityCode>(-1)), \
			CODE)

	/// A mock notification raised on the observer channel.
	DEFINE_MOCK_NOTIFICATION(Observer_1, 0xFFFF, Observer);

	/// A second mock notification raised on the observer channel.
	DEFINE_MOCK_NOTIFICATION(Observer_2, 0xFFFE, Observer);

	/// A mock notification raised on the validator channel.
	DEFINE_MOCK_NOTIFICATION(Validator_1, 0xFFFF, Validator);

	/// A second mock notification raised on the validator channel.
	DEFINE_MOCK_NOTIFICATION(Validator_2, 0xFFFE, Validator);

	/// A mock notification raised on all channels.
	DEFINE_MOCK_NOTIFICATION(All_1, 0xFFFF, All);

	/// A second mock notification raised on all channels.
	DEFINE_MOCK_NOTIFICATION(All_2, 0xFFFE, All);

	/// A hash notification raised on no channels.
	DEFINE_MOCK_NOTIFICATION(Hash, 0xFFFD, None);

#undef DEFINE_MOCK_NOTIFICATION

	/// Notifies the arrival of a hash.
	struct HashNotification : public model::Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Mock_Hash_Notification;

	public:
		/// Creates a hash notification around \a hash.
		explicit HashNotification(const Hash256& hash)
				: model::Notification(Notification_Type, sizeof(HashNotification))
				, Hash(hash)
		{}

	public:
		/// Hash.
		const Hash256& Hash;
	};

	// endregion

	// region mock transaction

#pragma pack(push, 1)

	/// Binary layout for a mock transaction body.
	template<typename THeader>
	struct MockTransactionBody : public THeader {
	private:
		using TransactionType = MockTransactionBody<THeader>;

	public:
		static constexpr auto Entity_Type = static_cast<model::EntityType>(0x4FFF);

		static constexpr uint8_t Current_Version = 0xFF;

	public:
		/// Binary layout for a variable data header.
		struct VariableDataHeader {
			/// Size of the data.
			uint16_t Size;
		};

	public:
		/// Transaction recipient.
		Key Recipient;

		/// Variable data header.
		VariableDataHeader Data;

		// followed by data if Data.Size != 0

	private:
		template<typename T>
		static auto DataPtrT(T& transaction) {
			return transaction.Data.Size ? THeader::PayloadStart(transaction) : nullptr;
		}

	public:
		/// Returns a const pointer to the variable data contained in this transaction.
		const uint8_t* DataPtr() const {
			return DataPtrT(*this);
		}

		/// Returns a pointer to the variable data contained in this transaction.
		uint8_t* DataPtr() {
			return DataPtrT(*this);
		}

	public:
		// Calculates the real size of mock \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.Data.Size;
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(Mock)

#pragma pack(pop)

	// endregion

	/// Creates a mock transaction with variable data composed of \a dataSize random bytes.
	std::unique_ptr<MockTransaction> CreateMockTransaction(uint16_t dataSize);

	/// Creates an embedded mock transaction with variable data composed of \a dataSize random bytes.
	std::unique_ptr<EmbeddedMockTransaction> CreateEmbeddedMockTransaction(uint16_t dataSize);

	/// Creates a mock transaction with a \a fee and \a transfers.
	std::unique_ptr<mocks::MockTransaction> CreateTransactionWithFeeAndTransfers(
			Amount fee,
			const std::vector<model::UnresolvedMosaic>& transfers);

	/// Creates a mock transaction with \a signer and \a recipient.
	std::unique_ptr<MockTransaction> CreateMockTransactionWithSignerAndRecipient(const Key& signer, const Key& recipient);

	/// Mock transaction plugin options.
	enum class PluginOptionFlags : uint8_t {
		/// Default plugin options.
		Default = 1,
		/// Configures the mock transaction plugin to not support embedding.
		Not_Embeddable = 2,
		/// Configures the mock transaction plugin to publish extra transaction data as balance transfers.
		Publish_Transfers = 4,
		/// Configures the mock transaction plugin to publish extra custom notifications.
		Publish_Custom_Notifications = 8,
		/// Configures the mock transaction plugin to return a custom data buffer (equal to the mock transaction's payload sans header).
		Custom_Buffers = 16
	};

	/// Returns \c true if \a options has \a flag set.
	bool IsPluginOptionFlagSet(PluginOptionFlags options, PluginOptionFlags flag);

	/// Creates a (mock) transaction plugin with the specified \a type.
	std::unique_ptr<model::TransactionPlugin> CreateMockTransactionPlugin(model::EntityType type);

	/// Creates a (mock) transaction plugin with \a options.
	std::unique_ptr<model::TransactionPlugin> CreateMockTransactionPlugin(PluginOptionFlags options = PluginOptionFlags::Default);

	/// Creates a (mock) transaction plugin with the specified \a type and \a options.
	std::unique_ptr<model::TransactionPlugin> CreateMockTransactionPlugin(model::EntityType type, PluginOptionFlags options);

	/// Creates a default transaction registry with a single registered (mock) transaction with \a options.
	model::TransactionRegistry CreateDefaultTransactionRegistry(PluginOptionFlags options = PluginOptionFlags::Default);
}}

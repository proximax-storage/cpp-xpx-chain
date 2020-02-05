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
#include "catapult/model/Cosignature.h"
#include "catapult/model/EmbeddedTransaction.h"
#include "catapult/model/Notifications.h"

namespace catapult { namespace model {

	// region aggregate notification types

/// Defines an aggregate notification type with \a DESCRIPTION, \a CODE and \a CHANNEL.
#define DEFINE_AGGREGATE_NOTIFICATION(DESCRIPTION, CODE, CHANNEL) DEFINE_NOTIFICATION_TYPE(CHANNEL, Aggregate, DESCRIPTION, CODE)

	/// Aggregate was received with cosignatures.
	DEFINE_AGGREGATE_NOTIFICATION(Cosignatures_v1, 0x001, Validator);

	/// Aggregate was received with an embedded transaction.
	DEFINE_AGGREGATE_NOTIFICATION(EmbeddedTransaction_v1, 0x002, Validator);

	/// Aggregate of specific type was received.
	DEFINE_AGGREGATE_NOTIFICATION(Type_v1, 0x003, Validator);

	/// Aggregate was received with hash and sub transactions.
	DEFINE_AGGREGATE_NOTIFICATION(Hash_v1, 0x004, Observer);

#undef DEFINE_AGGREGATE_NOTIFICATION

	// endregion

	/// A basic aggregate notification.
	template<typename TDerivedNotification>
	struct BasicAggregateNotification : public Notification {
	public:
		/// Creates a notification around \a signer, \a cosignaturesCount and \a pCosignatures.
		explicit BasicAggregateNotification(const Key& signer, size_t cosignaturesCount, const Cosignature* pCosignatures)
				: Notification(TDerivedNotification::Notification_Type, sizeof(TDerivedNotification))
				, Signer(signer)
				, CosignaturesCount(cosignaturesCount)
				, CosignaturesPtr(pCosignatures)
		{}

	public:
		/// Aggregate signer.
		const Key& Signer;

		/// Number of cosignatures.
		size_t CosignaturesCount;

		/// Const pointer to the first cosignature.
		const Cosignature* CosignaturesPtr;
	};

	/// Notification of an embedded aggregate transaction with cosignatures.
	template<VersionType version>
	struct AggregateEmbeddedTransactionNotification;

	template<>
	struct AggregateEmbeddedTransactionNotification<1> : public BasicAggregateNotification<AggregateEmbeddedTransactionNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Aggregate_EmbeddedTransaction_v1_Notification;

	public:
		/// Creates a notification around \a signer, \a transaction, \a cosignaturesCount and \a pCosignatures.
		explicit AggregateEmbeddedTransactionNotification(
				const Key& signer,
				const EmbeddedTransaction& transaction,
				size_t cosignaturesCount,
				const Cosignature* pCosignatures)
				: BasicAggregateNotification<AggregateEmbeddedTransactionNotification<1>>(signer, cosignaturesCount, pCosignatures)
				, Transaction(transaction)
		{}

	public:
		/// Embedded transaction.
		const EmbeddedTransaction& Transaction;
	};

	/// Notification of an aggregate transaction with transactions and cosignatures.
	/// \note TransactionsPtr and CosignaturesPtr are provided instead of minimally required keys in order to support undoing.
	template<VersionType version>
	struct AggregateCosignaturesNotification;

	template<>
	struct AggregateCosignaturesNotification<1> : public BasicAggregateNotification<AggregateCosignaturesNotification<1>> {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Aggregate_Cosignatures_v1_Notification;

	public:
		/// Creates a notification around \a signer, \a transactionsCount, \a pTransactions, \a cosignaturesCount and \a pCosignatures.
		explicit AggregateCosignaturesNotification(
				const Key& signer,
				size_t transactionsCount,
				const EmbeddedTransaction* pTransactions,
				size_t cosignaturesCount,
				const Cosignature* pCosignatures)
				: BasicAggregateNotification<AggregateCosignaturesNotification<1>>(signer, cosignaturesCount, pCosignatures)
				, TransactionsCount(transactionsCount)
				, TransactionsPtr(pTransactions)
		{}

	public:
		/// Number of transactions.
		size_t TransactionsCount;

		/// Const pointer to the first transaction.
		const EmbeddedTransaction* TransactionsPtr;
	};

	/// Notification of transaction entity type.
	template<VersionType version>
	struct AggregateTransactionTypeNotification;

	template<>
	struct AggregateTransactionTypeNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Aggregate_Type_v1_Notification;

	public:
		/// Creates a notification around \a type.
		explicit AggregateTransactionTypeNotification(const EntityType & type)
				: Notification(Notification_Type, sizeof(AggregateTransactionTypeNotification<1>))
				, Type(type)
		{}

	public:
		/// Transaction entity type.
		const EntityType& Type;
	};

	/// Notification of an aggregate transaction hash with sub transactions.
	template<VersionType version>
	struct AggregateTransactionHashNotification;

	template<>
	struct AggregateTransactionHashNotification<1> : public Notification {
	public:
		/// Matching notification type.
		static constexpr auto Notification_Type = Aggregate_Hash_v1_Notification;

	public:
		/// Creates a notification around \a aggregateHash, \a transactionsCount and \a pTransactions.
		explicit AggregateTransactionHashNotification(
			const Hash256& aggregateHash,
			size_t transactionsCount,
			const EmbeddedTransaction* pTransactions)
				: Notification(Notification_Type, sizeof(AggregateTransactionHashNotification<1>))
				, AggregateHash(aggregateHash)
				, TransactionsCount(transactionsCount)
				, TransactionsPtr(pTransactions)
		{}

	public:
		/// Hash of the aggregate transaction.
		Hash256 AggregateHash;

		/// Number of transactions.
		size_t TransactionsCount;

		/// Const pointer to the first transaction.
		const EmbeddedTransaction* TransactionsPtr;
	};
}}

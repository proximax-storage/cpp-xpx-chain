/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultConfigEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a catapult config transaction body.
	template<typename THeader>
	struct CatapultConfigTransactionBody : public THeader {
	private:
		using TransactionType = CatapultConfigTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Catapult_Config, 1)

	public:
		/// Number of blocks before applying config.
		BlockDuration ApplyHeightDelta;

		/// Blockchain configuration size in bytes.
		uint16_t BlockChainConfigSize;

		/// Supported entity versions configuration size in bytes.
		uint16_t SupportedEntityVersionsSize;

		// followed by blockchain configuration data if BlockChainConfigSize != 0
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(BlockChainConfig, uint8_t)

		// followed by blockchain configuration data if SupportedEntityVersionsSize != 0
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(SupportedEntityVersions, uint8_t)

	public:
		template<typename T>
		static auto* BlockChainConfigPtrT(T& transaction) {
			return transaction.BlockChainConfigSize ? THeader::PayloadStart(transaction) : nullptr;
		}

		template<typename T>
		static auto* SupportedEntityVersionsPtrT(T& transaction) {
			auto* pPayloadStart = THeader::PayloadStart(transaction);
			return transaction.SupportedEntityVersionsSize && pPayloadStart ? pPayloadStart + transaction.BlockChainConfigSize : nullptr;
		}

		// Calculates the real size of a catapult config \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(TransactionType) + transaction.BlockChainConfigSize + transaction.SupportedEntityVersionsSize;
		}
	};

	DEFINE_EMBEDDABLE_TRANSACTION(CatapultConfig)

#pragma pack(pop)
}}

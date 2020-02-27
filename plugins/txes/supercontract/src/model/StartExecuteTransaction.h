/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractTypes.h"
#include "SuperContractEntityType.h"
#include "catapult/model/Transaction.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ArraySet.h"
#include "plugins/txes/operation/src/model/OperationTypes.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout for a start execute transaction header.
    template<typename THeader>
    struct StartExecuteTransactionBody : public BasicOperationTransactionBody<THeader, StartExecuteTransactionBody<THeader>> {
    private:
		using TransactionType = StartExecuteTransactionBody<THeader>;
		using BaseTransactionType = BasicOperationTransactionBody<THeader, StartExecuteTransactionBody<THeader>>;

    public:
        DEFINE_TRANSACTION_CONSTANTS(Entity_Type_StartExecute, 1)

    public:
        /// Key of super contract account.
        Key SuperContract;

        /// Function size in bytes.
        uint8_t FunctionSize;

        /// Data size in bytes.
        uint16_t DataSize;

        // followed by function name if FunctionSize != 0
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Function, uint8_t)

        // followed by data if DataSize != 0
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Data, uint8_t)

    private:
        template<typename T>
        static auto* FunctionPtrT(T& transaction) {
            auto* pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.FunctionSize && pPayloadStart ? pPayloadStart + transaction.MosaicCount * sizeof(UnresolvedMosaic) : nullptr;
        }

        template<typename T>
        static auto* DataPtrT(T& transaction) {
            auto* pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.DataSize && pPayloadStart ? pPayloadStart
                + transaction.MosaicCount * sizeof(UnresolvedMosaic)
                + transaction.FunctionSize : nullptr;
        }

    public:
        // Calculates the real size of execute \a transaction.
        static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
            return BaseTransactionType::CalculateRealSize(transaction) + Key_Size + sizeof(uint8_t) + sizeof(uint16_t) +
                + transaction.FunctionSize + transaction.DataSize;
        }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(StartExecute)

#pragma pack(pop)

    /// Extracts public keys of additional accounts that must approve \a transaction.
    inline utils::KeySet ExtractAdditionalRequiredCosigners(const EmbeddedStartExecuteTransaction& transaction) {
        return { transaction.SuperContract };
    }
}}

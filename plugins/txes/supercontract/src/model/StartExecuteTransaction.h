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

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout for an start execute transaction header.
    template<typename THeader>
    struct StartExecuteTransactionBody : public THeader {
    private:
        using TransactionType = StartExecuteTransactionBody<THeader>;

    public:
        DEFINE_TRANSACTION_CONSTANTS(Entity_Type_StartExecute, 1)

    public:
        /// Key of super contract account.
        Key SuperContract;

        /// Function size in bytes.
        uint8_t FunctionSize;

        /// Number of mosaics.
        uint8_t MosaicsCount;

        /// Data size in bytes.
        uint16_t DataSize;

        // followed by function name if FunctionSize != 0
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Function, uint8_t)

        // followed by mosaics data if MosaicsCount != 0
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Mosaics, UnresolvedMosaic)

        // followed by data if DataSize != 0
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Data, uint8_t)

    private:
        template<typename T>
        static auto* FunctionPtrT(T& transaction) {
            return transaction.FunctionSize ? THeader::PayloadStart(transaction) : nullptr;
        }

        template<typename T>
        static auto* MosaicsPtrT(T& transaction) {
            auto* pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.MosaicsCount && pPayloadStart ? pPayloadStart + transaction.FunctionSize : nullptr;
        }

        template<typename T>
        static auto* DataPtrT(T& transaction) {
            auto* pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.DataSize && pPayloadStart ? pPayloadStart
                + transaction.FunctionSize
                + transaction.MosaicsCount * sizeof(UnresolvedMosaic) : nullptr;
        }

    public:
        // Calculates the real size of execute \a transaction.
        static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
            return sizeof(TransactionType)
                + transaction.FunctionSize + transaction.MosaicsCount * sizeof(UnresolvedMosaic) + transaction.DataSize;
        }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(StartExecute)

#pragma pack(pop)

    /// Extracts public keys of additional accounts that must approve \a transaction.
    inline utils::KeySet ExtractAdditionalRequiredCosigners(const EmbeddedStartExecuteTransaction& transaction) {
        return { transaction.SuperContract };
    }
}}

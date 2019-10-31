/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "HelloBuilder.h"

namespace catapult { namespace builders {

        HelloBuilder::HelloBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
                : TransactionBuilder(networkIdentifier, signer)
        {}

        void HelloBuilder::setMessageCount(uint16_t messageCount) {
            m_messageCount = messageCount;
        }

        template<typename TransactionType>
        model::UniqueEntityPtr<TransactionType> HelloBuilder::buildImpl() const {
            // 1. allocate
            auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType));

            // 2. set transaction fields
            pTransaction->MessageCount = m_messageCount;

            return pTransaction;
        }

        model::UniqueEntityPtr<HelloBuilder::Transaction> HelloBuilder::build() const {
            return buildImpl<Transaction>();
        }

        model::UniqueEntityPtr<HelloBuilder::EmbeddedTransaction> HelloBuilder::buildEmbedded() const {
            return buildImpl<EmbeddedTransaction>();
        }
    }}

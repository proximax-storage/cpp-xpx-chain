/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once

#include "TransactionBuilder.h"
#include "plugins/txes/hello/src/model/HelloTransaction.h"
#include <vector>

namespace catapult { namespace builders {

        /// Builder for a transfer transaction.
        class HelloBuilder : public TransactionBuilder {
        public:
            using Transaction = model::HelloTransaction;
            using EmbeddedTransaction = model::EmbeddedHelloTransaction;

        public:
            /// Creates a transfer builder for building a transfer transaction from \a signer
            /// for the network specified by \a networkIdentifier.
            HelloBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

        public:

            /// Sets the transaction message to \a message.
            void setMessageCount(uint16_t messageCount);

        public:
            /// Builds a new transfer transaction.
            model::UniqueEntityPtr<Transaction> build() const;

            /// Builds a new embedded transfer transaction.
            model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

        private:
            template<typename TTransaction>
            model::UniqueEntityPtr<TTransaction> buildImpl() const;

        private:
            uint16_t m_messageCount;

        };
    }}

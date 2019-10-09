/**
*** FOR TRAINING PURPOSES ONLY
**/

#pragma once
#include "HelloEntityType.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

	/// Binary layout for a transfer transaction body.
	template<typename THeader>
	struct HelloTransactionBody : public THeader {
	private:
		using TransactionType = HelloTransactionBody<THeader>;

	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Hello, 1)

	public:

		/// Number of message displayed
		uint16_t MessageCount;

		// Transaction Signer public key
		Key SignerKey;

	public:
		// Calculates the real size of transfer \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			return sizeof(transaction);     // might need additional size for Key?
		}
	};

	// defines the following types
	// struct EmbeddedHelloTransaction : public HelloTransactionBody<model::EmbeddedHello>
	// struct HelloTransaction : public HelloTransactionBody<model::Hello>
	DEFINE_EMBEDDABLE_TRANSACTION(Hello)

#pragma pack(pop)
}}

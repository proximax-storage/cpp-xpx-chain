/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "catapult/model/Notifications.h"
#include "catapult/model/SupercontractModel.h"
#include <utility>
#include <catapult/crypto/CurvePoint.h>

namespace catapult::model {

	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, Opinion_Signature_v1, 0x0004);
	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, Proof_Of_Execution_v1, 0x0006);
	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, Batch_Calls_Notification_v1, 0x0007);
	DEFINE_NOTIFICATION_TYPE(All, SuperContract_v2, End_Batch_Execution_Notification_v1, 0x0007);

	struct Opinion {
		Key PublicKey;
		Signature Sign;
		std::vector<uint8_t> Data;

		Opinion(const Key& publicKey, const Signature& sign, std::vector<uint8_t>&& data)
			: PublicKey(publicKey), Sign(sign), Data(std::move(data)) {}
	};

	template<VersionType version>
	struct OpinionSignatureNotification;

	template<>
	struct OpinionSignatureNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_Opinion_Signature_v1_Notification;

	public:
		explicit OpinionSignatureNotification(
				const std::vector<uint8_t>& commonData,
				const std::vector<Opinion>& opinions)
			: Notification(Notification_Type, sizeof(OpinionSignatureNotification<1>))
			, CommonData(commonData)
			, Opinions(opinions) {}

	public:
		std::vector<uint8_t> CommonData;
		std::vector<Opinion> Opinions;
	};

	struct ProofOfExecution {
		uint64_t StartBatchId;
		crypto::CurvePoint T;
		crypto::Scalar R;
		crypto::CurvePoint F;
		crypto::Scalar K;
	};

	template<VersionType version>
	struct ProofOfExecutionNotification;

	template<>
	struct ProofOfExecutionNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_Proof_Of_Execution_v1_Notification;

	public:
		explicit ProofOfExecutionNotification(const Key& contractKey, const std::map<Key, ProofOfExecution>& proofs)
		: Notification(Notification_Type, sizeof(ProofOfExecutionNotification<1>))
		, ContractKey(contractKey)
		, Proofs(proofs) {}

	public:
		Key ContractKey;
		std::map<Key, ProofOfExecution> Proofs;
	};

	template<VersionType version>
	struct BatchCallsNotification;

	template<>
	struct BatchCallsNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_Batch_Calls_Notification_v1_Notification;

	public:
		explicit BatchCallsNotification(const Key& contractKey,
										const std::vector<CallDigest>& digests,
										const std::vector<CallPaymentOpinion>& paymentOpinions)
				: Notification(Notification_Type, sizeof(BatchCallsNotification<1>))
				, ContractKey(contractKey)
				, PaymentOpinions(paymentOpinions)
				, Digests(digests) {}

	public:
		Key ContractKey;
		std::vector<CallDigest> Digests;
		std::vector<CallPaymentOpinion> PaymentOpinions;
	};

	template<VersionType version>
	struct EndBatchExecutionNotification;

	template<>
	struct EndBatchExecutionNotification<1> : public Notification {
	public:
		static constexpr auto Notification_Type = SuperContract_v2_End_Batch_Execution_Notification_v1_Notification;

	public:
		explicit EndBatchExecutionNotification(const Key& contractKey,
											   uint64_t batchId)
										: Notification(Notification_Type, sizeof(EndBatchExecutionNotification<1>))
										, ContractKey(contractKey) {}

	public:
		Key ContractKey;
		uint64_t BatchId;
	};
}
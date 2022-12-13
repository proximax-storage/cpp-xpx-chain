/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ProofOfExecutionNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ProofOfExecution, [](const Notification& notification, const ValidatorContext& context) {

		const auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
		auto contractIt = contractCache.find(notification.ContractKey);

		// For the moment we should have already been checked that the contract exists
		const auto& contractEntry = contractIt.get();

		for (const auto& [publicKey, proofToVerify]: notification.Proofs) {

			const auto& executorsInfo = contractEntry.executorsInfo();
			auto executorIt = executorsInfo.find(publicKey);

			if (executorIt == executorsInfo.end()) {

			}

			Hash512 dHash;
			crypto::Sha3_512_Builder dHasher;
			dHasher.update({ proofToVerify.F.toBytes(), proofToVerify.T.toBytes(), publicKey });
			dHasher.final(dHash);
			crypto::Scalar d(dHash.array());

			const auto base = crypto::CurvePoint::BasePoint();

			if (proofToVerify.F != proofToVerify.K * base + d * proofToVerify.T) {
				return Failure_SuperContract_Invalid_T_Proof;
			}

			crypto::CurvePoint previousT;
			crypto::Scalar previousR;
			uint64_t verifyStartBatchId = 0;

			const auto& executorInfo = executorIt->second;
			const state::ProofOfExecution& executorInfoPoEx = executorIt->second.PoEx;
			if (executorInfoPoEx.StartBatchId == proofToVerify.StartBatchId) {
				previousT = executorInfoPoEx.T;
				previousR = executorInfoPoEx.R;
				verifyStartBatchId = executorInfo.NextBatchToApprove;
			} else if (executorInfo.NextBatchToApprove <= proofToVerify.StartBatchId + 1) {
				// Actually the condition should be "<" here.
				// But due to possible executor's restart the situation with "<=" can occur
				verifyStartBatchId = proofToVerify.StartBatchId;
			} else {
				return Failure_SuperContract_Invalid_Start_Batch_Id;
			}

			crypto::CurvePoint left = proofToVerify.T - previousT;
			crypto::CurvePoint right = (proofToVerify.R - previousR) * crypto::CurvePoint::BasePoint();
			for (uint i = verifyStartBatchId; i <= contractEntry.batches().size(); i++) {
				const auto& verificationInfo = contractEntry.batches()[i].PoExVerificationInformation;
				crypto::Sha3_512_Builder hasher_h2;
				Hash512 cHash;
				hasher_h2.update({ base.toBytes(), verificationInfo.toBytes(), publicKey });
				hasher_h2.final(cHash);
				crypto::Scalar c(cHash.array());
				right += c * verificationInfo;
			}

			if (left != right) {
				return Failure_SuperContract_Invalid_Batch_Proof;
			}
		}

		return ValidationResult::Success;
	})

}}

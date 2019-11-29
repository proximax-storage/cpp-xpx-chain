/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/utils/ArraySet.h"

namespace catapult { namespace validators {

	using Notification = model::DeleteRewardNotification<1>;

	DEFINE_STATELESS_VALIDATOR(DeleteReward, [](const Notification& notification) {
	    if (notification.DeletedFiles.size() == 0)
	        return Failure_Service_Zero_Deleted_Files;

	    utils::HashSet fileHashes;
        for (const auto& pFile : notification.DeletedFiles) {
            fileHashes.insert(pFile->FileHash);

            auto pInfo = pFile->InfosPtr();
            utils::KeySet keys;
            for (auto i = 0u; i < pFile->InfosCount(); ++i, ++pInfo) {
                keys.insert(pInfo->Participant);

                if (pInfo->Uploaded == 0)
                    return Failure_Service_Zero_Upload_Info;
            }

            if (keys.size() != pFile->InfosCount())
                return Failure_Service_Participant_Redundant;
        }

        if (fileHashes.size() != notification.DeletedFiles.size())
            return Failure_Service_File_Hash_Redundant;

        return ValidationResult::Success;
    });
}}

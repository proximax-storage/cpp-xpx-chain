/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include <optional>

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout of an additional token.
    struct ServicePayment {
        MosaicId Token;
        Amount Amount;
    };

    using ServicePayments = std::vector<ServicePayment>;

    /// Binary layout of a start execute transaction parameters.
    struct StartExecuteParams {
        /// FileName to save stream in
        std::string FileName;

        /// FunctionName to save stream in
        std::string FunctionName;

        // ActualArguments to save stream in
        std::string ActualArguments;

        // Number of SC units provided to run the contract for all Executors on the Drive
        Amount ExecutionCallPayment;

        // Number of SM units provided to download data from the Internet for all Executors on the Drive
        Amount DownloadCallPayment;

        // Number of SM units provided to synchronize data for new Replicators
        Amount SynchronizeCallPayment;

        /// Number of necessary additional tokens to support specific Supercontract
        uint8_t ServicePaymentCount;

        /// Additional tokens to support specific Supercontract.
        std::optional<ServicePayments> ServicePayments;

        /// For a single approvement if it is allowed 
        bool SingleApprovement;
    };

#pragma pack(pop)
}}
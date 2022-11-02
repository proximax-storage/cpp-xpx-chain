/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Mosaic.h"
#include "src/model/StartExecuteTransactionParams.h"
#include <map>
#include <optional>

namespace catapult { namespace state {

    // Mixin for storing supercontract details.
	class SuperContractMixin {
        public:
            SuperContractMixin()
                : m_executorCount(0)
            {}

            /// Sets owner of super contract.
            void setOwner(const Key& owner) {
                m_owner = owner;
            }

            /// Gets owner of super contract.
            const Key& owner() const {
                return m_owner;
            }

            /// Sets name of the called .wasm file for automated executions.
            void setAutomatedExecutionFileName(const std::string& automatedExecutionFileName) {
                m_automatedExecutionFileName = automatedExecutionFileName;
            }

            /// Gets name of the called .wasm file for automated executions.
            const std::string& automatedExecutionFileName() const {
                return m_automatedExecutionFileName;
            }

            /// Sets name of the called function for automated executions.
            void setAutomatedExecutionFunctionName(const std::string& automatedExecutionFunctionName) {
                m_automatedExecutionFunctionName = automatedExecutionFunctionName;
            }   

            /// Gets name of the called function for automated executions.
            const std::string& automatedExecutionFunctionName() const {
                return m_automatedExecutionFunctionName;
            }

            /// Sets limit of the SC units for one automated Supercontract Execution.
            void setAutomatedExecutionCallPayment(const Amount& automatedExecutionCallPayment) {
                m_automatedExecutionCallPayment = automatedExecutionCallPayment;
            }

            /// Gets limit of the SC units for one automated Supercontract Execution.
            const Amount& automatedExecutionCallPayment() const {
                return m_automatedExecutionCallPayment;
            }

            /// Sets limit of the SM units for one automated Supercontract Execution.
            void setAutomatedDownloadCallPayment(const Amount& automatedDownloadCallPayment) {
                m_automatedDownloadCallPayment = automatedDownloadCallPayment;
            }

            /// Gets limit of the SM units for one automated Supercontract Execution.
            const Amount& automatedDownloadCallPayment() const {
                return m_automatedDownloadCallPayment;
            }
            
            // Sets limit of the SM units for one SM Synchronization.
            void setAutomatedSynchronizeCallPayment(const Amount& automatedSynchronizeCallPayment) {
                m_automatedSynchronizeCallPayment = automatedSynchronizeCallPayment;
            }

            // Gets limit of the SM units for one SM Synchronization.
            const Amount& automatedSynchronizeCallPayment() const {
                return m_automatedSynchronizeCallPayment;
            }
            
            /// Sets the number of prepaid automated executions.
            void setAutomatedExecutionsNumber(const uint32_t& automatedExecutionsNumber) {
                m_automatedExecutionsNumber = automatedExecutionsNumber;
            }

            /// Gets the number of prepaid automated executions.
            const uint32_t& automatedExecutionsNumber() const {
                return m_automatedExecutionsNumber;
            }

            /// Sets the Public Key to which the money is transferred in case of the Supercontract Closure.
            void setAssignee(const Key& assignee) {
                m_assignee = assignee;
            }

            /// Gets the Public Key to which the money is transferred in case of the Supercontract Closure.
            const Key& assignee() const {
                return m_assignee;
            }

            /// Sets the number of the supercontract \a executors.
    		void setExecutorCount(uint16_t executorCount) {
                m_executorCount = executorCount;
            }

            /// Gets the number of the supercontract executors.
            const uint16_t& executorCount() const {
                return m_executorCount;
            }

            /// Gets map of start execution parameters that belongs to respective supercontract key.
            const model::StartExecuteParams& startExecuteParams() const {
                return m_startExecuteParams;
            }

            /// Gets map of start execution paramters that belongs to respective supercontract key.
            model::StartExecuteParams& startExecuteParams() {
                return m_startExecuteParams;
            }

            /// Gets (date? time?) of when automatic executions is enabled
            const std::optional<uint64_t>& automaticExecutionsEnabledSince() const {
                return m_automaticExecutionsEnabledSince;
            } 

            /// Gets (date? time?) of when automatic executions is enabled
            std::optional<uint64_t>& automaticExecutionsEnabledSince() {
                return m_automaticExecutionsEnabledSince;
            }

        private:
            Key m_owner;
    		std::string m_automatedExecutionFileName;
            std::string m_automatedExecutionFunctionName;
            Amount m_automatedExecutionCallPayment;
            Amount m_automatedDownloadCallPayment;
            Amount m_automatedSynchronizeCallPayment;
            uint32_t m_automatedExecutionsNumber;
            Key m_assignee;
            uint16_t m_executorCount;
            model::StartExecuteParams m_startExecuteParams;
            std::optional<uint64_t> m_automaticExecutionsEnabledSince;
    };

    // Supercontract entry.
	class SuperContractEntry : public SuperContractMixin {
	    public:
            // Creates a super contract entry around \a key.
		    explicit SuperContractEntry(const Key& key) : m_key(key)
		    {}
        
        public:
            // Gets the super contract public key.
            const Key& key() const {
                return m_key;
            }

        private:
            Key m_key;
	};
}}
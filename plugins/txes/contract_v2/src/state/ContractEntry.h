/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

namespace catapult { namespace state {

    // Mixin for storing supercontract details.
	class ContractMixin {
        public:
            /// Gets owner of super contract.
            const Key& owner() const {
                return m_owner;
            }

            /// Sets owner of super contract.
            void setOwner(const Key& owner) {
                m_owner = owner;
            }

            /// Gets name of the called .wasm file for automated executions.
            const std::string& automatedExecutionFileName() const {
                return m_automatedExecutionFileName;
            }

            /// Sets name of the called .wasm file for automated executions.
            void setAutomatedExecutionFileName(const std::string& automatedExecutionFileName) {
                m_automatedExecutionFileName = automatedExecutionFileName;
            }

            /// Gets name of the called function for automated executions.
            const std::string& automatedExecutionFunctionName() const {
                return m_automatedExecutionFunctionName;
            }

            /// Sets name of the called function for automated executions.
            void setAutomatedExecutionFunctionName(const std::string& automatedExecutionFunctionName) {
                m_automatedExecutionFunctionName = automatedExecutionFunctionName;
            }

            /// Gets limit of the SC units for one automated Supercontract Execution.
            const Amount& automatedExecutionCallPayment() const {
                return m_automatedExecutionCallPayment;
            }

            /// Sets limit of the SC units for one automated Supercontract Execution.
            void setAutomatedExecutionCallPayment(const Amount& automatedExecutionCallPayment) {
                m_automatedExecutionCallPayment = automatedExecutionCallPayment;
            }

            /// Gets limit of the SM units for one automated Supercontract Execution.
            const Amount& automatedDownloadCallPayment() const {
                return m_automatedDownloadCallPayment;
            }

            /// Sets limit of the SM units for one automated Supercontract Execution.
            void setAutomatedDownloadCallPayment(const Amount& automatedDownloadCallPayment) {
                m_automatedDownloadCallPayment = automatedDownloadCallPayment;
            }

            /// Gets the number of prepaid automated executions.
            const uint32_t& automatedExecutionsNumber() const {
                return m_automatedExecutionsNumber;
            }

            /// Sets the number of prepaid automated executions.
            void setAutomatedExecutionsNumber(const uint32_t& automatedExecutionsNumber) {
                m_automatedExecutionsNumber = automatedExecutionsNumber;
            }

            /// Gets the Public Key to which the money is transferred in case of the Supercontract Closure.
            const Key& assignee() const {
                return m_assignee;
            }

            /// Sets the Public Key to which the money is transferred in case of the Supercontract Closure.
            void setAssignee(const Key& assignee) {
                m_assignee = assignee;
            }

        private:
            Key m_owner;
    		std::string m_automatedExecutionFileName;
            std::string m_automatedExecutionFunctionName;
            Amount m_automatedExecutionCallPayment;
            Amount m_automatedDownloadCallPayment;
            uint32_t m_automatedExecutionsNumber;
            Key m_assignee;
    };

    // Supercontract entry.
	class ContractEntry : public ContractMixin {
	    public:
            // Creates a super contract entry around \a key.
		    explicit ContractEntry(const Key& key) : m_key(key)
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
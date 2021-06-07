/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DataModificationApprovalBuilder.h"

namespace catapult { namespace builders {

	DataModificationApprovalBuilder::DataModificationApprovalBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
		, m_fileStructureSize(0)
		, m_usedDriveSize(0)
	{}

	void DataModificationApprovalBuilder::setDriveKey(const Key& driveKey) {
		m_driveKey = driveKey;
	}

	void DataModificationApprovalBuilder::setDataModificationId(const Hash256& dataModificationId) {
		m_dataModificationId = dataModificationId;
	}

	void DataModificationApprovalBuilder::setFileStructureCdi(const Hash256& fileStructureCdi) {
		m_fileStructureCdi = fileStructureCdi;
	}

	void DataModificationApprovalBuilder::setFileStructureSize(uint64_t fileStructureSize) {
		m_fileStructureSize = fileStructureSize;
	}

	void DataModificationApprovalBuilder::setUsedDriveSize(uint64_t usedDriveSize) {
		m_usedDriveSize = usedDriveSize;
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> DataModificationApprovalBuilder::buildImpl() const {
		// 1. allocate
		auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType));

		// 2. set transaction fields
		pTransaction->DriveKey = m_driveKey;
		pTransaction->DataModificationId = m_dataModificationId;
		pTransaction->FileStructureCdi = m_fileStructureCdi;
		pTransaction->FileStructureSize = m_fileStructureSize;
		pTransaction->UsedDriveSize = m_usedDriveSize;

		return pTransaction;
	}

	model::UniqueEntityPtr<DataModificationApprovalBuilder::Transaction> DataModificationApprovalBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<DataModificationApprovalBuilder::EmbeddedTransaction> DataModificationApprovalBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}

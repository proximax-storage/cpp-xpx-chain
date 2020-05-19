/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "TransactionBuilder.h"
#include "plugins/txes/mosaic/src/model/MosaicModifyLevyTransaction.h"
#include <vector>

namespace catapult { namespace builders {
		
		/// Builder for a mosaic modify levy transaction.
		class MosaicModifyLevyBuilder : public TransactionBuilder {
		public:
			using Transaction = model::MosaicModifyLevyTransaction;
			using EmbeddedTransaction = model::EmbeddedMosaicModifyLevyTransaction;
		
		public:
			/// Creates a mosaic modify levy builder for building a mosaic modify levy transaction from \a signer
			/// for the network specified by \a networkIdentifier.
			MosaicModifyLevyBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);
		
		public:
			/// Sets the mosaic nonce to \a mosaicId.
			void setMosaicId(const UnresolvedMosaicId& mosaicId);
		
			/// Sets the levy information to \a levy.
			void setMosaicLevy(const model::MosaicLevyRaw& levy);
			
		public:
			/// Builds a new mosaic definition transaction.
			model::UniqueEntityPtr<Transaction> build() const;
			
			/// Builds a new embedded mosaic definition transaction.
			model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;
		
		private:
			template<typename TTransaction>
			model::UniqueEntityPtr<TTransaction> buildImpl() const;
		
		private:
			UnresolvedMosaicId m_mosaicId;
			model::MosaicLevyRaw m_levy;
		};
	}}

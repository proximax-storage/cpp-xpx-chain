#pragma once
#include "catapult/model/Transaction.h"
#include "MosaicEntityType.h"
#include "src/model/MosaicLevy.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

		/// region bit mask definition for modify Transaction
		constexpr uint32_t MosaicLevyModifyBitNone	            = 0x0000;
		constexpr uint32_t MosaicLevyModifyBitChangeType     	= 0x0001;
		constexpr uint32_t MosaicLevyModifyBitChangeRecipient	= 0x0002;
		constexpr uint32_t MosaicLevyModifyBitChangeMosaicId	= 0x0004;
		constexpr uint32_t MosaicLevyModifyBitChangeLevyFee	    = 0x0008;
		/// endregion

		/// Binary layout for a mosaic modify levy transaction body.
		template<typename THeader>
		struct MosaicModifyLevyTransactionBody : public THeader {
		private:
			using TransactionType = MosaicModifyLevyTransactionBody<THeader>;

		public:
			DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Mosaic_Modify_Levy, 1)

		public:

			/// update type flag
			uint32_t ModifyFlag;

			/// Id of the mosaic.
			catapult::MosaicId MosaicId;

			/// levy container
			model::MosaicLevy LevyInfo;

		public:
			/// Calculates the real size of mosaic definition \a transaction.
			static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
				return sizeof(TransactionType);
			}
		};

		DEFINE_EMBEDDABLE_TRANSACTION(MosaicModifyLevy)

#pragma pack(pop)
}}
#include "src/model/MosaicModifyLevyTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/TestHarness.h"
#include "src/model/MosaicLevy.h"

namespace catapult { namespace model {

#define TEST_CLASS MosaicModifyLevyTransactionTests

		// region size + properties

		namespace {
			template<typename T>
			void AssertEntityHasExpectedSize(size_t baseSize) {
				// Arrange:
				auto expectedSize =
						baseSize // base
						+ sizeof(uint32_t )
						+ sizeof(MosaicId) // mosaic id
						+ sizeof(model::MosaicLevy); // levy

				// Assert:
				EXPECT_EQ(expectedSize, sizeof(T));
				EXPECT_EQ(baseSize + 55u, sizeof(T));
			}

			template<typename T>
			void AssertTransactionHasExpectedProperties() {
				// Assert:
				EXPECT_EQ(Entity_Type_Mosaic_Modify_Levy, T::Entity_Type);
				EXPECT_EQ(1u, T::Current_Version);
			}
		}

		ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(MosaicModifyLevy)

		// endregion

		TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
			// Arrange:
			MosaicModifyLevyTransaction transaction;
			transaction.Size = 0;

			// Act:
			auto realSize = MosaicModifyLevyTransaction::CalculateRealSize(transaction);

			// Assert:
			EXPECT_EQ(sizeof(MosaicModifyLevyTransaction), realSize);
		}
	}}

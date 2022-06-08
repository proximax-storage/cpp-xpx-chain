/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/types.h"
#include "catapult/model/Cosignature.h"
#include "tests/TestHarness.h"
namespace catapult { namespace test {

	namespace detail {
		template<typename TCosignatureType>
		const Key& GetSigner(const TCosignatureType& val)
		{
			return val.Signer;
		}

		template<>
		const Key& GetSigner(const model::WeakCosignaturePtr& val)
		{
			return val.Signer();
		}
	}
	template<typename TCosignatureType, typename TCompareType>
	void CompareCosignatures(TCosignatureType* originalType, TCompareType* targetType, size_t originalTypeSize)
	{
		for(auto i = 0; i < originalTypeSize; i++)
		{
			EXPECT_EQ(originalType->GetRawSignature(), targetType->GetRawSignature());
			EXPECT_EQ(detail::GetSigner(*originalType), detail::GetSigner(*targetType));
			EXPECT_EQ(originalType->GetDerivationScheme(), targetType->GetDerivationScheme());
			++originalType;
			++targetType;
		}
	}
	template<typename TCosignatureType>
	void CompareCosignatures(TCosignatureType* originalType, model::WeakCosignaturePtr val, size_t originalTypeSize)
	{
		for(auto i = 0; i < originalTypeSize; i++)
		{
			EXPECT_EQ(originalType->GetRawSignature(), val.GetRawSignature());
			EXPECT_EQ(detail::GetSigner(*originalType), detail::GetSigner(val));
			EXPECT_EQ(originalType->GetDerivationScheme(), val.GetDerivationScheme());
			++originalType;
			++val;
		}
	}

}}

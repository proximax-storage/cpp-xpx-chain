/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "KeyGenerator.h"
#include "CryptoUtils.h"
#include "PrivateKey.h"

extern "C" {
#include <ref10/ge.h>
#include <blst/blst.hpp>
}

namespace catapult { namespace crypto {

	void ExtractPublicKeyFromPrivateKey(const PrivateKey& privateKey, Key& publicKey) {
		Hash512 h;
		ge_p3 A;

		HashPrivateKey(privateKey, h);

		h[0] &= 0xF8;
		h[31] &= 0x7F;
		h[31] |= 0x40;

		ge_scalarmult_base(&A, h.data());
		ge_p3_tobytes(publicKey.data(), &A);
	}

	void ExtractPublicKeyFromPrivateKey(const BLSPrivateKey& privateKey, BLSPublicKey& publicKey) {
		blst::blst_p1_affine point;
		const auto* temp = reinterpret_cast<const blst::blst_scalar*>(&privateKey.m_array);
		blst::blst_sk_to_pk2_in_g1(nullptr, &point, temp);
		blst::blst_p1_affine_compress(publicKey.m_array, &point);
	}
}}

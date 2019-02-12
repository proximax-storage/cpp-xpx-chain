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

#pragma once
#include "catapult/types.h"
#include <cstring>

namespace catapult { namespace test {

	// private keys
	constexpr const char* Mijin_Test_Nemesis_Private_Key = "C06B2CC5D7B66900B2493CF68BE10B7AA8690D973B7F0B65D0DAE4F7AA464716";
	constexpr const char* Mijin_Test_Private_Keys[] = {
		"4A236D9F894CF0C4FC8C042DB5DB41CCF35118B7B220163E5B4BC1872C1CD618",
		"8473645728B15F007385CE2889D198D26369D2806DCDED4A9B219FD0DE23A505",
		"3519FA69D37F9A1A554A299D4F4B59FCD6552EA5AFC50523B6E220DF84B7E225",
		"97F821CE897F31736E40EBF53F35DE40D951A00E515412E1EE81D6862DAD64CC",
		"BBC3E5BE46A953070B0B9636E386C2006DA9EA8840596B601D4A1B92A9F93330",
		"264752A818349CFF23AB5F584CC702E9E356FD3C161B775ABC6E864BE856D368",
		"62AEBEDD1B93930FC4245408F146D5468A02779696E19F945DCB2378EBCA3936",
		"31DE76D1CF3486E8637F07D60B44EC26E99771C3B66AFFFBD53E309FB23F775C",
		"46C83EE87DAB6588DD82D1059140D3E5A7FAFF78C3A0C4CE4802486D71C69E40",
		"87E082692BC23CBAADEC51C6182F9EA5C694380F2EBA44A4E065E599B2C39C50",
		"FA19F42DDD6E757B3A2E39E75A7487F8FEC19C0E872153EC0EFD9AC2E5A84E58",
		"ED0C77E71FC59970F277D8CA8725F99263A2C0A1979AF91FC5ACB184FE0A3BAF",
		"F07E7A727B061D961935CAC91554FFFC517386A8B7D4CAA5BC35697615EBBD41",
		"F0C108F95FBE7A4D1DC178D484F52A64B0DEAA7069C75114DA969037D2CFF610",
		"239CCE2C5D2B83A70DC91AFEF0CE325FC9947FAA87C8B18473092CE6A745945A",
		"D8A7F4C3D2BC7291D4C5FE3B1E75F915E013FD9D51555FF66FFF429C3BBE089A",
		"A2E4B738F2DDF734DDFB146F488F33CF97D93185DB37AE2BC1A310A89487B288",
		"8FEE7840108CC6E59C638AF27F314486EA88D612392B03EABFE834B7719E4D82",
		"B21672BBADD2E5A9BCE9FB1C5B464EE9C7D14D5857DBB27D74867CD2D1CC26F1",
		"E08FCF65D436F4017A306B45378AAED5DD932466582CF3594E710A74EE13E488",
		"939491747DA684FDA8CC45C4C9923F396EA745F22FD59A0F5DD37DA350DB8EBA",
		"0E12D626A100C2E46964B6A6C095851D0E471632E126E5F11B3502714E6275E8"
	};

	// public keys
	constexpr const char* Namespace_Rental_Fee_Sink_Public_Key = "3E82E1C1E4A75ADAA3CBA8C101C3CD31D9817A2EB966EB3B511FB2ED45B8E262";
	constexpr const char* Mosaic_Rental_Fee_Sink_Public_Key = "53E140B5947F104CABC2D6FE8BAEDBC30EF9A0609C717D9613DE593EC2A266D3";

	/// Gets the nemesis importance for \a publicKey.
	CATAPULT_INLINE
	Importance GetNemesisImportance(const Key& publicKey) {
		auto index = 0u;
		for (const auto* pRecipientPrivateKeyString : test::Mijin_Test_Private_Keys) {
			auto keyPair = crypto::KeyPair::FromString(pRecipientPrivateKeyString);
			if (keyPair.publicKey() == publicKey)
				break;

			++index;
		}

		// to simulate real harvesting mosaics, test nemesis block uses discrete importance seedings
		// only first 11 accounts have any harvesting power, two special accounts have importance 4X, nine other 1X
		if (index > 10)
			return Importance(0);

		return 4 == index || 10 == index ? Importance(4'000) : Importance(1'000);
	}
}}

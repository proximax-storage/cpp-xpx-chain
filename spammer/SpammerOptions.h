//
// Created by green on 20.08.18.
//

#ifndef CATAPULT_SERVER_SPAMMEROPTIONS_H
#define CATAPULT_SERVER_SPAMMEROPTIONS_H

#include <string>
#include <vector>

using namespace std;

class SpammerOptions {
public:
	string Host;
	unsigned short Port;
	int Rate;
	string Mode;
	int Total;
	vector<string> privateKeys;
	string ApiNodePublicKey;
	string RestPrivateKey;
	uint64_t Token;
	uint64_t Amount;
};

#endif //CATAPULT_SERVER_SPAMMEROPTIONS_H

[nemesis]
networkIdentifier = {{network_identifier}}
nemesisGenerationHash = {{nemesis_generation_hash}}
nemesisSignerPrivateKey = {{nemesis_signer_private_key}}

[cpp]

cppFileHeader = ../HEADER.inc

[output]
cppFile =
binDirectory = ../seed/mijin-test

[namespaces]
prx = true
eur = false

[namespace>prx]
duration = 0
[namespace>eur]
duration = 123456789

[mosaics]
prx:xpx = true
eur:euro = false

[mosaic>prx:xpx]
divisibility = 6
duration = 0
supply = {{xpx.supply}}

isTransferable = true
isSupplyMutable = false
isLevyMutable = false

[distribution>prx:xpx]
{{#xpx.distribution}}
{{address}} = {{amount}}
{{/xpx.distribution}}

[mosaic>eur:euro]
divisibility = 2
duration = 1234567890
supply = 15'000'000
isTransferable = true
isSupplyMutable = true
isLevyMutable = false

# properties for nemesis block compatible with catapult signature scheme used in tests
# for proper generation, update following config-immutable properties:
# - shouldEnableVerifiableState = false
# - shouldEnableVerifiableReceipts = false

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
prx.xpx = true
prx.so = true
prx.sm = true
prx.sc = true
prx.rw = true

[namespace>prx]

duration = 0

[mosaics]
prx:xpx = true
prx:so = true
prx:sm = true
prx:sc = true
prx:rw = true

[mosaic>prx:xpx]
divisibility = 6
duration = 0
supply = {{xpx.supply}}

isTransferable = true
isSupplyMutable = false

[distribution>prx:xpx]
{{#xpx.distribution}}
{{address}} = {{amount}}
{{/xpx.distribution}}

[mosaic>prx:so]
divisibility = 0
duration = 0
supply = {{xpx.supply}}

isTransferable = true
isSupplyMutable = true

[distribution>prx:so]
{{#xpx.distribution}}
{{address}} = {{amount}}
{{/xpx.distribution}}

[mosaic>prx:sm]
divisibility = 0
duration = 0
supply = {{xpx.supply}}

isTransferable = true
isSupplyMutable = true

[distribution>prx:sm]
{{#xpx.distribution}}
{{address}} = {{amount}}
{{/xpx.distribution}}

[mosaic>prx:sc]
divisibility = 0
duration = 0
supply = {{xpx.supply}}

isTransferable = true
isSupplyMutable = true

[distribution>prx:sc]
{{#xpx.distribution}}
{{address}} = {{amount}}
{{/xpx.distribution}}

[mosaic>prx:rw]
divisibility = 0
duration = 0
supply = {{xpx.supply}}

isTransferable = true
isSupplyMutable = true

[distribution>prx:rw]
{{#xpx.distribution}}
{{address}} = {{amount}}
{{/xpx.distribution}}

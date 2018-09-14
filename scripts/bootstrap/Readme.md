runCatapultServers.sh will run 1 api node and N peer nodes.
Before run you need to install ash, mongodb, ruby, mustache for ruby

```
sudo apt-get install -y ash ruby
sudo gem install mustache
```

How to easy install mongodb you can find:

https://github.com/proximax-storage/proximax-catapult-rest/tree/SpammerUpgrade

And also update path to bootstrap folder and path to catapult server folder with pre-built catapult server binaries.

```
PATH_TO_CATAPULT_SERVER="~/proximax-catapult-server"
PATH_TO_BOOTSTRAP="~/proximax-catapult-server/scripts/bootstrap"
```

If you want to change a count of generated addresses, you need to change file
./ruby/lib/catapult/addresses.rb

If you want to change a count of peer nodes, you need to change file
./ruby/lib/catapult/config.rb in section CARDINALITY
and add this peers in runCatapultServers.sh

Example with 5 peers in runCatapultServers.sh:

```
...
generate_nem "api-node-0"
generate_nem "peer-node-0"
generate_nem "peer-node-1"
generate_nem "peer-node-2"
generate_nem "peer-node-3"
generate_nem "peer-node-4"
```

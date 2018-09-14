# Vagrant VM with dependencies

Creates a new Ubuntu 17.10 Virtual Machine and installs the needed dependencies to build Catapult Server.
Please wait to finish as this will take hours to build. 

```bash
# Start the vagrant environment
cd vagrant
vagrant up

# SSH into the VM
vagrant ssh

# Synced folder to the host's proximax-catapult-server directory 
cd /catapult-server
```
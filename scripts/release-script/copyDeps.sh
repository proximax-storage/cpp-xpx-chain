#!/bin/bash

function usage()
{
    cat << EOU
This script check required libs for a binary file(or directory with binary files) by ldd command and copy them to directory
Usage: bash $0 <path to directory with binaries> <path to copy the dependencies>
EOU
exit 1
}

#Validate the inputs
[[ $# < 2 ]] && usage

#Check if the paths are vaild
[[ ! -d $1 ]] && [[ ! -e $1 ]] && echo "Is not directory or binary file $1" && exit 1
[[ -d $2 ]] || echo "No such directory $2 creating..."&& mkdir -p "$2"

echo "Collecting the shared library dependencies for $1"

copyDependenciesOfBinary () {
  echo "Process $1"
  #Get the library dependencies
  deps=$(ldd $1 | awk 'BEGIN{ORS=" "}$1\
  ~/^\//{print $1}$3~/^\//{print $3}'\
   | sed 's/,$/\n/')
  echo "Copying the dependencies to $2"

  #Copy the deps
  for dep in $deps
  do
      echo "Copying $dep to $2$dep"
      mkdir -p $(dirname "$2$dep")
      cp  "$dep" "$2$dep"
  done
}

if [[ -d $1 ]]; then
  for filename in "$1"/*; do
    if [[ -e $filename ]]; then
      copyDependenciesOfBinary $filename $2
    fi
  done
else
  copyDependenciesOfBinary $1 $2
fi

echo "Done!"

#! /bin/bash

{
echo "#define PLUMED_VERSION_SHORT \"$(
  if test -f ../../VERSION ; then
    grep -v "#" ../../VERSION | sed  's/^\([0-9][0-9]*\.[0-9][0-9]*\).*/\1/'
  else
    echo "Unknown"
  fi
)\""

echo "#define PLUMED_VERSION_LONG \"$(
  if test -f ../../VERSION ; then
    grep -v "#" ../../VERSION
  else
    echo "Unknown"
  fi
)\""

echo "#define PLUMED_VERSION_GIT \"$(
  if test -d ../../.git && hash git 2> /dev/null ; then
    git describe --tags
  else
    echo "Unknown"
  fi
)\""

} > $1~

cmp -s $1~ $1 || cp $1~ $1
rm $1~




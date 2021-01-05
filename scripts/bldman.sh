#!/bin/bash

function WriteManifestLine
{
  if [ ! -d "$1" ]
  then
    mm5=`md5sum $1 2>/dev/null`
    if [[ ! "$mm5" == "" ]];
    then
      mm6=${mm5/ /,}
      mm6=${mm6// /}
      mm6=${mm6/.\//}
      echo "d,$mm6"
    fi
  fi

}

function ProcessDir
{
  for f2 in $1/*
  do
    if [ ! -d "$f2" ]
    then
      WriteManifestLine "$f2"
    else
      ProcessDir "$f2"
    fi

  done
}

ProcessDir "."


#!/bin/bash

# Attach to a MiniNExT host and run a command
# (Extends existing script to provide support for PID namespaces)

if [ -z $1 ]; then
  echo "usage: $0 Router or host name"
  exit 1
else
  host=$1
fi

echo $1 > /root/.prompt

pid=`ps ax | grep "mininet:$host$" | grep bash | grep -v mxexec | awk '{print $1};'`

if echo $pid | grep -q ' '; then
  echo "Error: found multiple mininet:$host processes"
  exit 2
fi

if [ "$pid" == "" ]; then
  echo "Could not find Mininet host $host"
  exit 3
fi

if [ -z $2 ]; then
  cmd='./root/.sh_ip_bgp.sh'
else
  shift
  cmd=$*
fi

cgroup=/sys/fs/cgroup/cpu/$host
if [ -d "$cgroup" ]; then
  cg="-g $host"
fi

# Check whether host should be running in a chroot dir
rootdir="/var/run/mn/$host/root"
if [ -d $rootdir -a -x $rootdir/bin/bash ]; then
    cmd="'cd `pwd`; exec $cmd'"
    cmd="chroot $rootdir /bin/bash -c $cmd"
fi

cmd="exec sudo mxexec -a $pid -b $pid -k $pid $cg $cmd"
eval $cmd

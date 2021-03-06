#!/bin/bash
# SZARP: SCADA software 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
# $Id$

Usage() {
	cat <<EOF
Usage: $0 [OPTION]... REPOSITORY_ADDRESS USER_NAME [ CONFIG_PREFIX ]

Script for replacing SZARP configuration with version fetched from SVN server.
Configuration must be first imported to repository using conf-import.py script.
REPOSITORY_ADDRESS must be full address of SVN repository. CONFIG_PREFIX, if
not given, is guessed from server hostname.
USER_NAME is used to access repository.

Options:
	-h, --help		print usage info and exit
	-p, --port=PORT		set ssh port number to PORT, default is 22

EOF
}

PORT=22
BASENAME=`basename "$0"`
TEMP=`getopt -o hp: --long help,port: -n "$BASENAME" -- "$@"`
if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit 1 ; fi
eval set -- "$TEMP"

while true ; do
	case "$1" in
		-h|--help)
			Usage $BASENAME
			exit 0
			shift
			;;
		-p|--port)
			PORT=$2
			shift 2
			;;
		--) shift; break;;
		*) echo "Internal error!" ; exit 1 ;;
	esac
done

REPO=
PREFIX=
if [ $# -eq 3 ]; then
	REPO=$1
	USERNAME=$2;
	PREFIX=$3;
	shift;
elif [ $# -eq 2 ]; then
	REPO=$1
	USERNAME=$2;
	echo -n "Guessing prefix... ";
	PREFIX=$(hostname -s);
	echo $PREFIX;
else
	Usage $0
	exit 1
fi

if [ ! -e /opt/szarp/$PREFIX/config/params.xml ]; then
	echo "File /opt/szarp/$PREFIX/config/params.xml does not exists, exiting...";
	exit 1;
fi

if [ $# -ne 2 ]; then
	Usage $0
	exit 1;
fi


DIRPATH=/opt/szarp/$PREFIX
if [ -e $DIRPATH/.svn ]; then
	echo "This configuration is already managed by svn, exiting";
	exit 1;
fi

echo "Checking if provided url ($REPO/$PREFIX) contains a svn repo.."
env SVN_SSH="ssh -l $USERNAME -p $PORT" svn ls $REPO/$PREFIX 1>/dev/null
if [ $? -ne 0 ]; then 
	echo "$REPO/$PREFIX is not a valid svn repository, no files will be modified";
	exit 1;
else
	echo "Ok";
fi
	
TMPDIR=$(mktemp -d "/opt/szarp/$PREFIX.XXXXXX")

cat <<EOF
Hello, that's what I'm gonna to do:
1. I will move all files and dirs from $DIRPATH except for 'szbase_stamp' and 'szbase' to $TMPDIR
2. Check out configuration files for $PREFIX from the repository into $DIRPATH
You'll have to type your password few times. Please check that after this operation completes
contents of $DIRPATH looks ok. SZARP must be turned off during execution of this script.
Press Enter to continue or Ctrl-C to exit.
EOF
read 

for i in $DIRPATH/*; do 
	if [ "$i" == "$DIRPATH/szbase" ]; then
		continue;
	elif [ "$i" == "$DIRPATH/szbase_stamp" ]; then
		continue;
	fi
		
	echo "Moving: $i to $TMPDIR";
	mv $i $TMPDIR || { echo "Failed to move $i, help!"; exit 1;}
done


echo "Checking out configuration"
CDIR=`pwd`
cd $DIRPATH;

echo Running SVN_SSH="ssh -l $USERNAME -p $PORT" svn co $REPO/$PREFIX . 
env SVN_SSH="ssh -l $USERNAME -p $PORT" svn co $REPO/$PREFIX . 

if [ $? -ne 0 ]; then 
	echo "Failed to fetch configuration, help!";
else
	echo "Ok";
fi

cd $CDIR

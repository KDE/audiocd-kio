#! /usr/bin/env bash

while read; do
    if [ "${REPLY#\[}" != "$REPLY" ] ; then # group name
	GROUP="${REPLY:1:${#REPLY}-2}"
	continue;
    fi
    # normal key=value pair:
    KEY="${REPLY%%=*}"
    VALUE="${REPLY#*=}"

    case "$GROUP/$KEY" in
	FileName/file_name_template)
			#
			# Swap
			#
			VALUE=`echo $VALUE | sed -e s/%a/%{albumtitle}/g`;
      VALUE=`echo $VALUE | sed -e s/%t/%{title}/g`;
      VALUE=`echo $VALUE | sed -e s/%n/%{number}/g`;
      VALUE=`echo $VALUE | sed -e s/%g/%{genre}/g`;
      VALUE=`echo $VALUE | sed -e s/%y/%{year}/g`;
      VALUE=`echo $VALUE | sed -e s/%A/%{albumartist}/g`;
	    echo "[FileName]";
			echo "file_name_template=$VALUE"
	    echo "# DELETE [FileName]file_name_template"
			;;
    esac
done

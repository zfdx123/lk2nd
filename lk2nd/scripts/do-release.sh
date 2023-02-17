#!/bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Nikita Travkin <nikita@trvn.ru>

# This script compiles listed projects from the lk2nd tree
# and renames it's artifacts of interest by the project name.

PROJECTS="
	lk2nd-msm8916
	lk2nd-msm8974
	lk2nd-msm8226
"

ARTIFACTS="
	lk2nd.img
"

# cd to the lk2nd root tree
SCRIPT_DIR=$(CDPATH="" cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR"/../.. || exit


if [ ! -d lk2nd ] || [ ! -f makefile ]
then
	echo "Failed to find the lk2nd project root"
	exit 1
fi

# Return -dirty or empty string
git_dirty() {
	if [ -n "$(git status --porcelain)" ]
	then
		echo "-dirty"
	fi
}

# Echo version of the project based on git
get_version_string() {
	if ! git rev-parse --is-inside-work-tree > /dev/null 2>&1
	then
		echo "unknown-$(date +%Y%m%d)"
		return
	fi

	head_rev=$(git rev-parse --short HEAD)
	head_date=$(git log -1 --format=%cd --date=format:"%Y%m%d")
	last_tag=$(git tag --sort=-taggerdate --merged | head -n1)
	if [ -z "$last_tag" ]
	then
		echo "next-$head_rev-$head_date$(git_dirty)"
		return
	fi

	last_tag_ref=$(git rev-list -n1 "$last_tag")
	if [ "$last_tag_ref" = "$(git rev-list -n1 HEAD)" ]
	then
		echo "$last_tag$(git_dirty)"
		return
	fi

	echo "$last_tag-next-$head_rev-$head_date$(git_dirty)"
}

usage() {
	echo "Usage: $0 [-hd] [project ...]"
	echo "Create tagged release builds for select lk2nd projects."
	echo
	echo "  -d    Only describe the version"
	echo "  -S    Sign the release"
	echo "  -h    This help"
	echo
}

sign_release=0

while getopts ":dhS" opt
do
	case $opt in
		S)
			sign_release=1
			;;
		d)
			get_version_string
			exit 0 ;;
		h)
			usage
			exit 0 ;;
		*)
			usage
			echo "Unkown option: -$OPTARG"
			echo
			exit 1 ;;
	esac
done
shift $((OPTIND-1))

if [ $# -ne 0 ]
then
	PROJECTS="$*"
fi

if [ "$(echo build-*)" != "build-*" ] && ! [ -d "build-*" ]
then
	echo "All existing build dirs will be removed."
	printf "Continue? [y]"
	read -r

	rm -rf build-*
fi

version_string="$(get_version_string)"

echo "Building release artifacts for $version_string"
echo

for proj in $PROJECTS
do
	echo "Building $proj"
	echo "----------------------"
	make -j"$(nproc)" TOOLCHAIN_PREFIX=arm-none-eabi- "$proj"
	echo
done

release_dir="release-$version_string"

if [ -d "$release_dir" ]
then
	echo "$release_dir already exists. It will be removed."
	printf "Continue? [y]"
	read -r

	rm -rf "$release_dir"
fi

mkdir "$release_dir"

for proj in $PROJECTS
do
	echo "Packing $proj"
	for artifact in $ARTIFACTS
	do
		if [ -e "build-$proj/$artifact" ]
		then
			new_name="$proj$(echo "$artifact" | sed "s/^lk2nd//")"
			echo "  $new_name"
			cp "build-$proj/$artifact" "$release_dir/$new_name"
		fi
	done
done

if [ $sign_release -eq 1 ]
then
	echo "Signing the release artifacts."
	cd "$release_dir" || exit
	sha256sum -- * > sha256sums.txt
	gpg --detach-sign sha256sums.txt
fi

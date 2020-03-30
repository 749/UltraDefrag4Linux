#!/bin/bash

version=$1

rm -rf doc src && \
curl -L https://sourceforge.net/projects/ultradefrag/files/stable-release/${version}/ultradefrag-${version}.src.7z/download >temp.7z && \
7z x temp.7z && \
read -p "[Enter] to continue" && \
rm -f temp.7z && \
#mv ultradefrag-*/* . && \
#rmdir ultradefrag-* && \
git add . && \
git restore --staged download.sh && \
echo "Please check git and push" && \
read -p "[Enter] to continue committing and taging" && \
git commit -m "[WIN] Import v${version}" && \
git tag "win_v${version}"

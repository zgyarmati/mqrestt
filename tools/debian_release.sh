#!/bin/sh

export DEBEMAIL="mr.zoltan.gyarmati@gmail.com"
export DEBFULLNAME="Zoltan Gyarmati"
export DEBDISTRO="stable"


if [ ! -f debian/changelog ]; then
    echo ">>> You must call this script from the project rootdir"
    exit 1
fi

BRANCH=`git rev-parse --abbrev-ref HEAD`
CURRENTVERSION=`dpkg-parsechangelog --show-field Version`
OLDVERSION=`dpkg-parsechangelog -o1 -c1 --show-field Version`
echo ">>> The currentversion to release: $CURRENTVERSION"
echo ">>> The OLD version used as baseline for the changelog: $OLDVERSION"

if git rev-parse $OLDVERSION >/dev/null 2>&1
then
    echo ">>> Found git tag for old version, attaching commits to dch"
    LASTCHANGES=`git log --reverse --pretty=format:%s $OLDVERSION..HEAD`
    # iterating through the commits, and add them to
    # changelog
    echo "$LASTCHANGES" | while read -r line; do
       echo "\t* $line"
       dch -a "$line"
    done
else
    echo ">>> Not found tag for previous debian version, leave the dch as is"
fi

## set the current version to released
dch --release "release $CURRENTVERSION" -D $DEBDISTRO
## commiting the change and tag it
git commit -am "release $CURRENTVERSION"
git tag $CURRENTVERSION

echo ">>> pushing changes to origin $BRANCH"
git push --tags origin "$BRANCH"

echo ">>> starting new version"
dch --upstream "starting new version"
FUTUREVERSION=`dpkg-parsechangelog --show-field Version`
echo ">>> Congratulations, now you can start to work on $FUTUREVERSION!"
echo ">>> If you don't like the autoincremented version, just modify it in
debian/changelog before the next release"

#! /usr/bin/env bash
subdirs="app kdchart massifdata visualizer"
rcfiles="`find $subdirs -name \*.rc`"
uifiles="`find $subdirs -name \*.ui`"
kcfgfiles="`find $subdirs -name \*.kcfg`"
if [[ "$rcfiles" != "" ]] ; then
    $EXTRACTRC $rcfiles >> rc.cpp || exit 11
fi
if [[ "$uifiles" != "" ]] ; then
    $EXTRACTRC $uifiles >> rc.cpp || exit 12
fi
if [[ "$kcfgfiles" != "" ]] ; then
    $EXTRACTRC $kcfgfiles >> rc.cpp || exit 13
fi
$XGETTEXT `find $subdirs -name \*.cc -o -name \*.cpp -o -name \*.h` rc.cpp -o $podir/massif-visualizer.pot
rm -f rc.cpp

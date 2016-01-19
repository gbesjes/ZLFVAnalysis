#!/bin/bash
# Installation:
#   cd my_gitproject
#   wget -O pre-commit.sh http://tinyurl.com/mkovs45
#   ln -s ../../pre-commit.sh .git/hooks/pre-commit
#   chmod +x pre-commit.sh

OPTIONS="--style=google --attach-classes --attach-inlines --attach-namespaces --indent-classes --indent-namespaces"

RETURN=0
ASTYLE=$(which astyle)
if [ $? -ne 0 ]; then
    echo "[!] astyle not installed. Unable to check source file format policy." >&2
    exit 1
fi

FILES=`git diff --cached --name-only --diff-filter=ACMR | grep -E "\.(c|cpp|cxx|h)$"`
for FILE in $FILES; do
    $ASTYLE $OPTIONS < $FILE | cmp -s $FILE -
    if [ $? -ne 0 ]; then
        echo "[!] $FILE does not respect the indentation." >&2
        RETURN=1
    fi
done

if [ $RETURN -eq 1 ]; then
    echo "" >&2
    echo "Make sure you have run astyle with the following options:" >&2
    echo astyle $OPTIONS >&2
fi

exit $RETURN

#!/bin/bash

set -e;

rm -rf /var/lib/apt/lists/* || true;
find /usr/share/locale -name '??' -o -name '??_??' | xargs rm -rf || true;
find /usr/share/doc -type f -a ! -iname 'copyright*' -delete || true;
find /usr/share/qt5/doc -type f -a ! -iname 'copyright*' -delete || true;
rm -rf /var/tmp/* || true;
rm -rf /tmp/* || true;
rm -rf /usr/share/man || true;

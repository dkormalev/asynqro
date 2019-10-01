#!/bin/bash

set -e;

rm -rf /var/lib/apt/lists/*;
find /usr/share/locale -name '??' -o -name '??_??' | xargs rm -rf;
find /usr/share/doc -type f -a ! -iname 'copyright*' -delete;
find /usr/share/qt5/doc -type f -a ! -iname 'copyright*' -delete;
rm -rf /var/tmp/*;
rm -rf /tmp/*;
rm -rf /usr/share/man;

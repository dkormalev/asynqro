FROM ubuntu:rolling

COPY build.sh /build.sh
COPY build_noqt.sh /build_noqt.sh
COPY codecov.sh /codecov.sh
COPY codestyle_check.sh /codestyle_check.sh
COPY clangtidy_check.sh /clangtidy_check.sh
COPY run-clang-tidy-asynqro.py /run-clang-tidy-asynqro.py

# Switch to system lcov when 1.14 will be released
RUN chmod +x /build.sh && chmod +x /codecov.sh && chmod +x /build_noqt.sh \
    && chmod +x /codestyle_check.sh && chmod +x /clangtidy_check.sh && chmod +x /run-clang-tidy-asynqro.py \
    && apt-get update \
    && apt-get install -y --no-install-recommends wget ca-certificates cmake make git clang clang-format-6.0 clang-tidy libc++abi-dev libc++-dev gcc g++ lcov qtbase5-dev \
    && apt-get -qq remove -y lcov \
    && mkdir /extra && cd /extra \
    && git clone https://github.com/linux-test-project/lcov.git && cd lcov && git checkout v1.14 && make install \
    && cd / && rm -rf /extra \
    && rm -rf /var/lib/apt/lists/* \
    && find /usr/share/locale -name '??' -o -name '??_??' | xargs rm -rf \
    && find /usr/share/doc -type f -a ! -iname 'copyright*' -delete \
    && find /usr/share/qt5/doc -type f -a ! -iname 'copyright*' -delete \
    && rm -rf /var/tmp/* \
    && rm -rf /tmp/* \
    && rm -rf /usr/share/man

ENV CTEST_OUTPUT_ON_FAILURE=1 \
    CLICOLOR_FORCE=1 \
    CC=clang \
    CXX=clang++

ENTRYPOINT /build.sh

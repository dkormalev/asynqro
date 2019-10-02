FROM ubuntu:eoan

COPY image_cleanup.sh /image_cleanup.sh

COPY build.sh /build.sh
COPY build_noqt.sh /build_noqt.sh
COPY codecov.sh /codecov.sh
COPY codestyle_check.sh /codestyle_check.sh
COPY clangtidy_check.sh /clangtidy_check.sh
COPY run-clang-tidy-asynqro.py /run-clang-tidy-asynqro.py

RUN chmod +x /image_cleanup.sh && chmod +x /build.sh && chmod +x /codecov.sh && chmod +x /build_noqt.sh \
    && chmod +x /codestyle_check.sh && chmod +x /clangtidy_check.sh && chmod +x /run-clang-tidy-asynqro.py \
    && apt-get update \
    && apt-get install -y --no-install-recommends wget ca-certificates cmake make git clang clang-format clang-tidy libc++abi-dev libc++-dev gcc g++ lcov qtbase5-dev python-yaml \
    && /image_cleanup.sh

ENV CTEST_OUTPUT_ON_FAILURE=1 \
    CLICOLOR_FORCE=1 \
    CC=clang \
    CXX=clang++

ENTRYPOINT /build.sh

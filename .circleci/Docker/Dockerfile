FROM ubuntu:19.04 as base

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    ninja-build \
    openssh-client \
    curl \
    gnupg2 \
    gosu \
    wget \
    locales \
    git \
    subversion \
    make \
    pkg-config \
    unzip \
    xz-utils \
    software-properties-common \
    sudo \
    apt-utils \
    && rm -rf /var/lib/apt/lists/*

# LLVM/Clang
ENV CLANG_VERSION=9
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
    && apt-add-repository "deb http://apt.llvm.org/disco/ llvm-toolchain-disco-$CLANG_VERSION main" \
    && apt-get update && apt-get install -y \
    clang-$CLANG_VERSION \
    clang-tidy-$CLANG_VERSION \
    clang-format-$CLANG_VERSION \
    llvm-$CLANG_VERSION-dev \
    libclang-$CLANG_VERSION-dev \
    && update-alternatives \
    --install /usr/bin/clang clang /usr/bin/clang-$CLANG_VERSION 100 \
    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-$CLANG_VERSION \
    --slave /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-$CLANG_VERSION \
    --slave /usr/bin/clang-format clang-format /usr/bin/clang-format-$CLANG_VERSION

# GCC
ENV GCC_VERSION=9
RUN sudo apt-get update \
    && sudo apt-get install -y --no-install-recommends \
    g++-$GCC_VERSION \
    gcc-$GCC_VERSION \
    && sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 100 --slave /usr/bin/g++ g++ /usr/bin/g++-9 --slave /usr/bin/gcov gcov /usr/bin/gcov-9

# Tomahawk deps
RUN sudo apt-get update \
    && apt-get install -y --no-install-recommends \
    cmake \
    libattica-dev \
    libboost-dev    \
    libboost-filesystem-dev \
    libboost-iostreams-dev \
    libboost-thread-dev \
    libfftw3-dev \
    libgnutls28-dev \
    libgsasl7-dev \
    liblastfm-dev \
    liblastfm5-dev \
    liblucene++-dev \
    libphonon-dev \
    libphononexperimental-dev \
    libqca-qt5-2-dev \
    libqca2-dev \
    libqca2-plugins \
    libqjson-dev \
    libqt5svg5-dev \
    libqt5webkit5-dev \
    libqt5webkit5\
    libsamplerate0-dev \
    libsparsehash-dev \
    libssl-dev \
    libtelepathy-qt5-dev \
    libvlc-dev \
    libvlccore-dev \
    libx11-dev \
    libz-dev \
    qt5-default \
    qtbase5-dev \
    qttools5-dev \
    qttools5-dev-tools \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone  --depth 1 https://github.com/zaphoyd/websocketpp.git --branch master --single-branch websocketpp \
    && mkdir websocketpp/build && cd websocketpp/build \
    && cmake .. \
    && sudo cmake --build . -j 16 --target install \
    && cd ../.. \
    && rm -r websocketpp

RUN git clone --depth 1 https://github.com/frankosterfeld/qtkeychain.git --branch master --single-branch qtkeychain \
    && mkdir qtkeychain/build && cd qtkeychain/build \
    && cmake .. \
    && sudo cmake --build . -j 16 --target install \
    && cd ../.. \
    && rm -r qtkeychain

RUN git clone --depth 1 https://github.com/taglib/taglib.git --branch master --single-branch taglib \
    && mkdir taglib/build && cd taglib/build \
    && cmake -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_BUILD_TYPE=Release  \
      -DBUILD_SHARED_LIBS=ON .. \
    && sudo cmake --build . -j 16 --target install \
    && cd ../.. \
    && rm -r taglib

RUN git clone --depth 1 https://anongit.kde.org/extra-cmake-modules.git --branch master --single-branch ecm \
    && mkdir ecm/build && cd ecm/build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && sudo cmake --build . -j 16 --target install \
    && cd ../.. \
    && rm -r ecm

RUN git clone --depth 1 https://github.com/KDE/attica.git --branch master --single-branch attica \
    && mkdir attica/build && cd attica/build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && sudo cmake --build . -j 16 --target install \
    && cd ../.. \
    && rm -r attica

RUN git clone --depth 1 https://github.com/stachenov/quazip.git --branch master --single-branch quazip \
    && mkdir quazip/build && cd quazip/build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && sudo cmake --build . -j 16 --target install \
    && cd ../.. \
    && rm -r quazip

RUN git clone --depth 1 https://github.com/euroelessar/jreen.git --branch master --single-branch jreen \
    && mkdir jreen/build && cd jreen/build \
    && cmake -DCMAKE_BUILD_TYPE=Release .. \
    && sudo cmake --build . -j 16 --target install \
    && cd ../.. \
    && rm -r jreen

# Language
ENV LANG=en_US.UTF-8
RUN echo "$LANG UTF-8" > /etc/locale.gen && locale-gen $LANG && update-locale LANG=$LANG

#entrypoint, if it is last here makes it easy to build new image without rebuilding all layers
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
COPY build-and-test.sh /usr/local/bin/build-and-test.sh
RUN chmod +x /usr/local/bin/build-and-test.sh
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

WORKDIR /tmp/workspace

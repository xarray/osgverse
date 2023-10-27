# !IMPORTANT!
# * Pass -m 2GB (or more) when building the image.
# * Configure Docker to use disks larger than the default 20 GB.

FROM ubuntu:20.04
LABEL author="xarray"
LABEL website="https://gitee.com/xarray/osgverse"

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        apt-transport-https build-essential ca-certificates \
        curl cmake dirmngr git gnupg gcc g++ \
        nasm npm pkg-config software-properties-common sudo unzip zip \
        libxext-dev libz-dev libjpeg-dev libpng-dev libcurl4-openssl-dev \
        libavdevice-dev libavcodec-dev libavutil-dev libavformat-dev libswscale-dev \
        libswresample-dev libx264-dev libegl1-mesa-dev libgles2-mesa-dev mesa-common-dev
RUN mkdir -p /home/osgv; \
    mkdir -p /home/osgv/Source; mkdir -p /home/osgv/Dependencies/x64; \
    mkdir -p /home/osgv/Builds; mkdir -p /home/osgv/Release

# Download OpenSceneGraph, osgVerse and 3rdparty libraries
RUN cd /home/osgv/Source; \
    git clone --branch 3.25 https://github.com/bulletphysics/bullet3.git bullet3
RUN cd /home/osgv/Source; \
    git clone --branch 2.5.0 https://github.com/cisco/libsrtp.git libsrtp; \
RUN cd /home/osgv/Source; \
    git clone https://github.com/ZLMediaKit/ZLMediaKit.git ZLMediaKit; \
    cd /home/osgv/Source/ZLMediaKit; git submodule update --init --recursive
RUN cd /home/osgv/Source; \
    git clone --branch 1.5.6 https://github.com/google/draco.git draco
RUN cd /home/osgv/Source; \
    git clone --branch release-2.28.4 https://github.com/libsdl-org/SDL.git SDL
RUN cd /home/osgv/Source; \
    git clone --branch OpenSceneGraph-3.6.5 https://github.com/openscenegraph/OpenSceneGraph.git OpenSceneGraph
RUN cd /home/osgv; \
    git clone https://github.com/xarray/osgverse.git osgverse

# Build 3rdparty libraries
RUN mkdir -p /home/osgv/Builds/bullet3; cd /home/osgv/Builds/bullet3; \
    cmake -DCMAKE_INSTALL_PREFIX=/home/osgv/Dependencies/x64 -DCMAKE_BUILD_TYPE=Release ../../Source/bullet3; \
    make; make install
#RUN mkdir -p /home/osgv/Builds/ZLMediaKit; cd /home/osgv/Builds/ZLMediaKit; \
#    cmake -DCMAKE_INSTALL_PREFIX=/home/osgv/Dependencies/x64 -DCMAKE_BUILD_TYPE=Release ../../Source/ZLMediaKit; \
#    make; make install
RUN mkdir -p /home/osgv/Builds/draco; cd /home/osgv/Builds/draco; \
    cmake -DCMAKE_INSTALL_PREFIX=/home/osgv/Dependencies/x64 -DCMAKE_BUILD_TYPE=Release ../../Source/draco; \
    make; make install
RUN mkdir -p /home/osgv/Builds/SDL; cd /home/osgv/Builds/SDL; \
    cmake -DCMAKE_INSTALL_PREFIX=/home/osgv/Dependencies/x64 -DCMAKE_BUILD_TYPE=Release ../../Source/SDL; \
    make; make install

# Build OpenSceneGraph
RUN mkdir -p /home/osgv/Builds/OpenSceneGraph; cd /home/osgv/Builds/OpenSceneGraph; \
    cmake -DCMAKE_INSTALL_PREFIX=/home/osgv/Release -DCMAKE_BUILD_TYPE=Release ../../Source/OpenSceneGraph; \
    make; make install
ENV OSG_DIR /home/osgv/Release

# Build osgverse
RUN mkdir -p /home/osgv/Builds/osgverse; cd /home/osgv/Builds/osgverse; \
    cmake -DCMAKE_INSTALL_PREFIX=/home/osgv/Release -DCMAKE_BUILD_TYPE=Release -DVERSE_FIND_LEGACY_OPENGL=ON ../../osgverse; \
    make; make install

# Finished!
RUN bash -c 'echo -e "%manavr ALL=(ALL) NOPASSWD: ALL\n" > /etc/sudoers.d/01_manavr'
RUN addgroup manavr && useradd -u 1000 -m osgv && usermod -aG manavr osgv

USER osgv
WORKDIR /home/osgv
CMD ["/bin/bash"]

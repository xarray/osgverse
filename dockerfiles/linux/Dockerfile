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
        libegl1-mesa-dev libgles2-mesa-dev mesa-common-dev \
        nasm npm pkg-config software-properties-common \
        sudo unzip zip
RUN mkdir -p /home/osgv; mkdir -p /home/osgv/builds; mkdir -p /home/osgv/release

# Download OpenSceneGraph, osgVerse and 3rdparty libraries
RUN cd /home/osgv; \
    git clone --branch OpenSceneGraph-3.6.5 https://github.com/openscenegraph/OpenSceneGraph.git OpenSceneGraph

# Build OpenSceneGraph
RUN mkdir -p /home/osgv/builds/OpenSceneGraph; cd /home/osgv/builds/OpenSceneGraph; \
    cmake -DCMAKE_INSTALL_PREFIX=/home/osgv/release -DCMAKE_BUILD_TYPE=Release ../../OpenSceneGraph; \
    make; make install

# Finished!
WORKDIR /home/osgv
CMD ["/bin/bash"]

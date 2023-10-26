# !IMPORTANT!
# * Pass -m 2GB (or more) when building the image.
# * Configure Docker to use disks larger than the default 20 GB.

FROM mcr.microsoft.com/windows/servercore:ltsc2019
LABEL author="xarray"
LABEL website="https://gitee.com/xarray/osgverse"

# Visual Studio version: 2017 = 15, 2022 = 17
ENV MSVC_VERSION "17"

SHELL ["cmd", "/S", "/C"]
COPY Install.cmd C:\TEMP
ADD https://aka.ms/vscollect.exe C:\TEMP\collect.exe

# Install Node.js LTS
ADD https://nodejs.org/dist/v8.11.3/node-v8.11.3-x64.msi C:\TEMP\node-install.msi
RUN start /wait msiexec.exe /i C:\TEMP\node-install.msi /l*vx "%TEMP%\MSI-node-install.log" /qn ADDLOCAL=ALL

# Download channel for fixed install.
ARG CHANNEL_URL=https://aka.ms/vs/%MSVC_VERSION%/release/channel
ADD ${CHANNEL_URL} C:\TEMP\VisualStudio.chman

# Download and install Build Tools for Visual Studio for native desktop workload.
ADD https://aka.ms/vs/%MSVC_VERSION%/release/vs_buildtools.exe C:\TEMP\vs_buildtools.exe
RUN C:\TEMP\Install.cmd C:\TEMP\vs_buildtools.exe --quiet --wait --norestart --nocache --channelUri C:\TEMP\VisualStudio.chman --installChannelUri C:\TEMP\VisualStudio.chman --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --installPath C:\BuildTools

# Install Choco
RUN powershell -Command \
    iex ((new-object net.webclient).DownloadString('https://chocolatey.org/install.ps1')); \
    choco feature disable --name showDownloadProgress
RUN choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=System'
RUN choco install -y git.install --params "/GitOnlyOnPath /NoGitLfs"

# Use developer command prompt
ENTRYPOINT C:\BuildTools\Common7\Tools\VsDevCmd.bat

# TODO...

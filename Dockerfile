FROM ubuntu:18.04
ENV DEBIAN_FRONTEND=noninteractive 
RUN apt-get update && apt-get install -y -qq \
    pkg-config libtool build-essential libcurl4-gnutls-dev \
    asciidoc dblatex libmosquitto-dev libconfuse-dev \
    libmicrohttpd-dev autoconf-archive
COPY . /app
RUN cd /app && ./autogen.sh && ./configure && make -j
CMD /app/src/mqrestt



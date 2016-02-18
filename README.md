# mod_kafka

mod_kafka is Kafka data collector module for Apache HTTPD Server.

## Requires

* [librdkafka](https://github.com/edenhill/librdkafka.git)

## Build

```
% git clone --depth=1 https://github.com/kjdev/apache-mod-kafka.git
% cd apache-mod-kafka
```

### Used embeddeded librdkafka [DEFAULT]

```
% git submodule update --init
% ./autogen.sh # OR autoreconf -i
% ./configure [OPTION]
% make
```

### Installed librdkafka

[librdkafka](https://github.com/edenhill/librdkafka.git)

```
% ./autogen.sh # OR autoreconf -i
% ./configure --with-rdkafka=/usr/include --with-rdkafka-libdir=/usr/lib [OPTION]
% make
```

### Build options

embeddeded librdkafka.

* `--enable-ssl`: use SSL library [default=yes]
* `--enable-sasl`: use Cyrus SASL library [default=yes]
* `--enable-zlib`: use Zlib library [default=yes]

librdkafka path (installed librdkafka).

* `--with-rdkafka=PATH`: find a __rdkafka.h__
* `--with-rdkafka-libdir=PATH`: find a __librdkafka.so__

apache path.

* `--with-apxs=PATH`
* `--with-apr=PATH`

## Install

```
$ install -p -m 755 -D .libs/mod_kafka.so /etc/httpd/modules/mod_kafka.so
```

## Configration

`httpd.conf`:

```
# Load module
LoadModule kafka_module modules/mod_kafka.so

# Kafka brokers: host[:port][,host[:port]] (DEFAULT: localhost:9092)
KafkaBrokers localhost:9092

# Kafka config: optional
KafkaConf global reconnect.backoff.jitter.ms 1000
KafkaConf topic request.timeout.ms 10000

# Kafka output: kafka:topic[@partition] log
CustomLog kafka:test combined
```

See [configration](https://github.com/edenhill/librdkafka/blob/master/CONFIGURATION.md).

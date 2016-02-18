
ACLOCAL_AMFLAGS = -I m4

moddir = @APACHE_MODULEDIR@
mod_LTLIBRARIES = mod_kafka.la

librdkafka_SOURCES = \
	librdkafka/src/rdaddr.c \
	librdkafka/src/rdcrc32.c \
	librdkafka/src/rdkafka.c \
	librdkafka/src/rdkafka_assignor.c \
	librdkafka/src/rdkafka_broker.c \
	librdkafka/src/rdkafka_buf.c \
	librdkafka/src/rdkafka_cgrp.c \
	librdkafka/src/rdkafka_conf.c \
	librdkafka/src/rdkafka_msg.c \
	librdkafka/src/rdkafka_offset.c \
	librdkafka/src/rdkafka_op.c \
	librdkafka/src/rdkafka_partition.c \
	librdkafka/src/rdkafka_pattern.c \
	librdkafka/src/rdkafka_queue.c \
	librdkafka/src/rdkafka_range_assignor.c \
	librdkafka/src/rdkafka_request.c \
	librdkafka/src/rdkafka_roundrobin_assignor.c \
	librdkafka/src/rdkafka_subscription.c \
	librdkafka/src/rdkafka_timer.c \
	librdkafka/src/rdkafka_topic.c \
	librdkafka/src/rdkafka_transport.c \
	librdkafka/src/rdlist.c \
	librdkafka/src/rdlog.c \
	librdkafka/src/rdrand.c \
	librdkafka/src/snappy.c \
	librdkafka/src/tinycthread.c

if HAVE_ZLIB
librdkafka_zlib_SOURCES = librdkafka/src/rdgz.c
else
librdkafka_zlib_SOURCES =
endif

if HAVE_SASL
librdkafka_sasl_SOURCES = librdkafka/src/rdkafka_sasl.c
else
librdkafka_sasl_SOURCES =
endif

if HAVE_LIBRDKAFKA
mod_kafka_la_SOURCES = \
	mod_kafka.c \
	$(librdkafka_SOURCES) $(librdkafka_zlib_SOURCES) $(librdkafka_sasl_SOURCES)
else
mod_kafka_la_SOURCES = mod_kafka.c
endif

mod_kafka_la_CFLAGS = @APACHE_CFLAGS@ @LIBRDKAFKA_CFLAGS@
mod_kafka_la_CPPFLAGS = @APACHE_CPPFLAGS@ @LIBRDKAFKA_CPPFLAGS@
mod_kafka_la_LDFLAGS = -avoid-version -module @APACHE_LDFLAGS@ @LIBRDKAFKA_LDFLAGS@
mod_kafka_la_LIBS = @APACHE_LIBS@ @LIBRDKAFKA_LIBS@

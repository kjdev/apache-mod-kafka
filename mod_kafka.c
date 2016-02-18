/*
 *  mod_kafka.c -- Apache sample kafka module
 *
 *  Then activate it in Apache's httpd.conf:
 *
 *    # httpd.conf
 *    LoadModule kafka_module modules/mod_kafka.so
 *    KafkaBrokers localhost:9092
 *    CustomLog kafka:topic[@partition] combined
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#  undef PACKAGE_NAME
#  undef PACKAGE_STRING
#  undef PACKAGE_TARNAME
#  undef PACKAGE_VERSION
#endif

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "ap_config.h"

#include "http_log.h"
#include "ap_mpm.h"
#include "apr_strings.h"
#include "apr_anylock.h"
#include "mod_log_config.h"
#include "apr_hash.h"

#include "rdkafka.h"

#ifndef KAFKA_DEBUG_LOG_LEVEL
#define KAFKA_DEBUG_LOG_LEVEL APLOG_DEBUG
#endif

#ifdef NDEBUG
#define DEBUG(p, format, args...)
#else
#define DEBUG(p, format, args...) ap_log_perror(APLOG_MARK, KAFKA_DEBUG_LOG_LEVEL, 0, p, "[kafka] %s(%d): "format, __FILE__, __LINE__, ##args)
#endif
#define ERROR(p, format, args...) ap_log_perror(APLOG_MARK, APLOG_ERR, 0, p, "[kafka] %s(%d): "format, __FILE__, __LINE__, ##args)

#define KAFKA_PREFIX "kafka:"
#define KAFKA_DEFAULT_BROKERS "localhost:9092"


module AP_MODULE_DECLARE_DATA kafka_module;

static int mpm_threads = 0;

char kafka_log_dummy[16];

typedef struct {
  struct {
    apr_hash_t *global;
    apr_hash_t *topic;
  } conf;
  char *brokers;
  rd_kafka_t *rk;
  apr_hash_t *topics;
} kafka_t;

typedef struct {
  apr_anylock_t mutex;
  apr_pool_t *pool;
  kafka_t kafka;
} kafka_config_server;

typedef struct {
  char *dummy;
  char *topic;
  int32_t partition;
} kafka_log_t;

static APR_OPTIONAL_FN_TYPE(ap_log_set_writer_init) *log_writer_init;
static APR_OPTIONAL_FN_TYPE(ap_log_set_writer) *log_writer;

static ap_log_writer_init *default_log_writer_init = NULL;
static ap_log_writer *default_log_writer = NULL;


static apr_status_t
kafka_connect(apr_pool_t *p, kafka_t *kafka)
{
  char *brokers = kafka->brokers;

  if (!brokers || strlen(brokers) == 0) {
    brokers = KAFKA_DEFAULT_BROKERS;
  }

  /* Configuration */
  rd_kafka_conf_t *conf = rd_kafka_conf_new();
  if (!conf) {
    ERROR(p, "Init Kafka conf");
    return APR_EINIT;
  }

  /* Quick termination */
  char tmp[512];
  snprintf(tmp, sizeof(tmp), "%i", SIGIO);
  rd_kafka_conf_set(conf, "internal.termination.signal", tmp, NULL, 0);

  /* Set configuration */
  apr_hash_index_t *hash = apr_hash_first(p, kafka->conf.global);
  while (hash) {
    const void *property = NULL;
    void *value = NULL;
    apr_hash_this(hash, &property, NULL, &value);
    if (value) {
      DEBUG(p, "global configration: %s = %s", (char *)property, (char *)value);

      if (rd_kafka_conf_set(conf, (char *)property, (char *)value,
                            tmp, sizeof(tmp)) != RD_KAFKA_CONF_OK) {
        ERROR(p, "Kafka config: %s", tmp);
        rd_kafka_conf_destroy(conf);
        return APR_EINIT;
      }
    }
    hash = apr_hash_next(hash);
  }

  /* Create producer handle */
  kafka->rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, NULL, 0);
  if (!kafka->rk) {
    ERROR(p, "Kafka producer init");
    rd_kafka_conf_destroy(conf);
    return APR_EINIT;
  }
  conf = NULL;

  DEBUG(p, "Create Kafka producer");

  rd_kafka_set_log_level(kafka->rk, 0); /* TODO */

  /* Add brokers */
  if (rd_kafka_brokers_add(kafka->rk, brokers) == 0) {
    ERROR(p, "Add Kafka brokers: %s", brokers);
    rd_kafka_destroy(kafka->rk);
    kafka->rk = NULL;
    return APR_EINIT;
  }

  DEBUG(p, "Add Kafka brokers: %s", brokers);

  return APR_SUCCESS;
}

static rd_kafka_topic_t *
kafka_topic_connect(apr_pool_t *p, kafka_t *kafka, char *topic)
{
  if (!topic || strlen(topic) == 0) {
    ERROR(p, "No such Kafka topic");
    return NULL;
  }

  if (!kafka->rk) {
    if (kafka_connect(p, kafka) != APR_SUCCESS) {
      return NULL;
    }
  }

  /* Fetch topic handle */
  rd_kafka_topic_t *rkt;
  rkt = (rd_kafka_topic_t *)apr_hash_get(kafka->topics,
                                         topic, APR_HASH_KEY_STRING);
  if (rkt) {
    return rkt;
  }

  /* Configuration topic */
  rd_kafka_topic_conf_t *topic_conf = rd_kafka_topic_conf_new();
  if (!topic_conf) {
    ERROR(p, "Init Kafka topic conf");
    return NULL;
  }

  /* Set configuration topic */
  apr_hash_index_t *hash = apr_hash_first(p, kafka->conf.topic);
  while (hash) {
    const void *property = NULL;
    void *value = NULL;
    apr_hash_this(hash, &property, NULL, &value);
    if (value) {
      char err[512];
      DEBUG(p, "topic configration: %s = %s", (char *)property, (char *)value);

      if (rd_kafka_topic_conf_set(topic_conf, (char *)property, (char *)value,
                                  err, sizeof(err)) != RD_KAFKA_CONF_OK) {
        ERROR(p, "Kafka topic config: %s", err);
        rd_kafka_topic_conf_destroy(topic_conf);
        return NULL;
      }
    }
    hash = apr_hash_next(hash);
  }

  /* Create topic handle */
  rkt = rd_kafka_topic_new(kafka->rk, topic, topic_conf);
  if (!rkt) {
    ERROR(p, "Kafka topic init");
    rd_kafka_topic_conf_destroy(topic_conf);
    return NULL;
  }
  topic_conf = NULL;

  DEBUG(p, "Create Kafka: topic = %s", topic);

  apr_hash_set(kafka->topics, topic, APR_HASH_KEY_STRING, (const void *)rkt);

  return rkt;
}

static void
kafka_produce(apr_pool_t *p, kafka_t *kafka,
              char *topic, int32_t partition, char *msg)
{
  rd_kafka_topic_t *rkt = kafka_topic_connect(p, kafka, topic);
  if (rkt) {
    DEBUG(p, "produce: (%s:%i) %s", topic, partition, msg);

    /* Produce send */
    if (rd_kafka_produce(rkt, partition, RD_KAFKA_MSG_F_COPY,
                         msg, strlen(msg), NULL, 0, NULL) == -1) {
      ERROR(p, "Failed to produce to topic %s partition %i: %s",
            rd_kafka_topic_name(rkt), partition,
            rd_kafka_err2str(rd_kafka_errno2err(errno)));
    }

    /* Poll to handle delivery reports */
    rd_kafka_poll(kafka->rk, 10);
  } else {
    ERROR(p, "No such kafka topic: %s", topic);
  }
}

static void *
kafka_log_writer_init(apr_pool_t *p, server_rec *s, const char * name)
{
  DEBUG(p, "writer_init name = %s", name);

  if (strncasecmp(KAFKA_PREFIX, name, sizeof(KAFKA_PREFIX) - 1) != 0) {
    if (default_log_writer_init) {
      return default_log_writer_init(p, s, name);
    }
    return NULL;
  }

  kafka_log_t *log = (kafka_log_t *)apr_palloc(p, sizeof(kafka_log_t));

  log->dummy = &kafka_log_dummy[0];

  char *topic = apr_pstrdup(p, name + sizeof(KAFKA_PREFIX) - 1);
  int32_t partition = RD_KAFKA_PARTITION_UA;
  char *c;

  if ((c = strchr(topic, '@')) != NULL) {
    uint32_t i = c - topic;
    topic[i++] = 0;
    partition = atoi(topic + i);
  }

  log->topic = topic;
  log->partition = partition;

  return log;
}

static apr_status_t
kafka_log_writer(request_rec *r, void *handle, const char **strs, int *strl,
                 int nelts, apr_size_t len)
{
  kafka_log_t *log = (kafka_log_t *)handle;
  if (log->dummy != kafka_log_dummy) {
    if (default_log_writer) {
      return default_log_writer(r, handle, strs, strl, nelts, len);
    }
    return OK;
  }

  int i;
  char *s, *msg = apr_palloc(r->pool, len + 1);
  for (i = 0, s = msg; i < nelts; ++i) {
    memcpy(s, strs[i], strl[i]);
    s += strl[i];
  }
  msg[len] = '\0';

  kafka_config_server *config;
  config = ap_get_module_config(r->server->module_config, &kafka_module);
  if (APR_ANYLOCK_LOCK(&config->mutex) == APR_SUCCESS) {
    kafka_produce(r->pool, &config->kafka, log->topic, log->partition, msg);
    APR_ANYLOCK_UNLOCK(&config->mutex);
  }

  return OK;
}

static const char *
kafka_set_brokers(cmd_parms *cmd, void *dummy, const char *arg)
{
  DEBUG(cmd->pool, "set brokers: %s", arg);
  if (!arg || strlen(arg) == 0) {
    return "KafkaBrokers argument must be a server[:port][,..]";
  }

  kafka_config_server *config;
  config = ap_get_module_config(cmd->server->module_config, &kafka_module);
  config->kafka.brokers = apr_pstrdup(cmd->pool, arg);

  return NULL;
}

static const char *
kafka_set_conf(cmd_parms *cmd, void *dummy, const char *arg)
{
  DEBUG(cmd->pool, "set conf: %s", arg);

  const char *type = ap_getword_conf(cmd->pool, &arg);
  const char *property = ap_getword_conf(cmd->pool, &arg);
  const char *value = ap_getword_conf(cmd->pool, &arg);

  if (!type || !property || !value
      || strlen(type) == 0 || strlen(property) == 0 || strlen(value) == 0) {
    return "KafkaConf argument must be a '[global|topic] <property> <value>'";
  }

  DEBUG(cmd->pool, "type = %s, property = %s, value = %s",
        type, property, value);

  kafka_config_server *config;
  config = ap_get_module_config(cmd->server->module_config, &kafka_module);

  if (strcmp(type, "global") == 0) {
    apr_hash_set(config->kafka.conf.global, property, APR_HASH_KEY_STRING,
                 (const void *)value);
  } else if(strcmp(type, "topic") == 0) {
    apr_hash_set(config->kafka.conf.topic, property, APR_HASH_KEY_STRING,
                 (const void *)value);
  } else {
    return "KafkaConf argument must be a '[global|topic] <property> <value>'";
  }

  return NULL;
}

static apr_status_t
kafka_cleanup(void *arg)
{
  kafka_config_server *config = (kafka_config_server *)arg;
  if (!config) {
    return APR_SUCCESS;
  }

  DEBUG(config->pool, "kafka_cleanup");

  apr_status_t rv;
  if ((rv = APR_ANYLOCK_LOCK(&config->mutex)) != APR_SUCCESS) {
    return rv;
  }

  if (config->kafka.rk) {
    DEBUG(config->pool, "Poll to handle delivery reports");
    rd_kafka_poll(config->kafka.rk, 0);

    DEBUG(config->pool, "Wait for messages to be delivered");
    while (rd_kafka_outq_len(config->kafka.rk) > 0) {
      rd_kafka_poll(config->kafka.rk, 10);
    }

    DEBUG(config->pool, "Destroy topic");
    apr_hash_index_t *hash = apr_hash_first(config->pool, config->kafka.topics);
    while (hash) {
      const void *topic = NULL;
      void *rkt = NULL;
      apr_hash_this(hash, &topic, NULL, &rkt);
      if (rkt) {
        DEBUG(config->pool, "kafka topic = %s", (char *)topic);
        rd_kafka_topic_destroy((rd_kafka_topic_t *)rkt);
      }
      hash = apr_hash_next(hash);
    }

    DEBUG(config->pool, "Destroy producer handle");
    rd_kafka_destroy(config->kafka.rk);
    config->kafka.rk = NULL;

    DEBUG(config->pool, "Let backgournd threds clean up");
    int32_t i = 5;
    while (i-- > 0 && rd_kafka_wait_destroyed(500) == -1) {
      ;
    }
  }

  APR_ANYLOCK_UNLOCK(&config->mutex);

  DEBUG(config->pool, "terminate cleanly");

  return APR_SUCCESS;
}

static int
kafka_pre_config(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp)
{
  if (!log_writer_init) {
    log_writer_init = APR_RETRIEVE_OPTIONAL_FN(ap_log_set_writer_init);
    log_writer = APR_RETRIEVE_OPTIONAL_FN(ap_log_set_writer);
  }

  if (!default_log_writer_init) {
    void *log = log_writer_init(kafka_log_writer_init);
    if (log != kafka_log_writer_init) {
      default_log_writer_init = log;
    }

    log = log_writer(kafka_log_writer);
    if (log != kafka_log_writer) {
      default_log_writer = log;
    }
  }

  return OK;
}

static void *
kafka_create_server_config(apr_pool_t *p, server_rec *s)
{
  kafka_config_server *config = apr_pcalloc(p, sizeof(kafka_config_server));
  if (!config) {
    return NULL;
  }

  DEBUG(p, "create_server_config");

  ap_mpm_query(AP_MPMQ_MAX_THREADS, &mpm_threads);

#if APR_HAS_THREADS
  if (mpm_threads > 1) {
    config->mutex.type = apr_anylock_threadmutex;
    apr_status_t rv = apr_thread_mutex_create(&config->mutex.lock.tm,
                                              APR_THREAD_MUTEX_DEFAULT, p);
    if (rv != APR_SUCCESS) {
      ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, APLOGNO(00647)
                   "could not initialize kafka module mutex");
      config->mutex.type = apr_anylock_none;
    }
  }
  else
#endif
  {
    config->mutex.type = apr_anylock_none;
  }

  config->pool = p;

  config->kafka.conf.global = apr_hash_make(p);
  config->kafka.conf.topic = apr_hash_make(p);
  config->kafka.brokers = NULL;
  config->kafka.rk = NULL;
  config->kafka.topics = apr_hash_make(p);

  return config;
}

static void
kafka_child_init(apr_pool_t *p, server_rec *s)
{
  kafka_config_server *config;
  config = ap_get_module_config(s->module_config, &kafka_module);
  if (config) {
    apr_pool_cleanup_register(p, config, kafka_cleanup, kafka_cleanup);
  }
}

static const command_rec kafka_cmds[] = {
  AP_INIT_TAKE1("KafkaBrokers", kafka_set_brokers, NULL, RSRC_CONF,
                "Kafka brokers"),
  AP_INIT_RAW_ARGS("KafkaConf", kafka_set_conf, NULL, RSRC_CONF,
                   "Kafka configuration"),
  {NULL}
};

static void
kafka_register_hooks(apr_pool_t *p)
{
  static const char *preload[] = { "mod_log_config.c", NULL };
  ap_hook_pre_config(kafka_pre_config, preload, NULL, APR_HOOK_REALLY_LAST);
  ap_hook_child_init(kafka_child_init, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA kafka_module = {
  STANDARD20_MODULE_STUFF,
  NULL,                       /* create per-dir    config structures */
  NULL,                       /* merge  per-dir    config structures */
  kafka_create_server_config, /* create per-server config structures */
  NULL,                       /* merge  per-server config structures */
  kafka_cmds,                 /* table of config file commands       */
  kafka_register_hooks        /* register hooks                      */
};

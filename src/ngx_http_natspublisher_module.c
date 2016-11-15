/*
 * Author: Guo-Guang Chiou
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_nats.h>
#include <ngx_nats_comm.h>

static char *ngx_http_natspublisher(ngx_conf_t *cf, void *post, void *data1);
static void ngx_http_post_handler(ngx_http_request_t *r); 
static ngx_conf_post_handler_pt ngx_http_natspublisher_p = ngx_http_natspublisher;

/* structure holding the value of the module directive natspublisher */
typedef struct {
  ngx_str_t subject;
} ngx_http_natspublisher_loc_conf_t;

/* The function initilizes memory for module configuration structure */
static void * ngx_http_natspublisher_create_loc_conf(ngx_conf_t *cf){
  ngx_http_natspublisher_loc_conf_t *conf;
  conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_natspublisher_loc_conf_t));
  if(conf == NULL) {
    return NULL;
  }
  return conf;
}

/* The command array of array */
static ngx_command_t ngx_http_natspublisher_commands[] = {
  {  ngx_string("natspublisher"), 
     NGX_HTTP_LOC_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_str_slot,
     NGX_HTTP_LOC_CONF_OFFSET,
     offsetof(ngx_http_natspublisher_loc_conf_t, subject),
     &ngx_http_natspublisher_p
  },
  ngx_null_command
};

/* subject defined in nginx.conf */
static ngx_str_t natspublisher_subject;

/*  The module context hooks location configuration */
static ngx_http_module_t ngx_http_natspublisher_module_ctx = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  ngx_http_natspublisher_create_loc_conf,
  NULL
};

/* bind context and commands */
ngx_module_t ngx_http_natspublisher_module = {
  NGX_MODULE_V1,
  &ngx_http_natspublisher_module_ctx,
  ngx_http_natspublisher_commands,
  NGX_HTTP_MODULE,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NGX_MODULE_V1_PADDING
};

static void ngx_http_post_handler(ngx_http_request_t *r) {
  if(r->request_body == NULL || 
     r->request_body->bufs == NULL) { 
    return;
  } 

  u_char *p = NULL;
  size_t len = 0;
  ngx_buf_t *buf;
  ngx_chain_t *cl;
  cl = r->request_body->bufs;
  if(cl != NULL) {
    for(/* void */; cl != NULL; cl = cl->next) {
      buf = cl->buf;
      if(buf != NULL) {
        len += buf->last - buf->pos;
      }
    }
    if(((int)len) > 0) {
      p = ngx_pnalloc(r->pool, len);
      if(p == NULL) {
        ngx_log_stderr(0,"%s: Failed to allocate memory", __func__);
        ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return;
      }
      cl = r->request_body->bufs;
      for(/* void */; cl != NULL; cl = cl->next) {
        buf = cl->buf;
        ngx_cpymem(p, buf->pos, buf->last - buf->pos);
      } 
      ngx_nats_client_t *ncf = ngx_http_get_module_loc_conf(r, ngx_http_natspublisher_module);
      ngx_nats_publish(ncf, &natspublisher_subject, NULL, p, len);
      ngx_http_finalize_request(r,NGX_DONE);
    }
  } else {
    ngx_log_stderr(0,"%s: chain is null", __func__);
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "chain is null");
  }
  return;
}

/* manin handler */
static ngx_int_t ngx_http_natspublisher_handler(ngx_http_request_t *r) {
  ngx_int_t rc;
  ngx_buf_t *b = NULL;
  ngx_chain_t out;
  
  /* reject 'DELETE', 'PUT', and 'HEAD' */
  if(!(r->method & (NGX_HTTP_GET | NGX_HTTP_POST))) {
    return NGX_HTTP_NOT_ALLOWED;
  }

  if(r->method & NGX_HTTP_POST) {
    rc = ngx_http_read_client_request_body(r, ngx_http_post_handler);
    if(rc >= NGX_HTTP_SPECIAL_RESPONSE) {
      return rc;
    }
  }

  /* discard request body */
  rc = ngx_http_discard_request_body(r);
  if(rc != NGX_OK) {
    return rc;
  }

  /* set the 'Content-type' header */
  r->headers_out.content_type_len = sizeof("text/html") - 1;
  r->headers_out.content_type.len = sizeof("text/html") - 1;
  r->headers_out.content_type.data = (u_char *) "text/html";

  /*allocate a buffer for your response body */
  b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
  if(b == NULL) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }
  out.buf = b;
  out.next = NULL;
  
  /* adjust the pointers of the buffer */
  b->pos = natspublisher_subject.data;
  b->last = natspublisher_subject.data + natspublisher_subject.len;
  b->memory = 1;
  b->last_buf = 1;

  /* set the status line */
  r->headers_out.status = NGX_HTTP_OK;
  r->headers_out.content_length_n = natspublisher_subject.len;

  /* send the headers of your response */
  rc = ngx_http_send_header(r);
  if(rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
    return rc;
  }

  /* send the buffer chain of your response */
  return ngx_http_output_filter(r, &out);
}

/* Function for the directive natspublisher */
static char * ngx_http_natspublisher(ngx_conf_t *cf, void *post, void *data1) {
  ngx_http_core_loc_conf_t *clcf;
  clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  clcf->handler = ngx_http_natspublisher_handler;
  ngx_str_t *name = data1;
  if(ngx_strcmp(name->data, "") == 0) {
    return NGX_CONF_ERROR;
  }
  natspublisher_subject.data = name->data;
  natspublisher_subject.len = ngx_strlen(natspublisher_subject.data);
  return NGX_CONF_OK;
}

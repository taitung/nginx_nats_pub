/*
 * Jarvish Inc. 
 *
 * Author: Guo-Guang Chiou
 * Date:   11/08/2016
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_nats.h>
#include <ngx_nats_comm.h>

static char *ngx_http_natspublisher(ngx_conf_t *cf, void *post, void *data);
static ngx_conf_post_handler_pt ngx_http_natspublisher_p = ngx_http_natspublisher;

/* structure holding the value of the module directive natspublisher */
typedef struct {
  ngx_str_t name;
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
     offsetof(ngx_http_natspublisher_loc_conf_t, name),
     &ngx_http_natspublisher_p
  }
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

/* manin handler */
static ngx_int_t ngx_http_natspublisher_handler(ngx_http_request_t *r) {
  ngx_int_t rc;
  ngx_buf_t *b = NULL;
  ngx_chain_t out;
  
  /* reject 'DELETE', 'PUT', and 'HEAD' */
  if(!(r->method & (NGX_HTTP_GET | NGX_HTTP_POST))) {
    return NGX_HTTP_NOT_ALLOWED;
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

  //fprintf(stderr,"%s\n",natspublisher_subject.data); 
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

nginx_nats_pub
==========

ngxin_nats_pub using nginx_nats is a nginx location module saving http post data to NATS server.

### Configuration

```nginx
  nats {
    server host:post
    reconnect 2;
    ping 30s:
    .
    .
    .
  }

  http {
    server {
      location ~/path {
        natspublisher "subject" "replyTo"
      }
      .
      .
      .
    }
  }
```

### build
--add-module=/path/to/nginx_nats_pub

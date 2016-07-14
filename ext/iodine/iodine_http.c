#include "iodine_http.h"
#include "websockets.h"

/* the Iodine::Rack HTTP server class*/
VALUE IodineHttp;
/* these three are used also by rb-rack-io.c */
VALUE R_HIJACK_IO;
VALUE R_HIJACK_CB;
VALUE IODINE_UPGRADE;
VALUE IODINE_WEBSOCKET;

static VALUE hijack_func_sym;

#define rack_declare(rack_name) static VALUE rack_name

#define rack_set(rack_name, str)                                      \
  (rack_name) = rb_enc_str_new((str), strlen((str)), BinaryEncoding); \
  rb_global_variable(&(rack_name));                                   \
  rb_obj_freeze(rack_name);

#define rack_autoset(rack_name) rack_set((rack_name), #rack_name)

static VALUE ENV_TEMPLATE;

rack_declare(HTTP_SCHEME);
rack_declare(HTTPS_SCHEME);
rack_declare(QUERY_ESTRING);
rack_declare(REQUEST_METHOD);
rack_declare(PATH_INFO);
rack_declare(QUERY_STRING);
rack_declare(QUERY_ESTRING);
rack_declare(SERVER_NAME);
rack_declare(SERVER_PORT);
rack_declare(CONTENT_LENGTH);
rack_declare(CONTENT_TYPE);
rack_declare(R_URL_SCHEME);  // rack.url_scheme
rack_declare(R_INPUT);       // rack.input
// rack_declare(R_HIJACK); // rack.hijack
// rack_declare(R_HIJACK_CB);// rack.hijack_io

/*
rack_declare(HTTP_VERSION);
rack_declare(REQUEST_URI);
rack_declare(REQUEST_METHOD);
rack_declare(CONTENT_TYPE);
rack_declare(CONTENT_LENGTH);
rack_declare(SCRIPT_NAME);
rack_declare(PATH_INFO);
rack_declare(SERVER_NAME);
rack_declare(SERVER_PORT);

rack_declare(UPGRADE_HEADER);      // upgrade support
rack_declare(UPGRADE_HEADER);      // upgrade support
rack_declare(CONNECTION_HEADER);   // upgrade support
rack_declare(CONNECTION_CLOSE);    // upgrade support
rack_declare(WEBSOCKET_STR);       // upgrade support
rack_declare(WEBSOCKET_VER);       // upgrade support
rack_declare(WEBSOCKET_SEC_VER);   // upgrade support
rack_declare(WEBSOCKET_SEC_EXT);   // upgrade support
rack_declare(WEBSOCKET_SEC_ACPT);  // upgrade support

rack_declare(QUERY_ESTRING);         // ""
rack_declare(SERVER_PORT_80);        // 80
rack_declare(SERVER_PORT_443);       // 443
rack_declare(R_VERSION);             // rack.version
rack_declare(R_VERSION_V);           // rack.version array
rack_declare(R_SCHEME);              // rack.url_scheme
rack_declare(R_SCHEME_HTTP);         // http
rack_declare(R_SCHEME_HTTPS);        // https
rack_declare(R_INPUT);               // rack.input
rack_declare(R_ERRORS);              // rack.errors
rack_declare(R_ERRORS_V);            // rack.errors array
rack_declare(R_MTHREAD);             // for Rack: rack.multithread
rack_declare(R_MTHREAD_V);           // for Rack: rack.multithread
rack_declare(R_MPROCESS);            // for Rack: rack.multiprocess
rack_declare(R_MPROCESS_V);          // for Rack: rack.multiprocess
rack_declare(R_RUN_ONCE);            // for Rack: rack.run_once
rack_declare(R_HIJACK_Q);            // for Rack: rack.hijack?
rack_declare(R_IODINE_UPGRADE);      // iodine.upgrade
rack_declare(R_IODINE_UPGRADE_DYN);  // Iodine upgrade support

rack_declare(_hijack_sym);

// for Rack
rack_declare(HTTP_VERSION);          // extending Rack
rack_declare(REQUEST_URI);           // extending Rack
rack_declare(REQUEST_METHOD);        // for Rack
rack_declare(CONTENT_TYPE);          // for Rack.
rack_declare(CONTENT_LENGTH);        // for Rack.
rack_declare(SCRIPT_NAME);           // for Rack
rack_declare(PATH_INFO);             // for Rack
rack_declare(QUERY_STRING);          // for Rack
rack_declare(QUERY_ESTRING);         // for rack (if no query)
rack_declare(SERVER_NAME);           // for Rack
rack_declare(SERVER_PORT);           // for Rack
rack_declare(SERVER_PORT_80);        // for Rack
rack_declare(SERVER_PORT_443);       // for Rack
rack_declare(R_VERSION);             // for Rack: rack.version
rack_declare(R_VERSION_V);           // for Rack: rack.version
rack_declare(R_SCHEME);              // for Rack: rack.url_scheme
rack_declare(R_SCHEME_HTTP);         // for Rack: rack.url_scheme value
rack_declare(R_SCHEME_HTTPS);        // for Rack: rack.url_scheme value
rack_declare(R_INPUT);               // for Rack: rack.input
rack_declare(R_ERRORS);              // for Rack: rack.errors
rack_declare(R_ERRORS_V);            // for Rack: rack.errors
rack_declare(R_MTHREAD);             // for Rack: rack.multithread
rack_declare(R_MTHREAD_V);           // for Rack: rack.multithread
rack_declare(R_MPROCESS);            // for Rack: rack.multiprocess
rack_declare(R_MPROCESS_V);          // for Rack: rack.multiprocess
rack_declare(R_RUN_ONCE);            // for Rack: rack.run_once
rack_declare(R_HIJACK_Q);            // for Rack: rack.hijack?
rack_declare(R_IODINE_UPGRADE);      // Iodine upgrade support
rack_declare(R_IODINE_UPGRADE_DYN);  // Iodine upgrade support
rack_declare(UPGRADE_HEADER);        // upgrade support
rack_declare(UPGRADE_HEADER);        // upgrade support
rack_declare(CONNECTION_HEADER);     // upgrade support
rack_declare(CONNECTION_CLOSE);      // upgrade support
rack_declare(WEBSOCKET_STR);         // upgrade support
rack_declare(WEBSOCKET_VER);         // upgrade support
rack_declare(WEBSOCKET_SEC_VER);     // upgrade support
rack_declare(WEBSOCKET_SEC_EXT);     // upgrade support
rack_declare(WEBSOCKET_SEC_ACPT);    // upgrade support

*/

/* *****************************************************************************
HTTP Protocol initialization
*/
/* allow quick access to the Rack app */
static VALUE rack_app_handler = 0;

#define to_upper(c) (((c) >= 'a' && (c) <= 'z') ? ((c) & ~32) : (c))

static inline VALUE copy2env(http_request_s* request) {
  VALUE env = rb_hash_dup(ENV_TEMPLATE);
  VALUE hname; /* will be used later, both as tmp and to iterate header names */
  Registry.add(env);
  /* Copy basic data */
  rb_hash_aset(
      env, REQUEST_METHOD,
      rb_enc_str_new(request->method, request->method_len, BinaryEncoding));

  rb_hash_aset(env, PATH_INFO, rb_enc_str_new(request->path, request->path_len,
                                              BinaryEncoding));
  rb_hash_aset(
      env, QUERY_STRING,
      (request->query
           ? rb_enc_str_new(request->path, request->path_len, BinaryEncoding)
           : QUERY_ESTRING));

  /* setup input IO + hijack support */
  rb_hash_aset(env, R_INPUT, (hname = RackIO.new(request, env)));

  hname = rb_obj_method(hname, hijack_func_sym);
  rb_hash_aset(env, R_HIJACK_CB, hname);

  /* handle the HOST header, including the possible host:#### format*/
  char* pos = NULL;
  for (size_t i = 0; i < request->host_len; i++) {
    if (request->host[i] == ':') {
      pos = (char*)request->host + i;
      break;
    }
  }
  if (pos == NULL) {
    rb_hash_aset(
        env, SERVER_NAME,
        rb_enc_str_new(request->host, request->host_len, BinaryEncoding));
    rb_hash_aset(env, SERVER_PORT, QUERY_ESTRING);
  } else {
    rb_hash_aset(
        env, SERVER_NAME,
        rb_enc_str_new(request->host, pos - request->host, BinaryEncoding));
    rb_hash_aset(
        env, SERVER_PORT,
        rb_enc_str_new(pos + 1, request->host_len - ((pos - request->host) - 1),
                       BinaryEncoding));
  }

  /* default schema to http, it might be updated later */
  rb_hash_aset(env, R_URL_SCHEME, HTTP_SCHEME);

  /* add all headers, exclude special cases */
  http_headers_s* header = request->headers;
  for (size_t i = 0; i < request->headers_count; i++, header++) {
    if (header->name_length == 14 &&
        strncasecmp("content-length", header->name, 14) == 0) {
      rb_hash_aset(
          env, CONTENT_LENGTH,
          rb_enc_str_new(header->value, header->value_length, BinaryEncoding));
      continue;
    } else if (header->name_length == 12 &&
               strncasecmp("content-type", header->name, 12) == 0) {
      rb_hash_aset(
          env, CONTENT_TYPE,
          rb_enc_str_new(header->value, header->value_length, BinaryEncoding));
      continue;
    } else if (header->name_length == 27 &&
               strncasecmp("x-forwarded-proto", header->name, 27) == 0) {
      if (header->value_length >= 5 &&
          !strncasecmp(header->value, "https", 5)) {
        rb_hash_aset(env, R_URL_SCHEME, HTTPS_SCHEME);
      } else if (header->value_length == 4 &&
                 *((uint32_t*)header->value) == *((uint32_t*)"http")) {
        rb_hash_aset(env, R_URL_SCHEME, HTTP_SCHEME);
      } else {
        rb_hash_aset(env, R_URL_SCHEME,
                     rb_enc_str_new(header->value, header->value_length,
                                    BinaryEncoding));
      }
    } else if (header->name_length == 9 &&
               strncasecmp("forwarded", header->name, 9) == 0) {
      pos = (char*)header->value;
      while (*pos) {
        if (((*(pos++) | 32) == 'p') && ((*(pos++) | 32) == 'r') &&
            ((*(pos++) | 32) == 'o') && ((*(pos++) | 32) == 't') &&
            ((*(pos++) | 32) == 'o') && ((*(pos++) | 32) == '=')) {
          if ((pos[0] | 32) == 'h' && (pos[1] | 32) == 't' &&
              (pos[2] | 32) == 't' && (pos[3] | 32) == 'p') {
            if ((pos[4] | 32) == 's') {
              rb_hash_aset(env, R_URL_SCHEME, HTTPS_SCHEME);
            } else {
              rb_hash_aset(env, R_URL_SCHEME, HTTP_SCHEME);
            }
          } else {
            char* tmp = pos;
            while (*tmp && *tmp != ';')
              tmp++;
            rb_hash_aset(env, R_URL_SCHEME, rb_str_new(pos, tmp - pos));
          }
          break;
        }
      }
    }

    hname = rb_str_buf_new(6 + header->name_length);
    memcpy(RSTRING_PTR(hname), "HTTP_", 5);
    pos = RSTRING_PTR(hname) + 5;
    for (size_t j = 0; j < header->name_length; j++) {
      *(pos++) = to_upper(header->name[j]);
    }
    *pos = 0;
    rb_str_set_len(hname, 5 + header->name_length);
    rb_hash_aset(env, hname, rb_enc_str_new(header->value, header->value_length,
                                            BinaryEncoding));
  }
  return env;
}

static inline void ruby2c_response_headers(http_response_s* response,
                                           VALUE rbresponse) {}

static inline int ruby2c_review_upgrade(http_response_s* response,
                                        VALUE rbresponse) {
  /* Need to deal with:
    rack_set(IODINE_UPGRADE, "iodine.upgrade");
    rack_set(IODINE_WEBSOCKET, "iodine.websocket");
  */
  return 0;
}
static inline void ruby2c_response_send(http_response_s* response,
                                        VALUE rbresponse) {
  http_response_write_body(response, "Fix ME!", 7);
}

static void* on_rack_request_in_GVL(http_request_s* request) {
  http_response_s response = http_response_init(request);
  // create /register env variable
  VALUE env = copy2env(request);
  // pass env variable to handler
  VALUE rbresponse = RubyCaller.call2(rack_app_handler, call_proc_id, 1, &env);
  Registry.add(rbresponse);
  // handle header copy from ruby land to C land.
  ruby2c_response_headers(&response, rbresponse);
  // review for upgrade.
  if (ruby2c_review_upgrade(&response, rbresponse) == 0) {
    // send the request body.
    ruby2c_response_send(&response, rbresponse);
  }
  Registry.remove(rbresponse);
  Registry.remove(env);
  http_response_destroy(&response);
  return NULL;
}

static void on_rack_request(http_request_s* request) {
  RubyCaller.call_c((void* (*)(void*))on_rack_request_in_GVL, request);
}

/* *****************************************************************************
Initializing basic ENV template
*/

#define add_str_to_env(env, key, value)                                 \
  {                                                                     \
    VALUE k = rb_enc_str_new((key), strlen((key)), BinaryEncoding);     \
    rb_obj_freeze(k);                                                   \
    VALUE v = rb_enc_str_new((value), strlen((value)), BinaryEncoding); \
    rb_obj_freeze(v);                                                   \
    rb_hash_aset(env, k, v);                                            \
  }
#define add_value_to_env(env, key, value)                           \
  {                                                                 \
    VALUE k = rb_enc_str_new((key), strlen((key)), BinaryEncoding); \
    rb_obj_freeze(k);                                               \
    rb_hash_aset(env, k, value);                                    \
  }

static void init_env_template(void) {
  VALUE tmp;
  ENV_TEMPLATE = rb_hash_new();
  rb_global_variable(&ENV_TEMPLATE);

  // add the rack.version
  tmp = rb_ary_new();  // rb_ary_new is Ruby 2.0 compatible
  rb_ary_push(tmp, INT2FIX(1));
  rb_ary_push(tmp, INT2FIX(3));
  // rb_ary_push(tmp, rb_enc_str_new("1", 1, BinaryEncoding));
  // rb_ary_push(tmp, rb_enc_str_new("3", 1, BinaryEncoding));
  add_value_to_env(ENV_TEMPLATE, "rack.version", tmp);
  add_value_to_env(ENV_TEMPLATE, "rack.errors", rb_stderr);
  add_value_to_env(ENV_TEMPLATE, "rack.multithread", Qtrue);
  add_value_to_env(ENV_TEMPLATE, "rack.multiprocess", Qtrue);
  add_value_to_env(ENV_TEMPLATE, "rack.run_once", Qfalse);
  add_value_to_env(ENV_TEMPLATE, "rack.hijack?", Qtrue);
  add_str_to_env(ENV_TEMPLATE, "SCRIPT_NAME", "");
}
#undef add_str_to_env
#undef add_value_to_env

/* *****************************************************************************
Rack object API
*/

int iodine_http_review(void) {
  rack_app_handler = rb_iv_get(IodineHttp, "@app");
  if (rack_app_handler != Qnil &&
      rb_respond_to(rack_app_handler, call_proc_id)) {
    VALUE rbport = rb_iv_get(IodineHttp, "@port");
    VALUE rbaddress = rb_iv_get(IodineHttp, "@address");
    VALUE rbmaxbody = rb_iv_get(IodineHttp, "@max_body_size");
    VALUE rbwww = rb_iv_get(IodineHttp, "@public");
    VALUE rblog = rb_iv_get(IodineHttp, "@log");
    VALUE rbtout = rb_iv_get(IodineHttp, "@timeout");
    const char* port = "3000";
    const char* address = NULL;
    const char* public_folder = NULL;
    size_t max_body_size;
    uint8_t log_static;
    // review port
    if (TYPE(rbport) != T_FIXNUM && TYPE(rbport) != T_STRING &&
        TYPE(rbport) != Qnil)
      rb_raise(rb_eTypeError,
               "The port variable must be either a Fixnum or a String.");
    if (TYPE(rbport) == T_FIXNUM)
      rbport = rb_funcall2(rbport, rb_intern("to_s"), 0, NULL);
    if (TYPE(rbport) == T_STRING)
      port = StringValueCStr(rbport);
    // review address
    if (TYPE(rbaddress) != T_STRING && rbaddress != Qnil)
      rb_raise(rb_eTypeError,
               "The address variable must be either a String or `nil`.");
    if (TYPE(address) == T_STRING)
      address = StringValueCStr(rbaddress);
    // review public folder
    if (TYPE(rbwww) != T_STRING && rbwww != Qnil)
      rb_raise(rb_eTypeError,
               "The public folder variable `public` must be either a String or "
               "`nil`.");
    if (TYPE(rbwww) == T_STRING)
      public_folder = StringValueCStr(rbwww);
    // review timeout
    uint8_t timeout = (TYPE(rbtout) == T_FIXNUM) ? FIX2ULONG(rbtout) : 0;
    if (FIX2ULONG(rbtout) > 255) {
      fprintf(stderr,
              "Iodine Warning: Iodine::Rack timeout value is over 255 and is "
              "silently ignored.\n");
      timeout = 0;
    }
    // review max body size
    max_body_size = (TYPE(rbmaxbody) == T_FIXNUM) ? FIX2ULONG(rbmaxbody) : 0;
    // review logging
    log_static = (rblog == Qnil || rblog == Qfalse) ? 0 : 1;
    // initialize the Rack env template
    init_env_template();
    // listen
    return http1_listen(port, address, .on_request = on_rack_request,
                        .log_static = log_static,
                        .max_body_size = max_body_size,
                        .public_folder = public_folder, .timeout = timeout);
  }
  return -1;
}

/* *****************************************************************************
Initializing the library
*/

void Init_iodine_http(void) {
  rack_autoset(REQUEST_METHOD);
  rack_autoset(PATH_INFO);
  rack_autoset(QUERY_STRING);
  rack_autoset(SERVER_NAME);
  rack_autoset(SERVER_PORT);
  rack_autoset(CONTENT_LENGTH);
  rack_autoset(CONTENT_TYPE);
  rack_set(HTTP_SCHEME, "http");
  rack_set(HTTPS_SCHEME, "https");
  rack_set(QUERY_ESTRING, "");
  rack_set(R_URL_SCHEME, "rack.url_scheme");
  rack_set(R_INPUT, "rack.input");

  rack_set(R_HIJACK_IO, "rack.hijack_io");
  rack_set(R_HIJACK_CB, "rack.hijack");

  rack_set(IODINE_UPGRADE, "iodine.upgrade");
  rack_set(IODINE_WEBSOCKET, "iodine.websocket");

  rack_set(QUERY_ESTRING, "");
  rack_set(QUERY_ESTRING, "");

  hijack_func_sym = ID2SYM(rb_intern("_hijack"));

  IodineHttp = rb_define_module_under(Iodine, "Rack");
  RackIO.init();
}

// REQUEST_METHOD
// PATH_INFO
// QUERY_STRING
// SERVER_NAME
// SERVER_PORT
// CONTENT_LENGTH
// rack.url_scheme rack.input rack.hijack rack.hijack_io HTTP_ Variables

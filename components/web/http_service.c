#include "http_service.h"
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_http_server.h"

#ifdef EXAMPLE
/* An HTTP GET handler */
esp_err_t hello_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ets_printf("Found header => Host: %s", buf);
        }
        free(buf);
    }
  
    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ets_printf("Found URL query => %s", buf);
            char param[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK) {
                ets_printf("Found URL query parameter => query1=%s", param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK) {
                ets_printf("Found URL query parameter => query3=%s", param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK) {
                ets_printf("Found URL query parameter => query2=%s", param);
            }
        }
        free(buf);
    }

    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ets_printf("Request headers lost");
    }
    return ESP_OK;
}

httpd_uri_t hello = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "Hello World!"
};


/* An HTTP POST handler */
esp_err_t echo_post_handler(httpd_req_t *req)
{ ets_printf(".........POST......\n");
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        ets_printf("=========== RECEIVED DATA ==========");
        ets_printf("%.*s", ret, buf);
        ets_printf("====================================");
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

httpd_uri_t echo = {
    .uri       = "/echo",
    .method    = HTTP_POST,
    .handler   = echo_post_handler,
    .user_ctx  = NULL
};

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */
esp_err_t ctrl_put_handler(httpd_req_t *req)
{ets_printf(".........PUT......\n");
    char buf;
    int ret;

    if ((ret = httpd_req_recv(req, &buf, 1)) <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    if (buf == '0') {
        /* Handler can be unregistered using the uri string */
        ets_printf("Unregistering /hello and /echo URIs");
        httpd_unregister_uri(req->handle, "/hello");
        httpd_unregister_uri(req->handle, "/echo");
    }
    else {
        ets_printf("Registering /hello and /echo URIs");
        httpd_register_uri_handler(req->handle, &hello);
        httpd_register_uri_handler(req->handle, &echo);
    }

    /* Respond with empty body */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

httpd_uri_t ctrl = {
    .uri       = "/ctrl",
    .method    = HTTP_PUT,
    .handler   = ctrl_put_handler,
    .user_ctx  = NULL
};

httpd_handle_t start_http_service()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ets_printf("Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ets_printf("Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &echo);
        httpd_register_uri_handler(server, &ctrl);
        return server;
    }

    ets_printf("Error starting server!");
    return NULL;
}
#endif

const char * html_text = "\
<html>\
<head><style> table, th, td { border: 1px solid black; border-collapse: collapse; } td, th { padding: 10px; }</style></head>\
<body>\
   <div id='container'></div>\
   <script>\
      let jsonData = [\
            {\
               name: 'Saurabh',\
               age: 20,\
               city: 'Prayagraj'\
            },\
            {\
               name: 'Vipin',\
               age: 23,\
               city: 'Lucknow',\
            },\
            {\
               name: 'Saksham',\
               age: 21,\
               city: 'Noida'\
            }\
         ];\
      function convert2table() {\
         let container = document.getElementById('container');\
         let table = document.createElement('table');\
         let cols = Object.keys(jsonData[0]);\
         let thead = document.createElement('thead');\
         let tr = document.createElement('tr');\
         cols.forEach((item) => {\
            let th = document.createElement('th');\
            th.innerText = item;\
            tr.appendChild(th);});\
         thead.appendChild(tr);\
         table.append(tr);\
         jsonData.forEach((item) => {\
            let tr = document.createElement('tr');\
            let vals = Object.values(item);\
            vals.forEach((elem) => {\
               let td = document.createElement('td');\
               td.innerText = elem;\
               tr.appendChild(td);});\
            table.appendChild(tr);});\
         container.appendChild(table);};\
     convert2table();\
   </script>\
</body>\
</html>";

//https://www.w3schools.com/bootstrap/tryit.asp?filename=trybs_table_condensed&stacked=h

esp_err_t data_get_handler(httpd_req_t *req)
{
    httpd_resp_send(req, html_text, strlen(html_text));
    return ESP_OK;
}

void stop_http_service(httpd_handle_t server)
{
    ets_printf("Http stoping ...");
    httpd_stop(server);
}

httpd_uri_t data = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = data_get_handler
};
httpd_handle_t start_http_service()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ets_printf("Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &data);
        return server;
    }

    ets_printf("Error starting server!");
    return NULL;
}
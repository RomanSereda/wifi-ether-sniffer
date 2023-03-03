idf_component_register(SRCS "scan.c" "parser.c" "container.c"
                       INCLUDE_DIRS .
                       PRIV_INCLUDE_DIRS .
                       PRIV_REQUIRES nvs_flash)



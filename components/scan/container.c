#include "container.h"
#include "freertos/FreeRTOS.h"
#include "freertos/private/list.h"

void init_container()
{
    List_t* list = malloc(sizeof(List_t));
    vListInitialise(list);


}

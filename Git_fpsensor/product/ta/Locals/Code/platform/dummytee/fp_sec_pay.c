#include "fp_log.h"
#include <stdio.h>
#include "fp_internal_api.h"

uint32_t last_finger_id = 0;

int32_t fp_sec_set_pass_id(uint32_t tpl_id, uint64_t timestamp)
{
    last_finger_id = tpl_id;
    return 0;
}
uint32_t fp_sec_get_pass_id(void)
{
    return last_finger_id;
}

int fp_sec_pay_init(void)
{
    return 0;
}


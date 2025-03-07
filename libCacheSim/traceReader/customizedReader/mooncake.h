#pragma once

#include "../../include/libCacheSim/reader.h"
#include "./cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

int mooncakeReader_setup(reader_t *reader);

int mooncakeReader_read_one_req(reader_t *reader, request_t *req);

#ifdef __cplusplus
}
#endif

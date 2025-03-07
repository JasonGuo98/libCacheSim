#include "mooncake.h"

int mooncakeReader_setup(reader_t *reader) {
  reader->trace_type = MOONCAKE_TRACE;
  reader->trace_format = TXT_TRACE_FORMAT;

  reader->obj_id_is_num = true;
  reader->mooncake_req = NULL;
  return 0;
}

int mooncakeReader_read_one_req(reader_t *reader, request_t *req) {
//   printf("mooncakeReader_read_one_req\n");
  // 1. 判断当前是否有未完成的json请求，if false，读取下一个json请求
  if (reader->mooncake_req == NULL) {
    // printf("read one line\n");
    // printf("file: %p\n", reader->file);
    // 读取一行
    char *line = NULL;
    size_t len = 0;
    int read_size = getline(&line, &len, reader->file);
    // printf("line: %s\n", line);
    // 2. 如果没有请求，则结束
    if (read_size == -1) {
      req->valid = FALSE;
      return 1;
    }

    // 1.1 解析并初始化请求
    cJSON *root = cJSON_Parse(line);
    if (root == NULL) {
      req->valid = FALSE;
      return 1;
    }

    cJSON_AddNumberToObject(root, "next_token_idx", 0);

    // 1.2 保存json请求
    reader->mooncake_req = root;
  }

//   printf("%d\n", __LINE__);

  // 3. 对于一个打开的json请求，读取下一个token请求
  cJSON *root = (cJSON *)reader->mooncake_req;
  int next_token_idx = cJSON_GetObjectItem(root, "next_token_idx")->valueint;

  // 获取 JSON 数组
  cJSON *token_hash_ids = cJSON_GetObjectItem(root, "hash_ids");
  if (token_hash_ids == NULL) {
    fprintf(stderr, "token_hash_ids array not found.\n");
    cJSON_Delete(root);
    req->valid = FALSE;
    return 1;
  }

  // 获取数组长度
  int token_hash_ids_count = cJSON_GetArraySize(token_hash_ids);
  // printf("Number of token_hash_ids: %d\n", token_hash_ids_count);

  if (next_token_idx >= token_hash_ids_count) {
    cJSON_Delete(root);
    req->valid = FALSE;
    return 1;
  }

  int current_token = cJSON_GetArrayItem(token_hash_ids, next_token_idx)->valueint;

  next_token_idx += 1;
  cJSON_GetObjectItem(root, "next_token_idx")->valueint = next_token_idx;
  cJSON_GetObjectItem(root, "next_token_idx")->valuedouble = (double)next_token_idx;  // 确保 double 值也被更新


  req->clock_time = cJSON_GetObjectItem(root, "timestamp")->valueint;
  req->obj_id = current_token;
  req->obj_size = 1;
  req->next_access_vtime = MAX_REUSE_DISTANCE;

  if (next_token_idx == token_hash_ids_count) {
    cJSON_Delete(root);
    reader->mooncake_req = NULL;
  }

  return 0;
}
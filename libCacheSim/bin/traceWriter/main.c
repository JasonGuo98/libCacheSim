

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../include/libCacheSim/reader.h"

void fix_msr_oracleGeneral_trace(char *trace_path);
void fix_msr_oracleGeneralOpNS_trace(char *trace_path);


int main(int argc, char *argv[]) {
  fix_msr_oracleGeneral_trace(argv[1]);
  // fix_msr_oracleGeneralOpNS_trace(argv[1]);

  return 0;
}


void fix_msr_oracleGeneral_trace(char *trace_path) {
  reader_t *reader = setup_reader(trace_path, ORACLE_GENERAL_TRACE, OBJ_ID_NUM, NULL);
  request_t *req = new_request();

  char ofilepath[256];
  sprintf(ofilepath, "%s.new", trace_path);
  FILE *ofile = fopen(ofilepath, "wb");
  read_one_req(reader, req);
  uint64_t start_ts = req->real_time;

  while (req->valid) {
    req->real_time = (req->real_time - start_ts) / 10;
    if (req->next_access_vtime == INT64_MAX) {
      req->next_access_vtime = -1;
    }

    fwrite(&req->real_time, 4, 1, ofile);
    fwrite(&req->obj_id, 8, 1, ofile);
    fwrite(&req->obj_size, 4, 1, ofile);
    fwrite(&req->next_access_vtime, 8, 1, ofile);

    read_one_req(reader, req);
  }
  printf("%s trace time %lu hour\n", trace_path, req->real_time / 3600);

  fclose(ofile);
}

void fix_msr_oracleGeneralOpNS_trace(char *trace_path) {
  reader_t *reader = setup_reader(trace_path, ORACLE_GENERALOPNS_TRACE, OBJ_ID_NUM, NULL);
  request_t *req = new_request();

  char ofilepath[256];
  sprintf(ofilepath, "%s.new", trace_path);
  FILE *ofile = fopen(ofilepath, "wb");
  char ofilepath2[256];
  sprintf(ofilepath2, "%s.new.txt", trace_path);
  FILE *ofile2 = fopen(ofilepath2, "w");
  read_one_req(reader, req);
  uint64_t start_ts = req->real_time;

  while (req->valid) {
    // req->real_time = (req->real_time - start_ts) / 10;
    // if (req->next_access_vtime == INT64_MAX) {
    //   req->next_access_vtime = -1;
    // }

    uint8_t op = req->op;
    uint16_t ns = req->ns;

    fwrite(&req->real_time, 4, 1, ofile);
    fwrite(&req->obj_id, 8, 1, ofile);
    fwrite(&req->obj_size, 4, 1, ofile);
    fwrite(&op, 1, 1, ofile);
    fwrite(&ns, 2, 1, ofile);
    fwrite(&req->next_access_vtime, 8, 1, ofile);

    fprintf(ofile2, "%lu %lu %lu %lu %u %u\n", req->real_time, req->obj_id, req->obj_size, req->next_access_vtime, op, ns);

    read_one_req(reader, req);
  }
  printf("%s trace time %lu hour\n", trace_path, req->real_time / 3600);

  fclose(ofile);
}
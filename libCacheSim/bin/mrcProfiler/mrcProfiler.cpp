#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include "./mrcProfiler.h"
#include "../include/libCacheSim/const.h"


mrcProfiler::MRCProfilerBase * mrcProfiler::create_mrc_profiler(mrc_profiler_e type, reader_t *reader, std::string output_path, const profiler_params_t & params) {
    switch (type) {
    case mrc_profiler_e::SHARDS_PROFILER:
      return new MRCProfilerSHARDS(reader, output_path, params);
    case mrc_profiler_e::MINISIM_PROFILER:
      return new MRCProfilerMINISIM(reader, output_path, params);
    default:
      printf("unknown profiler type %d\n", type);
      exit(1);
    }
}
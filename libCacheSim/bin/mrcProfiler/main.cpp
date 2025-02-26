//
// Created by Juncheng Yang on 5/9/21.
//

#include "../../traceAnalyzer/analyzer.h"
#include "../cli_reader_utils.h"
#include "internal.h"

using namespace traceAnalyzer;


void print_args(struct arguments *args) {

  printf("args: \n");
  for (int i = 0; i < N_ARGS; i++) {
    printf("%s\n", args->args[i]);
  }
  printf("trace_path: %s\n", args->trace_path);
  printf("trace_type: %d\n", args->trace_type);
  printf("trace_type_params: %s\n", args->trace_type_params);
  printf("ofilepath: %s\n", args->ofilepath);
  printf("n_req: %ld\n", args->n_req);
  printf("verbose: %d\n", args->verbose);
  printf("cache_algorithm_str: %s\n", args->cache_algorithm_str);
  printf("mrc_size_str: %s\n", args->mrc_size_str);
  printf("profiler_str: %s\n", args->profiler_str);
  printf("profiler_params_str: %s\n", args->profiler_params_str);

  for(int i = 0; i < args->profiler_params.profile_size.size(); i++){
    printf("profile_size: %ld\n", args->profiler_params.profile_size[i]);
  }
  printf("====\n");
  for(int i = 0; i < args->profiler_params.profile_wss_ratio.size(); i++){
    printf("profile_wss_ratio: %f\n", args->profiler_params.profile_wss_ratio[i]);
  }

  args->profiler_params.shards_params.print();
  args->profiler_params.minisim_params.print();
}

int main(int argc, char *argv[]) {
  struct arguments args;
  parse_cmd(argc, argv, &args);

  print_args(&args);

  mrcProfiler::MRCProfilerBase * profiler = create_mrc_profiler(args.profiler_type, args.reader, args.ofilepath, args.profiler_params);

  profiler->run();

  profiler->print();

  delete profiler;

  close_reader(args.reader);

  return 0;
}
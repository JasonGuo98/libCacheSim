

#include <argp.h>
#include <stdbool.h>
#include <string.h>
#include <vector>
#include <string>

#include "../../include/libCacheSim/const.h"
#include "../../utils/include/mystr.h"
#include "../../utils/include/mysys.h"
#include "../cli_reader_utils.h"
#include "internal.h"
#include "./mrcProfiler.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *argp_program_version = "mrcProfiler 0.0.1";
const char *argp_program_bug_address =
    "https://groups.google.com/g/libcachesim/";

enum argp_option_short {
  OPTION_TRACE_TYPE_PARAMS = 't',
  OPTION_OUTPUT_PATH = 'o',
  OPTION_NUM_REQ = 'n',
  OPTION_VERBOSE = 'v',


  /* profiler params */
  OPTION_CACHE_ALGORITHM = 0x100,
  OPTION_MRC_SIZE = 0x101,
  OPTION_PROFILER = 0x102,
  OPTION_PROFILER_PARAMS = 0x103,
};

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option options[] = {
    {NULL, 0, NULL, 0, "trace reader related parameters:", 0},
    {"trace-type-params", OPTION_TRACE_TYPE_PARAMS,
     "time-col=1,obj-id-col=2,obj-size-col=3,delimiter=,", 0,
     "Parameters used for csv trace", 1},
    {"num-req", OPTION_NUM_REQ, "-1", 0,
     "Num of requests to process, default -1 means all requests in the trace",
     1},

    {NULL, 0, NULL, 0, "mrc profiler task options:", 0},
    {"algo", OPTION_CACHE_ALGORITHM, "lru", OPTION_ARG_OPTIONAL,
     "Which algorithm to profile", 2},
    {"size", OPTION_MRC_SIZE, "0.01,1,100", OPTION_ARG_OPTIONAL,
     "MRC profile size", 2},
    {"profiler", OPTION_PROFILER, "SHARDS", OPTION_ARG_OPTIONAL,
     "which profiler to use", 
     2},
    {"profiler-params", OPTION_PROFILER_PARAMS, "",
     OPTION_ARG_OPTIONAL,
     "profiler parameters", 
     2},

    {NULL, 0, NULL, 0, "common parameters:", 0},

    {"output", OPTION_OUTPUT_PATH, "", OPTION_ARG_OPTIONAL, "Output path", 3},
    {"verbose", OPTION_VERBOSE, NULL, OPTION_ARG_OPTIONAL,
     "Produce verbose output", 3},
    {0}};

/*
   PARSER. Field 2 in ARGP.
   Order of parameters: KEY, ARG, STATE.
*/
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments =
      reinterpret_cast<struct arguments *>(state->input);

  switch (key) {
    case OPTION_TRACE_TYPE_PARAMS:
      arguments->trace_type_params = arg;
      break;
    case OPTION_OUTPUT_PATH:
      strncpy(arguments->ofilepath, arg, OFILEPATH_LEN);
      break;
    case OPTION_NUM_REQ:
      arguments->n_req = atoll(arg);
      break;
    case OPTION_CACHE_ALGORITHM:
      arguments->cache_algorithm_str = arg;
      break;
    case OPTION_MRC_SIZE:
      arguments->mrc_size_str = arg;
      break;
    case OPTION_PROFILER:
      arguments->profiler_str = arg;
      break;
    case OPTION_PROFILER_PARAMS:
      arguments->profiler_params_str = arg;
      break;

    case OPTION_VERBOSE:
      arguments->verbose = is_true(arg) ? true : false;
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= N_ARGS) {
        printf("found too many arguments, current %s\n", arg);
        argp_usage(state);
        exit(1);
      }
      arguments->args[state->arg_num] = arg;
      break;
    case ARGP_KEY_END:
      if (state->arg_num < N_ARGS) {
        printf("not enough arguments found\n");
        argp_usage(state);
        exit(1);
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

/*
   ARGS_DOC. Field 3 in ARGP.
   A description of the non-option command-line arguments
     that we accept.
*/
static char args_doc[] = "trace_path trace_type --algo=[lru] --profiler=[SHARDS] --profiler-params=[FIX_RATE,0.01,random_seed|FIX_SIZE,8192,random_seed] --size=[0.01,1,100|1MiB,100MiB,100|0.001,0.002,0.004,0.008,0.016|1MiB,10MiB,10MiB,1GiB]";

/* Program documentation. */
static char doc[] =
    "example: ./bin/mrcProfiler ../data/cloudPhysicsIO.vscsi vscsi --algo=lru --profiler=SHARDS --profiler-params=FIX_RATE,0.01,42 --size=0.01,1,100\n\n"
    "trace_type: txt/csv/twr/vscsi/oracleGeneralBin and more\n"
    "if using csv trace, considering specifying -t obj-id-is-num=true\n"
    "profiler: "
    "SHARDS only supports lru, MINISIM supports other eviction algorithms\n"
    "profiler-params: "
    "only SHARDS support fix_size sampling\n"
    "size: "
    "profiling working set size related mrc or fixed size mrc\n\n";


/**
 * @brief split string by char
 * 
 * @param input
 * @param c
 */
std::vector<std::string> split_by_char(const char * input, const char &c){
    std::vector<std::string> result;
    if (!input) return result; // if input is null, return empty vector

    const char* start = input;
    const char* current = input;

    while (*current) {
        if (*current == c) {
            result.emplace_back(start, current - start);
            start = current + 1; 
        }
        ++current;
    }

    if(current != start)
        result.emplace_back(start, current - start);
    return result;
}

/**
 *
 * @brief convert cache size string to byte, e.g., 100MB -> 100 * 1024 * 1024
 * the cache size can be an integer or a string with suffix KB/MB/GB
 *
 * @param cache_size_str
 * @return unsigned long
 */
static unsigned long conv_size_str_to_byte_ul(char *cache_size_str) {
  if (strcasecmp(cache_size_str, "auto") == 0) {
    return 0;
  }

  if (strcasestr(cache_size_str, "kb") != NULL ||
      cache_size_str[strlen(cache_size_str) - 1] == 'k' ||
      cache_size_str[strlen(cache_size_str) - 1] == 'K') {
    return strtoul(cache_size_str, NULL, 10) * KiB;
  } else if (strcasestr(cache_size_str, "mb") != NULL ||
             cache_size_str[strlen(cache_size_str) - 1] == 'm' ||
             cache_size_str[strlen(cache_size_str) - 1] == 'M') {
    return strtoul(cache_size_str, NULL, 10) * MiB;
  } else if (strcasestr(cache_size_str, "gb") != NULL ||
             cache_size_str[strlen(cache_size_str) - 1] == 'g' ||
             cache_size_str[strlen(cache_size_str) - 1] == 'G') {
    return strtoul(cache_size_str, NULL, 10) * GiB;
  } else if (strcasestr(cache_size_str, "tb") != NULL ||
             cache_size_str[strlen(cache_size_str) - 1] == 't' ||
             cache_size_str[strlen(cache_size_str) - 1] == 'T') {
    return strtoul(cache_size_str, NULL, 10) * TiB;
  }

  long cache_size = strtol(cache_size_str, NULL, 10);
  cache_size = cache_size == -1 ? 0 : cache_size;

  return (unsigned long)cache_size;
}


/**
 * @brief parse the mrc size string
 * 
 * @param mrc_size_str
 * @param params
 */
static void parse_mrc_size_params(const char * mrc_size_str, mrcProfiler::profiler_params_t &params){
    std::vector<std::string> mrc_size_vec = split_by_char(mrc_size_str, ',');

    // parse mrc size
    if(mrc_size_vec.size() == 0){
        printf("mrc size must be set\n");
        exit(1);
    }

    bool wss_based_mrc = false;
    bool interval_based_mrc = false;
    // 1. check whether the first size is a double
    if(mrc_size_vec[0].find_first_not_of("0123456789.") == std::string::npos){
        wss_based_mrc = true;
    }

    // 2. check whether the last size is an integer
    if(mrc_size_vec.size() == 3 && mrc_size_vec[mrc_size_vec.size() - 1].find_first_not_of("0123456789") == std::string::npos){
        // if the last size is an integer and greater than 1, then it is interval based mrc
        int64_t mrc_points = atoi(mrc_size_vec[mrc_size_vec.size() - 1].c_str());
        if(mrc_points > 1){
            interval_based_mrc = true;
        }
    }
    
    if(interval_based_mrc){
        int64_t mrc_points = atoi(mrc_size_vec[mrc_size_vec.size() - 1].c_str());
        mrc_size_vec.erase(mrc_size_vec.end() - 1);
        if(mrc_size_vec.size() != 2){
            printf("mrc size setting wrong, current %s\n", mrc_size_str);
            exit(1);
        }
        if(wss_based_mrc){
            double start_ratio = atof(mrc_size_vec[0].c_str());
            double end_ratio = atof(mrc_size_vec[1].c_str());
            if(start_ratio < 0 || end_ratio > 1 || start_ratio >= end_ratio){
                printf("mrc start size or end size wrong, current %s\n", mrc_size_str);
                exit(1);
            }
            double interval = (end_ratio - start_ratio) / (mrc_points - 1);
            for(int i = 0; i < mrc_points - 1; i++){
                double ratio = start_ratio + interval * i;
                params.profile_wss_ratio.push_back(ratio);
            }
            params.profile_wss_ratio.push_back(end_ratio);
        }
        else{
            uint64_t start_size = conv_size_str_to_byte_ul((char *)mrc_size_vec[0].c_str());
            uint64_t end_size = conv_size_str_to_byte_ul((char *)mrc_size_vec[1].c_str());
            if(start_size < 0 || start_size >= end_size){
                printf("mrc start size or end size wrong, current %s\n", mrc_size_str);
                exit(1);
            }
            uint64_t interval = (end_size - start_size) / (mrc_points - 1);
            for(int i = 0; i < mrc_points - 1; i++){
                uint64_t test_size = start_size + interval * i;
                params.profile_size.push_back(test_size);
            }
            params.profile_size.push_back(end_size);
        }
    }
    else{
        // not interval based mrc
        if(wss_based_mrc){
            for(int i = 0; i < mrc_size_vec.size(); i++){
                // must be double
                double ratio = atof(mrc_size_vec[i].c_str());
                if(ratio < 0 || ratio > 1){
                    printf("mrc wss ratio must be in [0, 1], current %s\n", mrc_size_str);
                    exit(1);
                }
                params.profile_wss_ratio.push_back(ratio);
            }

            // cache size must be increasing
            for(int i = 0; i < params.profile_wss_ratio.size() - 1; i++){
                if(params.profile_wss_ratio[i] >= params.profile_wss_ratio[i + 1]){
                    printf("mrc wss ratio must be increasing, current %s\n", mrc_size_str);
                    exit(1);
                }
            }
        }
        else{
            for(int i = 0; i < mrc_size_vec.size(); i++){
                uint64_t size = conv_size_str_to_byte_ul((char *)mrc_size_vec[1].c_str());
                params.profile_size.push_back(size);
            }

            // cache size must be increasing
            for(int i = 0; i < params.profile_size.size() - 1; i++){
                if(params.profile_size[i] >= params.profile_size[i + 1]){
                    printf("mrc size must be increasing, current %s\n", mrc_size_str);
                    exit(1);
                }
            }
        }
    }
}


/**
 * @brief initialize the arguments
 * 
 * @param cache_algorithm_str
 * @param profiler_str
 * @param params_str
 * @param mrc_size_str
 * @param profiler_type
 * @param params
 */
void mrc_profiler_init(const char * cache_algorithm_str, const char * profiler_str, const char * params_str, const char * mrc_size_str, mrcProfiler::mrc_profiler_e &profiler_type, mrcProfiler::profiler_params_t &params){
    if(strcmp(profiler_str, "SHARDS") == 0 || strcmp(profiler_str, "shards") == 0){
        profiler_type = mrcProfiler::SHARDS_PROFILER;
        if(strcmp(cache_algorithm_str, "LRU") != 0 && strcmp(cache_algorithm_str, "lru") != 0){
            printf("cache algorithm must be LRU for SHARDS\n");
            exit(1);
        }

        params.cache_algorithm_str = (char *)cache_algorithm_str;

        // init shards params
        params.shards_params.parse_params(params_str);
    }else if(strcmp(profiler_str, "MINISIM") == 0 || strcmp(profiler_str, "minisim") == 0){
        profiler_type = mrcProfiler::MINISIM_PROFILER;

        // init minisim params
        params.minisim_params.parse_params(params_str);
    }
    else{
        printf("profiler type %s not supported\n", profiler_str);
        exit(1);
    }

    parse_mrc_size_params(mrc_size_str, params);
}


/**
 * @brief initialize the arguments
 *
 * @param args
 */
static void init_arg(struct arguments *args) {
  memset(args, 0, sizeof(struct arguments));

  args->trace_path = NULL;
  args->trace_type_params = NULL;
  args->verbose = true;
  memset(args->ofilepath, 0, OFILEPATH_LEN);
  args->n_req = -1;
  args->verbose = false;

  /* profiler params */
  args->cache_algorithm_str = "LRU";
  args->mrc_size_str = "0.01,1,100";
  args->profiler_str = "SHARDS";
  args->profiler_params_str = "FIX_RATE,0.01,42";

  args->reader = NULL;
}

/**
 * @brief parse the command line arguments
 *
 * @param argc
 * @param argv
 */
void parse_cmd(int argc, char *argv[], struct arguments *args) {
  init_arg(args);

  static struct argp argp = {options, parse_opt, args_doc, doc};

  argp_parse(&argp, argc, argv, 0, 0, args);

  args->trace_path = args->args[0];
  const char *trace_type_str = args->args[1];

  if (args->ofilepath[0] == '\0') {
    char *trace_filename = rindex(args->trace_path, '/');
    snprintf(args->ofilepath, OFILEPATH_LEN, "%s",
             trace_filename == NULL ? args->trace_path : trace_filename + 1);
  }

  args->reader = create_reader(trace_type_str, args->trace_path,
                               args->trace_type_params, args->n_req, false, 1);

  mrc_profiler_init(args->cache_algorithm_str, args->profiler_str, args->profiler_params_str, args->mrc_size_str, args->profiler_type, args->profiler_params);

  if(args->profiler_params.profile_wss_ratio.size() != 0){
    // need calcuate the working set size
    long wss = 0;
    int64_t wss_obj = 0, wss_byte = 0;
    cal_working_set_size(args->reader, &wss_obj, &wss_byte);
    // TODO: support ignore_obj_size
    wss = wss_byte;

    for(int i = 0; i < args->profiler_params.profile_wss_ratio.size(); i++){
      args->profiler_params.profile_size.push_back(wss * args->profiler_params.profile_wss_ratio[i]);
    }
  }
}

void free_arg(struct arguments *args) { close_reader(args->reader); }

#ifdef __cplusplus
}
#endif
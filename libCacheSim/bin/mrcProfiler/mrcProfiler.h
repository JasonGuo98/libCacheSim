#pragma once
#include <inttypes.h>

#include <vector>

#include "../../dataStructure/hash/hash.h"
#include "../../dataStructure/robin_hood.h"
#include "../../include/libCacheSim/macro.h"
#include "../../include/libCacheSim/reader.h"
#include "./splaytree.h"

namespace mrcProfiler {

typedef enum {
  SHARDS_PROFILER,
  MINISIM_PROFILER,

  INVALID_PROFILER
} mrc_profiler_e;

static uint64_t get_hash_value_int_64_with_seed(uint64_t obj_id, uint64_t seed) {
  int64_t key = obj_id ^ seed;
  return get_hash_value_int_64(&key);
}

typedef struct profiler_params {
  struct {
    bool enable_fix_size;
    int64_t sample_size;
    double sample_rate;
    int64_t seed;

    void print() {
      printf("shards params:\n");
      printf("  enable_fix_size: %d\n", enable_fix_size);
      printf("  sample_size: %ld\n", sample_size);
      printf("  sample_rate: %f\n", sample_rate);
      printf("  seed: %ld\n", seed);
    }

    void parse_params(const char *str) {
      // format: FIX_RATE,0.01,random_seed|FIX_SIZE,8192,random_seed
      if (strlen(str) == 0) {
        printf("invalid params for shards\n");
        exit(1);
      }

      char buffer[1024];
      char *start = (char *)str;
      char *end = (char *)str;
      int current_param_idx = 0;
      while (*end != '\0') {
        end++;
        if (*end == ',' || *end == '\0') {
          // copy from start to end to buffer
          int need_size = end - start;
          if (need_size > 1024) {
            printf("params too long for shards: %s\n", str);
            exit(1);
          }
          memcpy(buffer, start, end - start);
          buffer[end - start] = '\0';

          if (current_param_idx == 0) {
            // check the sample type
            if (strcmp(buffer, "FIX_SIZE") == 0) {
              enable_fix_size = true;
            } else if (strcmp(buffer, "FIX_RATE") == 0) {
              enable_fix_size = false;
            } else {
              printf("invalid sample type for shards: %s\n", str);
              exit(1);
            }
          } else if (current_param_idx == 1) {
            // check the sample rate or sample size
            if (enable_fix_size) {
              sample_size = atoi(buffer);
              if (sample_size <= 0) {
                printf("invalid sample size for shards: %s\n", str);
                exit(1);
              }
            } else {
              sample_rate = atof(buffer);
              if (sample_rate <= 0 || sample_rate > 1) {
                printf("invalid sample rate for shards: %s\n", str);
                exit(1);
              }
            }
          } else if (current_param_idx == 2) {
            // check the seed
            seed = atoi(buffer);
          } else {
            printf("too many params for shards: %s\n", str);
            exit(1);
          }

          current_param_idx++;

          start = end + 1;
        }
      }
    }
  } shards_params;

  struct {
    double sample_rate;
    int64_t seed;

    void print() {
      printf("minisim params:\n");
      printf("  sample_rate: %f\n", sample_rate);
      printf("  seed: %ld\n", seed);
    }

    void parse_params(const char *str) {
      // format: FIX_RATE,0.01,random_seed|FIX_SIZE,8192,random_seed
      if (strlen(str) == 0) {
        printf("invalid params for shards\n");
        exit(1);
      }

      char buffer[1024];
      char *start = (char *)str;
      char *end = (char *)str;
      int current_param_idx = 0;
      while (*end != '\0') {
        end++;
        if (*end == ',' || *end == '\0') {
          // copy from start to end to buffer
          int need_size = end - start;
          if (need_size > 1024) {
            printf("params too long for shards: %s\n", str);
            exit(1);
          }
          memcpy(buffer, start, end - start);
          buffer[end - start] = '\0';

          if (current_param_idx == 0) {
            // check the sample type
            if (strcmp(buffer, "FIX_RATE") == 0) {
              ;
            } else {
              printf("invalid sample type for minisim: %s\n", str);
              exit(1);
            }
          } else if (current_param_idx == 1) {
            // check the sample rate or sample size
            sample_rate = atof(buffer);
            if (sample_rate <= 0 || sample_rate > 1) {
              printf("invalid sample rate for shards: %s\n", str);
              exit(1);
            }
          } else if (current_param_idx == 2) {
            // check the seed
            seed = atoi(buffer);
          } else {
            printf("too many params for shards: %s\n", str);
            exit(1);
          }

          current_param_idx++;

          start = end + 1;
        }
      }
    }
  } minisim_params;

  std::vector<size_t> profile_size;
  std::vector<double> profile_wss_ratio;
  char *cache_algorithm_str;

} profiler_params_t;

class MRCProfilerBase {
 public:
  MRCProfilerBase(reader_t *reader, std::string output_path, const profiler_params_t &params)
      : reader_(reader),
        output_path_(std::move(output_path)),
        params_(params),
        mrc_size_vec(params.profile_size),
        hit_cnt_vec(params.profile_size.size(), 0),
        hit_size_vec(params.profile_size.size(), 0) {}
  virtual void run() = 0;
  virtual void print() = 0;

 protected:
  reader_t *reader_ = nullptr;
  std::string output_path_;
  profiler_params_t params_;
  bool has_run_ = false;

  size_t n_req_ = 0;
  size_t sum_obj_size_req = 0;
  std::vector<size_t> mrc_size_vec;
  std::vector<int64_t> hit_cnt_vec;
  std::vector<int64_t> hit_size_vec;
};

class MRCProfilerSHARDS : public MRCProfilerBase {
 public:
  explicit MRCProfilerSHARDS(reader_t *reader, std::string output_path, const profiler_params_t &params)
      : MRCProfilerBase(reader, output_path, params) {
    ;
  }

  void run() override {
    if (has_run_) return;

    if (params_.shards_params.enable_fix_size) {
      fixed_sample_size_run();
    } else {
      fixed_sample_rate_run();
    }

    has_run_ = true;
  }

  void print() override {
    if (!has_run_) {
      printf("MRCProfilerSHARDS has not been run\n");
      return;
    }

    printf("SHARDS profiler:\n");

    printf("  n_req: %ld\n", n_req_);
    printf("  sum_obj_size_req: %ld\n", sum_obj_size_req);

    if(params_.profile_wss_ratio.size()!=0){
        printf("wss_ratio\t");
    }
    printf("cache_size\thit_rate\tbyte_hit_rate\n");
    for(int i = 0; i < mrc_size_vec.size(); i++){
        if(params_.profile_wss_ratio.size()!=0){
            printf("%lf\t", params_.profile_wss_ratio[i]);
        }
      printf("%ld\t%ld\t%ld\n", mrc_size_vec[i], hit_cnt_vec[i], hit_size_vec[i]);
    }
  }

 private:


  void fixed_sample_rate_run() {
    double sample_rate = params_.shards_params.sample_rate;
    request_t *req = new_request();
    read_one_req(reader_, req);

    uint64_t sample_max = UINT64_MAX * sample_rate;
    int64_t sampled_cnt = 0, sampled_size = 0;
    int64_t current_time = 0;

    robin_hood::unordered_map<obj_id_t, int64_t> last_access_time_map;
    SplayTree<int64_t, uint64_t> rd_tree;
    /* going through the trace */
    do {
      DEBUG_ASSERT(req->obj_size != 0);
      n_req_ += 1;
      sum_obj_size_req += req->obj_size;

      uint64_t hash_value = get_hash_value_int_64_with_seed(req->obj_id, params_.shards_params.seed);
      current_time += 1;
      if (hash_value <= sample_max) {
        sampled_cnt += 1;
        sampled_size += req->obj_size;

        if (last_access_time_map.count(req->obj_id)) {
          int64_t last_access_time = last_access_time_map[req->obj_id];
          size_t stack_dist = rd_tree.getDistance(last_access_time)/sample_rate;

          last_access_time_map[req->obj_id] = current_time;

          // update tree
          rd_tree.erase(last_access_time);
          rd_tree.insert(current_time, req->obj_size);

          // find bucket to increase hit cnt and hit size
          auto it = std::lower_bound(mrc_size_vec.begin(), mrc_size_vec.end(), stack_dist);

          if(it != mrc_size_vec.end()){
            // update hit cnt and hit size
            int idx = std::distance(mrc_size_vec.begin(), it);
            hit_cnt_vec[idx] += 1;
            hit_size_vec[idx] += req->obj_size;
          }

        } else {
          last_access_time_map[req->obj_id] = current_time;
          // update the tree
          rd_tree.insert(current_time, req->obj_size);
        }
      }

      read_one_req(reader_, req);
    } while (req->valid);

    
    int64_t should_sampled_cnt = n_req_ * sample_rate, should_sampled_size = sum_obj_size_req * sample_rate;
    printf("sampled_cnt: %ld, sampled_size: %ld\n", sampled_cnt, sampled_size);
    printf("should_sampled_cnt: %ld, should_sampled_size: %ld\n", should_sampled_cnt, should_sampled_size);
    hit_cnt_vec[0] += should_sampled_cnt - sampled_cnt;
    hit_size_vec[0] += should_sampled_size - sampled_size;
  }

  void fixed_sample_size_run() { ; }
};

class MRCProfilerMINISIM : public MRCProfilerBase {
 public:
  explicit MRCProfilerMINISIM(reader_t *reader, std::string output_path, const profiler_params_t &params)
      : MRCProfilerBase(reader, output_path, params) {}

  void run() override {}
  void print() override {
    if (!has_run_) {
      printf("MRCProfilerMINISIM has not been run\n");
      return;
    }

    printf("MINISIM profiler:\n");
  }

 private:
};

MRCProfilerBase *create_mrc_profiler(mrc_profiler_e type, reader_t *reader, std::string output_path,
                                     const profiler_params_t &params);

}  // namespace mrcProfiler
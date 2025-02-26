#pragma once
#include <inttypes.h>

#include <vector>

#include "../../dataStructure/hash/hash.h"
#include "../../dataStructure/robin_hood.h"
#include "../../include/libCacheSim.h"
#include "../../include/libCacheSim/cache.h"
#include "../../include/libCacheSim/macro.h"
#include "../../include/libCacheSim/plugin.h"
#include "../../include/libCacheSim/reader.h"
#include "../../include/libCacheSim/simulator.h"
#include "./minvaluemap.h"
#include "./splaytree.h"

namespace mrcProfiler {

#define MAX_MRC_PROFILE_POINTS 128

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
    int64_t thread_num;

    void print() {
      printf("minisim params:\n");
      printf("  sample_rate: %f\n", sample_rate);
      printf("  thread_num: %ld\n", thread_num);
    }

    void parse_params(const char *str) {
      // format: FIX_RATE,0.01,thread_num
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
            // check the thread_num
            thread_num = atoi(buffer);
            if (thread_num <= 0) {
              printf("invalid thread_num for minisim: %s\n", str);
              exit(1);
            }
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

} mrc_profiler_params_t;

class MRCProfilerBase {
 public:
  MRCProfilerBase(reader_t *reader, std::string output_path, const mrc_profiler_params_t &params)
      : reader_(reader),
        output_path_(std::move(output_path)),
        params_(params),
        mrc_size_vec(params.profile_size),
        hit_cnt_vec(params.profile_size.size(), 0),
        hit_size_vec(params.profile_size.size(), 0) {}
  virtual void run() = 0;
  void print() {
    if (!has_run_) {
      printf("MRCProfiler has not been run\n");
      return;
    }

    printf("%s profiler:\n", profiler_name_);
    printf("  n_req: %ld\n", n_req_);
    printf("  sum_obj_size_req: %ld\n", sum_obj_size_req);

    if (params_.profile_wss_ratio.size() != 0) {
      printf("wss_ratio\t");
    }
    printf("cache_size\tmiss_rate\tbyte_miss_rate\n");
    for (int i = 0; i < mrc_size_vec.size(); i++) {
      if (params_.profile_wss_ratio.size() != 0) {
        printf("%lf\t", params_.profile_wss_ratio[i]);
      }
      double miss_rate = 1 - (double)hit_cnt_vec[i] / (n_req_);
      double byte_miss_rate = 1 - (double)hit_size_vec[i] / (sum_obj_size_req);

      // clip to [0, 1]
      miss_rate = miss_rate > 1 ? 1 : (miss_rate < 0 ? 0 : miss_rate);
      byte_miss_rate = byte_miss_rate > 1 ? 1 : (byte_miss_rate < 0 ? 0 : byte_miss_rate);
      printf("%ldB\t%lf\t%lf\n", mrc_size_vec[i], miss_rate, byte_miss_rate);
    }
  }

 protected:
  reader_t *reader_ = nullptr;
  std::string output_path_;
  mrc_profiler_params_t params_;
  bool has_run_ = false;
  char * profiler_name_ = nullptr;

  size_t n_req_ = 0;
  size_t sum_obj_size_req = 0;
  std::vector<size_t> mrc_size_vec;
  std::vector<int64_t> hit_cnt_vec;
  std::vector<int64_t> hit_size_vec;
};

class MRCProfilerSHARDS : public MRCProfilerBase {
 public:
  explicit MRCProfilerSHARDS(reader_t *reader, std::string output_path, const mrc_profiler_params_t &params)
      : MRCProfilerBase(reader, output_path, params) {
    profiler_name_ = "SHARDS";
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


 private:
  void fixed_sample_rate_run() {
    // 1. init
    request_t *req = new_request();
    double sample_rate = params_.shards_params.sample_rate;
    std::vector<double> local_hit_cnt_vec(mrc_size_vec.size(), 0);
    std::vector<double> local_hit_size_vec(mrc_size_vec.size(), 0);
    uint64_t sample_max = UINT64_MAX * sample_rate;
    if (sample_rate == 1) {
      printf("sample_rate is 1, no need to sample\n");
      sample_max = UINT64_MAX;
    }
    double sampled_cnt = 0, sampled_size = 0;
    int64_t current_time = 0;
    robin_hood::unordered_map<obj_id_t, int64_t> last_access_time_map;
    SplayTree<int64_t, uint64_t> rd_tree;

    // 2. go through the trace
    read_one_req(reader_, req);
    /* going through the trace */
    do {
      DEBUG_ASSERT(req->obj_size != 0);
      n_req_ += 1;
      sum_obj_size_req += req->obj_size;

      uint64_t hash_value = get_hash_value_int_64_with_seed(req->obj_id, params_.shards_params.seed);
      current_time += 1;
      if (hash_value <= sample_max) {
        sampled_cnt += 1.0 / sample_rate;
        sampled_size += 1.0 * req->obj_size / sample_rate;

        if (last_access_time_map.count(req->obj_id)) {
          int64_t last_access_time = last_access_time_map[req->obj_id];
          size_t stack_distance = rd_tree.getDistance(last_access_time) / sample_rate;

          last_access_time_map[req->obj_id] = current_time;

          // update tree
          rd_tree.erase(last_access_time);
          rd_tree.insert(current_time, req->obj_size);

          // find bucket to increase hit cnt and hit size
          auto it = std::lower_bound(mrc_size_vec.begin(), mrc_size_vec.end(), stack_distance);

          if (it != mrc_size_vec.end()) {
            // update hit cnt and hit size
            int idx = std::distance(mrc_size_vec.begin(), it);
            local_hit_cnt_vec[idx] += 1.0 / sample_rate;
            local_hit_size_vec[idx] += 1.0 * req->obj_size / sample_rate;
          }

        } else {
          last_access_time_map[req->obj_id] = current_time;
          // update the tree
          rd_tree.insert(current_time, req->obj_size);
        }
      }

      read_one_req(reader_, req);
    } while (req->valid);

    // 3. adjust the hit cnt and hit size
    local_hit_cnt_vec[0] += n_req_ - sampled_cnt;
    local_hit_size_vec[0] += sum_obj_size_req - sampled_size;

    free_request(req);

    // 4. calculate the mrc
    int64_t accu_hit_cnt = 0, accu_hit_size = 0;
    for (int i = 0; i < mrc_size_vec.size(); i++) {
      accu_hit_cnt += local_hit_cnt_vec[i];
      accu_hit_size += local_hit_size_vec[i];
      hit_cnt_vec[i] = accu_hit_cnt;
      hit_size_vec[i] = accu_hit_size;
    }
  }

  void fixed_sample_size_run() {
    double sample_rate = 1.0;
    request_t *req = new_request();
    read_one_req(reader_, req);

    uint64_t sample_max = UINT64_MAX;
    int64_t max_to_keep = params_.shards_params.sample_size;
    double sampled_cnt = 0, sampled_size = 0;
    int64_t current_time = 0;

    MinValueMap<int64_t, uint64_t> min_value_map(max_to_keep);

    robin_hood::unordered_map<obj_id_t, int64_t> last_access_time_map;
    SplayTree<int64_t, uint64_t> rd_tree;
    /* going through the trace */
    do {
      DEBUG_ASSERT(req->obj_size != 0);
      n_req_ += 1;
      sum_obj_size_req += req->obj_size;

      uint64_t hash_value = get_hash_value_int_64_with_seed(req->obj_id, params_.shards_params.seed);
      current_time += 1;
      if (!min_value_map.full() || hash_value <= min_value_map.get_max_value()) {
        // this is a sampled req
        if (!min_value_map.full()) {
          sample_rate = 1.0;  // still 100% sample rate
        } else {
          sample_rate = min_value_map.get_max_value() * 1.0 / UINT64_MAX;  // change the sample rate
          printf("sample_rate: %lf\n", sample_rate);
        }
        int64_t poped_id = min_value_map.insert(req->obj_id, hash_value);

        if (poped_id != -1) {
          // this is a sampled req
          int64_t poped_id_access_time = last_access_time_map[poped_id];
          rd_tree.erase(poped_id_access_time);
          last_access_time_map.erase(poped_id);
        }

        sampled_cnt += 1.0 / sample_rate;
        sampled_size += 1.0 * req->obj_size / sample_rate;

        if (last_access_time_map.count(req->obj_id)) {
          int64_t last_acc_time = last_access_time_map[req->obj_id];
          int64_t stack_distance = rd_tree.getDistance(last_acc_time) * 1.0 / sample_rate;

          last_access_time_map[req->obj_id] = current_time;

          rd_tree.erase(last_acc_time);
          rd_tree.insert(current_time, req->obj_size);

          // find bucket to increase hit cnt and hit size
          auto it = std::lower_bound(mrc_size_vec.begin(), mrc_size_vec.end(), stack_distance);

          printf("it: %d\n");

          if (it != mrc_size_vec.end()) {
            // update hit cnt and hit size
            int idx = std::distance(mrc_size_vec.begin(), it);
            hit_cnt_vec[idx] += 1.0 / sample_rate;
            hit_size_vec[idx] += req->obj_size * 1.0 / sample_rate;
          }
        } else {
          last_access_time_map[req->obj_id] = current_time;
          rd_tree.insert(current_time, req->obj_size);
        }
      }

      read_one_req(reader_, req);
    } while (req->valid);

    // adjust the hit cnt and hit size
    hit_cnt_vec[0] += n_req_ - sampled_cnt;
    hit_size_vec[0] += sum_obj_size_req - sampled_size;

    free_request(req);
  }
};

class MRCProfilerMINISIM : public MRCProfilerBase {
 public:
  explicit MRCProfilerMINISIM(reader_t *reader, std::string output_path, const mrc_profiler_params_t &params)
      : MRCProfilerBase(reader, output_path, params) {
        profiler_name_ = "MINISIM";
      }

  void run() override {
    has_run_ = true;

    // 1. obtain the n_req_ and sum_obj_size_req
    request_t *req = new_request();
    read_one_req(reader_, req);
    double sample_rate = params_.minisim_params.sample_rate;
    double sampled_cnt = 0, sampled_size = 0;

    do {
      DEBUG_ASSERT(req->obj_size != 0);
      n_req_ += 1;
      sum_obj_size_req += req->obj_size;

      read_one_req(reader_, req);
    } while (req->valid);
    // 2. set spatial sampling to the reader, and obtain the sampled_cnt and sampled_size
    reset_reader(reader_);
    if (sample_rate > 0.5) {
      printf("sample_rate is too large, do not sample\n");
    } else {
      sampler_t *sampler = create_spatial_sampler(sample_rate);
      set_spatial_sampler_seed(sampler, 10000019);
      reader_->init_params.sampler = sampler;
      reader_->sampler = sampler;
      printf("sampling_ratio_inv: %d\n", sampler->sampling_ratio_inv);
    }
    read_one_req(reader_, req);
    do {
      DEBUG_ASSERT(req->obj_size != 0);
      sampled_cnt += 1;
      sampled_size += req->obj_size;

      read_one_req(reader_, req);
    } while (req->valid);

    // 3. run the simulate_with_multi_caches
    cache_t *caches[MAX_MRC_PROFILE_POINTS];
    for (int i = 0; i < params_.profile_size.size(); i++) {
      size_t _cache_size = mrc_size_vec[i] * sample_rate;
      // size_t _cache_size = mrc_size_vec[i];
      common_cache_params_t cc_params = {.cache_size = _cache_size};
      caches[i] = create_cache(params_.cache_algorithm_str, cc_params, nullptr);
    }
    result = simulate_with_multi_caches(reader_, caches, mrc_size_vec.size(), NULL, 0, 0,
                                        params_.minisim_params.thread_num, true, true);

    // 4. adjust hit cnt and hit size
    printf("sampled_cnt: %lf, sampled_size: %lf\n", sampled_cnt, sampled_size);
    for (int i = 0; i < mrc_size_vec.size(); i++) {

      printf("n_miss %ld, n_miss_byte %ld\n", result[i].n_miss, result[i].n_miss_byte);

      hit_cnt_vec[i] = n_req_ - result[i].n_miss * reader_->sampler->sampling_ratio_inv;
      hit_size_vec[i] = sum_obj_size_req - result[i].n_miss_byte * reader_->sampler->sampling_ratio_inv;
    }
  }

 private:
  cache_stat_t *result = nullptr;
};

MRCProfilerBase *create_mrc_profiler(mrc_profiler_e type, reader_t *reader, std::string output_path,
                                     const mrc_profiler_params_t &params);

}  // namespace mrcProfiler
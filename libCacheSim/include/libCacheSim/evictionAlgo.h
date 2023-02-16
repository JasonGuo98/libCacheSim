#pragma once

#include "cache.h"

typedef struct {
  cache_obj_t *q_head;
  cache_obj_t *q_tail;
} FIFO_params_t;

/* used by LFU related */
typedef struct {
  cache_obj_t *q_head;
  cache_obj_t *q_tail;
} LRU_params_t;

/* used by LFU related */
typedef struct freq_node {
  int64_t freq;
  cache_obj_t *first_obj;
  cache_obj_t *last_obj;
  uint32_t n_obj;
} freq_node_t;

cache_t *ARC_init(const common_cache_params_t ccache_params,
                  const char *cache_specific_params);

cache_t *ARCv0_init(const common_cache_params_t ccache_params,
                    const char *cache_specific_params);

cache_t *Belady_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *BeladySize_init(const common_cache_params_t ccache_params,
                         const char *cache_specific_params);

cache_t *Cacheus_init(const common_cache_params_t ccache_params,
                      const char *cache_specific_params);

cache_t *Clock_init(const common_cache_params_t ccache_params,
                    const char *cache_specific_params);

cache_t *CR_LFU_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *FIFO_init(const common_cache_params_t ccache_params,
                   const char *cache_specific_params);

cache_t *GDSF_init(const common_cache_params_t ccache_params,
                   const char *cache_specific_params);

cache_t *Hyperbolic_init(const common_cache_params_t ccache_params,
                         const char *cache_specific_params);

cache_t *LeCaR_init(const common_cache_params_t ccache_params,
                    const char *cache_specific_params);

cache_t *LeCaRv0_init(const common_cache_params_t ccache_params,
                      const char *cache_specific_params);

cache_t *LFU_init(const common_cache_params_t ccache_params,
                  const char *cache_specific_params);

cache_t *LFUCpp_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *LFUDA_init(const common_cache_params_t ccache_params,
                    const char *cache_specific_params);

cache_t *LHD_init(const common_cache_params_t ccache_params,
                  const char *cache_specific_params);

cache_t *LRU_init(const common_cache_params_t ccache_params,
                  const char *cache_specific_params);

cache_t *LRUv0_init(const common_cache_params_t ccache_params,
                    const char *cache_specific_params);

cache_t *MRU_init(const common_cache_params_t ccache_params,
                  const char *cache_specific_params);

cache_t *Random_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *SLRU_init(const common_cache_params_t ccache_params,
                   const char *cache_specific_params);

cache_t *SLRUv0_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *SFIFOv0_init(const common_cache_params_t ccache_params,
                      const char *cache_specific_params);

cache_t *SR_LRU_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *TwoQ_init(const common_cache_params_t ccache_params,
                   const char *cache_specific_params);

cache_t *LIRS_init(const common_cache_params_t ccache_params,
                   const char *cache_specific_params);

#ifdef ENABLE_LRB
cache_t *LRB_init(const common_cache_params_t ccache_params,
                  const char *cache_specific_params);
#endif

#ifdef INCLUDE_PRIV
cache_t *LRU_Prob_init(const common_cache_params_t ccache_params,
                       const char *cache_specific_params);

cache_t *MyClock_init(const common_cache_params_t ccache_params,
                      const char *cache_specific_params);

cache_t *QDLP_init(const common_cache_params_t ccache_params,
                   const char *cache_specific_params);

cache_t *QDLPv1_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *QDLPv2_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *myMQv1_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *MClock_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *SFIFO_Merge_init(const common_cache_params_t ccache_params,
                          const char *cache_specific_params);

cache_t *SFIFO_Reinsertion_init(const common_cache_params_t ccache_params,
                                const char *cache_specific_params);

cache_t *SFIFO_init(const common_cache_params_t ccache_params,
                    const char *cache_specific_params);

cache_t *WTinyLFU_init(const common_cache_params_t ccache_params,
                       const char *cache_specific_params);

cache_t *LP_SFIFO_init(const common_cache_params_t ccache_params,
                       const char *cache_specific_params);

cache_t *LP_ARC_init(const common_cache_params_t ccache_params,
                     const char *cache_specific_params);

cache_t *LP_TwoQ_init(const common_cache_params_t ccache_params,
                      const char *cache_specific_params);
#endif

#if defined(ENABLE_GLCACHE) && ENABLE_GLCACHE == 1

cache_t *GLCache_init(const common_cache_params_t ccache_params,
                      const char *cache_specific_params);

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <memory>
#include <ostream>
#include <vector>
#include <set>
#include <unordered_map>
#include "./mrcProfiler.h"
#include "../include/libCacheSim/const.h"



template <typename K, typename V>
class MinValueMap
{
public:
    MinValueMap(size_t n) : n(n) {}

    bool find(const K &key){
        return map.count(key);
    }


    K insert(const K &key, const V &value, bool & poped)
    {
        poped = false;
        auto it = map.find(key);
        if (it != map.end())
        {
            // Key already exists, update its value
            auto setIt = set.find({it->second, it->first});
            set.erase(setIt);
            set.insert({value, key});
            it->second = value;
        }
        else
        {
            // New key
            map[key] = value;
            if (set.size() < n)
            {
                set.insert({value, key});
            }
            else if (value < set.rbegin()->first)
            {
                auto last = *set.rbegin();
                set.erase(last);
                set.insert({value, key});
                map.erase(last.second);
                poped = true;
                return last.second;
            }
        }
        return -1;
    }

    bool full() const
    {
        return set.size() == n;
    }

    bool empty() const
    {
        return set.empty();
    }

    V get_max_value() const
    {
        return set.rbegin()->first;
    }

    size_t n;
    std::set<std::pair<V, K>> set;
    std::unordered_map<K, V> map;
private:
};

template <typename K, typename V>
class SplayTree
{
    struct node
    {
        node() : parent(nullptr) {}
        node(K key, V val) : first(key), second(val), sum(val), size(1), parent(nullptr) {}
        std::unique_ptr<node> left, right;
        node *parent;
        K first;
        V second;
        V sum;
        uint32_t size; // size of childs
        void maintain()
        {
            size = 1;
            sum = second;
            if (left)
            {
                size += left->size;
                sum += left->sum;
            }
            if (right)
            {
                size += right->size;
                sum += right->sum;
            }
        }
    };
    typedef typename std::unique_ptr<node> node_ptr;

public:
    class iterator
    {
    public:
        iterator() : ptr_(nullptr) {}
        iterator(node *ptr) : ptr_(ptr) {}
        iterator(const iterator &it) { ptr_ = it.ptr_; }
        iterator &operator=(iterator it)
        {
            if (this == &it)
                return *this;
            ptr_ = it.ptr_;
            return *this;
        }

        iterator &operator--()
        {
            node *prev = ptr_;
            if (ptr_ != nullptr)
            {
                if (ptr_->left != nullptr)
                {
                    ptr_ = ptr_->left.get();
                    while (ptr_->right != nullptr)
                    {
                        ptr_ = ptr_->right.get();
                    }
                }
                else
                {
                    ptr_ = ptr_->parent;
                    while (ptr_ != nullptr && ptr_->left.get() == prev)
                    {
                        prev = ptr_;
                        ptr_ = ptr_->parent;
                    }
                }
            }
            return *this;
        }
        iterator &operator++()
        {
            node *prev = ptr_;
            if (ptr_ != nullptr)
            {
                if (ptr_->right != nullptr)
                {
                    ptr_ = ptr_->right.get();
                    while (ptr_->left)
                        ptr_ = (ptr_->left).get();
                }
                else
                {
                    ptr_ = ptr_->parent;
                    while (ptr_ != nullptr && ptr_->right.get() == prev)
                    {
                        prev = ptr_;
                        ptr_ = ptr_->parent;
                    }
                }
            }
            return *this;
        }
        iterator operator++(int junk)
        {
            iterator prev = *this;
            ++(*this);
            return prev;
        }
        iterator operator--(int junk)
        {
            iterator prev = *this;
            --(*this);
            return prev;
        }
        std::pair<K, V> operator*() { return std::make_pair(ptr_->first, ptr_->second); }
        node *operator->() { return ptr_; }
        bool operator==(const iterator &rhs) { return ptr_ == rhs.ptr_; }
        bool operator!=(const iterator &rhs) { return ptr_ != rhs.ptr_; }

    private:
        node *ptr_;
    };

public:
    SplayTree() : node_count_{0}, sum(0) {}
    ~SplayTree() {}
    void insert(const K &key, const V &val);
    iterator find(const K &key) { return find_(key) ? iterator(find_(key)) : end(); }
    V getK(int k) { return getK_(root_.get(), k); }
    int getRank(K key) { return getRank_(root_.get(), key); }

    V getSum(K key) { return getSum_(root_.get(), key); }
    V getDistance(K key) { return getDistance_(root_.get(), key); }

    void erase(const K &key) { remove(find_(key)); }
    void erase(iterator pos) { remove(pos.operator->()); }
    void get_dot(std::ostream &o) { getDOT(o); }
    void clear() { purgeTree(root_); }
    bool empty() const { return node_count_ == 0 ? true : false; }
    uint32_t size() const { return node_count_; }
    iterator begin() { return node_count_ == 0 ? end() : iterator(subtreeMin(root_.get())); }
    iterator end() { return iterator(); }
    void swap(SplayTree &other)
    {
        node_ptr temp = move(root_);
        root_ = move(other.root_);
        other.root_ = move(temp);
    }
    V sum;

private:
    uint32_t node_count_;

    node_ptr root_;
    void leftRotate(node *x);
    void rightRotate(node *x);
    void splay(node *x);
    V getK_(node *x, int k);
    int getRank_(node *x, K k);
    V getSum_(node *x, K key);
    V getDistance_(node *x, K key);
    node *subtreeMax(node *p) const; // find max in subtree
    node *subtreeMin(node *p) const; // find min in subtree
    node *find_(const K &key);
    node *find_(const K &key) const { return find_(key); }
    void purgeTree(node_ptr &n);
    void getDOT(std::ostream &o);
    void remove(node *n);
};

template <typename K, typename V>
void SplayTree<K, V>::leftRotate(node *rt)
{
    node *pivot = rt->right.get();
    node_ptr pv = move(rt->right);
    node *grand_pa = rt->parent;
    if (pv->left)
    {
        rt->right = move(pv->left);
        rt->right->parent = rt;
    }

    if (!grand_pa)
    {
        pv->left = move(root_);
        root_ = move(pv);
        pivot->parent = nullptr;
        rt->parent = pivot;
    }
    else
    {
        if (grand_pa->right.get() == rt)
        {
            pv->left = move(grand_pa->right);
            grand_pa->right = move(pv);
        }
        else
        {
            pv->left = move(grand_pa->left);
            grand_pa->left = move(pv);
        }
        pivot->parent = grand_pa;
        rt->parent = pivot;
    }
    rt->maintain();
    pivot->maintain();
    if (grand_pa)
        grand_pa->maintain();
}

template <typename K, typename V>
void SplayTree<K, V>::rightRotate(node *rt)
{
    node *pivot = rt->left.get();
    node_ptr pv = move(rt->left);
    node *grand_pa = rt->parent;
    if (pv->right)
    { // right node of pv to left node of rt
        rt->left = move(pv->right);
        rt->left->parent = rt;
    }
    if (!grand_pa)
    { // if rt is really root of the tree
        pv->right = move(root_);
        root_ = move(pv);
        pivot->parent = nullptr;
        rt->parent = pivot;
    }
    else
    {
        if (grand_pa->right.get() == rt)
        { // rt is in right branch from his grandparent
            pv->right = move(grand_pa->right);
            grand_pa->right = move(pv);
            pivot->parent = grand_pa;
            rt->parent = pivot;
        }
        else
        { // rt is in left branch from his grandparent
            pv->right = move(grand_pa->left);
            grand_pa->left = move(pv);
            pivot->parent = grand_pa;
            rt->parent = pivot;
        }
    }
    rt->maintain();
    pivot->maintain();
    if (grand_pa)
        grand_pa->maintain();
}

template <typename K, typename V>
void SplayTree<K, V>::splay(node *x)
{
    if (x == nullptr)
        return;
    while (x->parent)
    {
        if (!x->parent->parent) // zig step
        {
            if (x == x->parent->left.get())
                rightRotate(x->parent);
            else
                leftRotate(x->parent);
        }
        else if (x == x->parent->left.get() && x->parent == x->parent->parent->left.get()) // zig zig step left
        {
            rightRotate(x->parent->parent);
            rightRotate(x->parent);
        }
        else if (x == x->parent->right.get() && x->parent == x->parent->parent->right.get()) // zig zig step right
        {
            leftRotate(x->parent->parent);
            leftRotate(x->parent);
        }
        else if (x == x->parent->left.get() && x->parent == x->parent->parent->right.get()) // zig zag rotation left
        {
            rightRotate(x->parent);
            leftRotate(x->parent);
        }
        else // zig zag rotation righ
        {
            leftRotate(x->parent);
            rightRotate(x->parent);
        }
    }
}

template <typename K, typename V>
V SplayTree<K, V>::getK_(node *rt, int k)
{
    if (rt->left)
    {
        if (rt->left->size >= k)
            return getK_(rt->left.get(), k);
        else
            k -= rt->left->size;
    }
    if (k == 1)
        return rt->second;
    return getK_(rt->right.get(), k - 1);
}

template <typename K, typename V>
int SplayTree<K, V>::getRank_(node *rt, K k)
{
    int rs = 0;
    if (k < rt->first)
        return getRank_(rt->left.get(), k);
    if (rt->left)
        rs += rt->left->size;
    if (k == rt->first)
    {
        return rs + 1;
    }
    else
    {
        return rs + 1 + getRank_(rt->right.get(), k);
    }
}

template <typename K, typename V>
V SplayTree<K, V>::getSum_(node *rt, K key)
{
    V rs = V(0);
    if (key < rt->first)
        return getSum_(rt->left.get(), key);
    if (rt->left)
        rs = rt->left->sum;
    if (key == rt->first)
        return rs;
    else
        return rs + rt->second + getSum_(rt->right.get(), key);
}
template <typename K, typename V>
V SplayTree<K, V>::getDistance_(node *x, K key)
{
    if (x == nullptr)
        return 0;
    if (key > x->first)
    {
        return getDistance_(x->right.get(), key);
    }
    else if (key == x->first)
    {
        if (x->left)
        {
            return x->sum - x->left->sum;
        }
        else
        {
            return x->sum;
        }
    }
    else
    {
        if (x->left)
        {
            return getDistance_(x->left.get(), key) + x->sum - x->left->sum;
        }
        else
        {
            return getDistance_(x->left.get(), key) + x->sum;
        }
    }
}

template <typename K, typename V>
typename SplayTree<K, V>::node *SplayTree<K, V>::subtreeMax(node *p) const // find max in subtree
{
    node *t = p;
    while (t->right)
        t = (t->right).get();
    return t;
}

template <typename K, typename V>
typename SplayTree<K, V>::node *SplayTree<K, V>::subtreeMin(node *p) const // find min in subtree
{
    node *t = p;
    while (t->left)
        t = t->left.get();
    return t;
}

template <typename K, typename V>
void SplayTree<K, V>::purgeTree(node_ptr &n)
{
    n.release();
}

template <typename K, typename V>
void SplayTree<K, V>::insert(const K &key, const V &val)
{
    node *p = root_.get();
    node *prev = nullptr;
    while (p)
    {
        prev = p;
        if (key == p->first)
        {
            p->second = val;
            return;
        }
        else if (key < p->first)
            p = p->left.get();
        else if (key > p->first)
            p = p->right.get();
    }
    node_ptr n(new node(key, val));

    ++node_count_;
    sum += val;

    if (prev == nullptr)
    {
        root_ = move(n);
    }
    else if (key < prev->first)
    {
        prev->left = move(n);
        prev->left->parent = prev;
        splay(prev->left.get());
    }
    else
    {
        prev->right = move(n);
        prev->right->parent = prev;
        splay(prev->right.get());
    }
}

template <typename K, typename V>
typename SplayTree<K, V>::node *SplayTree<K, V>::find_(const K &key)
{
    node *p = root_.get();
    while (p)
    {
        if (key < p->first)
            p = p->left.get();
        else if (key > p->first)
            p = p->right.get();
        else
        {
            splay(p);
            return root_.get();
        }
    }
    return nullptr;
}

template <typename K, typename V>
void SplayTree<K, V>::remove(node *n)
{
    if (n == nullptr)
        return;
    splay(n);
    --node_count_;
    sum = sum - n->second;
    if (node_count_ == 0)
    {
        root_.release();
        return;
    }
    if (!n->left)
    {
        root_ = move(n->right);
        root_->parent = nullptr;
        return;
    }
    else if (!n->right)
    {
        root_ = move(n->left);
        root_->parent = nullptr;
    }
    else
    {
        node *nxt = subtreeMin(n->right.get());
        splay(nxt);
        root_->left = move(root_->left->left);
        root_->left->parent = root_.get();
    }
    root_->maintain();
}

template <typename K, typename V>
void SplayTree<K, V>::getDOT(std::ostream &o)
{
    o << "digraph G{ graph[ordering = out];" << std::endl;
    if (node_count_ == 1)
        o << root_->first << ";" << std::endl;
    else
    {
        for (auto it = begin(); it != end(); ++it)
        {
            // 1[label="1 (size=1)"];
            char out[30];
            sprintf(out, "%d[label=\"%d (size=%d)\"];", it->first, it->first, (int)it->size);
            o << out << std::endl;
            if (it->parent)
            {
                std::string type = (iterator(it->parent->right.get()) == it ? "right" : "left");
                o << it->parent->first << " -> " << it->first << "[label=\"" << type << "\"];" << std::endl;
            }
        }
    }
    o << "}" << std::endl;
}

mrcProfiler::MRCProfilerBase * mrcProfiler::create_mrc_profiler(mrc_profiler_e type, reader_t *reader, std::string output_path, const mrc_profiler_params_t & params) {
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



void mrcProfiler::MRCProfilerBase::print(const char * output_path) {
  if (!has_run_) {
    printf("MRCProfiler has not been run\n");
    return;
  }

  FILE *outfp = stdout;
  bool open_output_file = false;
  if(output_path != nullptr && strlen(output_path) != 0){
    outfp = fopen(output_path, "w");
    open_output_file = true;
    if (outfp == nullptr) {
      printf("failed to open file %s, print to stdout\n", output_path);
      fclose(outfp);
      outfp = stdout;
      open_output_file = false;
    }
  }

  fprintf(outfp, "%s profiler:\n", profiler_name_);
  fprintf(outfp, "  trace: %s\n", reader_->trace_path);
  fprintf(outfp, "  cache_algorithm: %s\n", params_.cache_algorithm_str);
  fprintf(outfp, "  n_req: %ld\n", n_req_);
  fprintf(outfp, "  sum_obj_size_req: %ld\n", sum_obj_size_req);

  if (params_.profile_wss_ratio.size() != 0) {
    fprintf(outfp, "wss_ratio\t");
  }
  fprintf(outfp, "cache_size\tmiss_rate\tbyte_miss_rate\n");
  for (int i = 0; i < mrc_size_vec.size(); i++) {
    if (params_.profile_wss_ratio.size() != 0) {
      fprintf(outfp, "%lf\t", params_.profile_wss_ratio[i]);
    }
    double miss_rate = 1 - (double)hit_cnt_vec[i] / (n_req_);
    double byte_miss_rate = 1 - (double)hit_size_vec[i] / (sum_obj_size_req);

    // clip to [0, 1]
    miss_rate = miss_rate > 1 ? 1 : (miss_rate < 0 ? 0 : miss_rate);
    byte_miss_rate = byte_miss_rate > 1 ? 1 : (byte_miss_rate < 0 ? 0 : byte_miss_rate);
    fprintf(outfp, "%ldB\t%lf\t%lf\n", mrc_size_vec[i], miss_rate, byte_miss_rate);
  }

  if (open_output_file) {
    fclose(outfp);
  }
}

void mrcProfiler::MRCProfilerSHARDS::run() {
  if (has_run_) return;

  if (params_.shards_params.enable_fix_size) {
    fixed_sample_size_run();
  } else {
    fixed_sample_rate_run();
  }

  has_run_ = true;
}

void mrcProfiler::MRCProfilerSHARDS::fixed_sample_rate_run() {
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

void mrcProfiler::MRCProfilerSHARDS::fixed_sample_size_run() {
  // 1. init
  request_t *req = new_request();
  double sample_rate = 1.0;
  std::vector<double> local_hit_cnt_vec(mrc_size_vec.size(), 0);
  std::vector<double> local_hit_size_vec(mrc_size_vec.size(), 0);
  double sampled_cnt = 0, sampled_size = 0;
  int64_t current_time = 0;
  int64_t max_to_keep = params_.shards_params.sample_size;

  MinValueMap<int64_t, uint64_t> min_value_map(max_to_keep);
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
    if (!min_value_map.full() || hash_value < min_value_map.get_max_value() || last_access_time_map.count(req->obj_id)) {
      // this is a sampled req

      if(!last_access_time_map.count(req->obj_id)){
        bool poped = false;
        int64_t poped_id = min_value_map.insert(req->obj_id, hash_value, poped);
        if (poped) {
          // this is a sampled req
          int64_t poped_id_access_time = last_access_time_map[poped_id];
          rd_tree.erase(poped_id_access_time);
          last_access_time_map.erase(poped_id);
        }
      }

      if (!min_value_map.full()) {
        sample_rate = 1.0;  // still 100% sample rate
      } else {
        sample_rate = min_value_map.get_max_value() * 1.0 / UINT64_MAX;  // adjust the sample rate
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

        if (it != mrc_size_vec.end()) {
          // update hit cnt and hit size
          int idx = std::distance(mrc_size_vec.begin(), it);
          local_hit_cnt_vec[idx] += 1.0 / sample_rate;
          local_hit_size_vec[idx] += req->obj_size * 1.0 / sample_rate;
        }
      } else {
        last_access_time_map[req->obj_id] = current_time;
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


void mrcProfiler::MRCProfilerMINISIM::run(){
  has_run_ = true;

  request_t *req = new_request();
  double sample_rate = params_.minisim_params.sample_rate;
  double sampled_cnt = 0, sampled_size = 0;
  sampler_t *sampler = nullptr;
  if (sample_rate > 0.5) {
    printf("sample_rate is too large, do not sample\n");
  } else {
    sampler = create_spatial_sampler(sample_rate);
    set_spatial_sampler_seed(sampler, 10000019); // TODO: seed can be changed by params
  }

  // 1. obtain the n_req_, sum_obj_size_req, sampled_cnt and sampled_size
  read_one_req(reader_, req);
  do {
    DEBUG_ASSERT(req->obj_size != 0);
    n_req_ += 1;
    sum_obj_size_req += req->obj_size;
    if(sampler == nullptr || sampler->sample(sampler, req)){
      sampled_cnt += 1;
      sampled_size += req->obj_size;
    }

    read_one_req(reader_, req);
  } while (req->valid);
  // 2. set spatial sampling to the reader
  reset_reader(reader_);
  reader_->init_params.sampler = sampler;
  reader_->sampler = sampler;

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
  for (int i = 0; i < mrc_size_vec.size(); i++) {
    hit_cnt_vec[i] = n_req_ - result[i].n_miss * reader_->sampler->sampling_ratio_inv;
    hit_size_vec[i] = sum_obj_size_req - result[i].n_miss_byte * reader_->sampler->sampling_ratio_inv;
  }
}
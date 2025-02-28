// created by Xiaojun Guo, 02/28/25

#include <unordered_map>
#include <algorithm>
#include <vector>

using namespace std;

class SegmentTree {
private:
    vector<int64_t> tree, lazy;
    int size;


    void build(int node, int start, int end) {
        if (start == end) {
            tree[node] = 0;
        } else {
            int mid = (start + end) / 2;
            build(2 * node, start, mid);
            build(2 * node + 1, mid + 1, end);
            tree[node] = max(tree[2 * node], tree[2 * node + 1]);
        }
    }


    void updateRange(int node, int start, int end, int l, int r, int64_t val) {
        if (lazy[node] != 0) {
            tree[node] += lazy[node];
            if (start != end) {
                lazy[node * 2] += lazy[node];
                lazy[node * 2 + 1] += lazy[node];
            }
            lazy[node] = 0;
        }
        if (start > end || start > r || end < l)
            return;
        if (start >= l && end <= r) {
            tree[node] += val;
            if (start != end) {
                lazy[node * 2] += val;
                lazy[node * 2 + 1] += val;
            }
            return;
        }
        int mid = (start + end) / 2;
        updateRange(node * 2, start, mid, l, r, val);
        updateRange(node * 2 + 1, mid + 1, end, l, r, val);
        tree[node] = max(tree[node * 2], tree[node * 2 + 1]);
    }


    int64_t queryRange(int node, int start, int end, int l, int r) {
        if (start > end || start > r || end < l)
            return 0;
        if (lazy[node] != 0) {
            tree[node] += lazy[node];
            if (start != end) {
                lazy[node * 2] += lazy[node];
                lazy[node * 2 + 1] += lazy[node];
            }
            lazy[node] = 0;
        }
        if (start >= l && end <= r)
            return tree[node];
        int mid = (start + end) / 2;
        int64_t p1 = queryRange(node * 2, start, mid, l, r);
        int64_t p2 = queryRange(node * 2 + 1, mid + 1, end, l, r);
        return max(p1, p2);
    }


    void resizeTree(int newSize) {
        tree.resize(4 * newSize, 0);
        lazy.resize(4 * newSize, 0);
        build(1, 0, newSize - 1);
    }


public:
    SegmentTree() {
        size = 0;
    }


    void update(int l, int r, int64_t val) {
        if (r >= size) {
            int newSize = max(size * 2, r + 1);
            resizeTree(newSize);
            size = newSize;
        }
        updateRange(1, 0, size - 1, l, r, val);
    }


    int64_t query(int l, int r) {
        if (r >= size) {
            return 0;
        }
        return queryRange(1, 0, size - 1, l, r);
    }
};

class BeladyOnline {
    int64_t cache_size_;
    SegmentTree tree;
    unordered_map<int64_t, int64_t> last_access_vtime_;
    int64_t v_time_;

public:
    BeladyOnline(int64_t cache_size) {
        cache_size_ = cache_size;
        v_time_ = 0;
    }


    bool BeladyOnline_get(int64_t obj_id, int64_t size){
        bool ret = false;
        bool accessed_before = false;
        int64_t last_access_vtime = 0;

        /* 1. get the last access time */
        if(last_access_vtime_.count(obj_id)){
            accessed_before = true;
            last_access_vtime = last_access_vtime_[obj_id];
        }


        /* 2. decide whether it is a hit */
        if(accessed_before){
            int64_t current_occ = tree.query(last_access_vtime, v_time_);
            if((current_occ + size) <= cache_size_){
                tree.update(last_access_vtime, v_time_, size);
                ret = true;
            }
        }
        
        /* 3. update the last access time */
        last_access_vtime_[obj_id] = v_time_;
        
        /* 4. update the v_time_ */
        v_time_ ++;

        return ret;
    }

};

extern "C" {

    BeladyOnline* BeladyOnline_new(int64_t cache_size) { return new BeladyOnline(cache_size); }
    bool _BeladyOnline_get(BeladyOnline* obj, int64_t obj_id, int64_t size) { return obj->BeladyOnline_get(obj_id, size); }
    void BeladyOnline_delete(BeladyOnline* obj) { delete obj; }
}
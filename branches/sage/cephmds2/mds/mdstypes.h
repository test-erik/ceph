#ifndef __MDSTYPES_H
#define __MDSTYPES_H


#include <math.h>
#include <ostream>
#include <set>
#include <map>
using namespace std;

#include "config.h"
#include "common/DecayCounter.h"

#include <cassert>

#include "include/frag.h"


#define MDS_PORT_MAIN     0
#define MDS_PORT_SERVER   1
#define MDS_PORT_CACHE    2
#define MDS_PORT_LOCKER   3
#define MDS_PORT_STORE    4
#define MDS_PORT_BALANCER 5
#define MDS_PORT_MIGRATOR 6
#define MDS_PORT_RENAMER  7
#define MDS_PORT_ANCHORCLIENT 10
#define MDS_PORT_ANCHORTABLE  11

#define MAX_MDS                   0x100

#define MDS_INO_ROOT              1
#define MDS_INO_PGTABLE           2
#define MDS_INO_ANCHORTABLE       3
#define MDS_INO_LOG_OFFSET        0x100
#define MDS_INO_IDS_OFFSET        0x200
#define MDS_INO_STRAY_OFFSET      0x300
#define MDS_INO_BASE              0x1000

#define MDS_INO_STRAY(x) (MDS_INO_STRAY_OFFSET+(x))
#define MDS_INO_IS_STRAY(i) ((i) >= MDS_INO_STRAY_OFFSET && (i) < MDS_INO_STRAY_OFFSET+MAX_MDS)

#define MDS_TRAVERSE_FORWARD       1
#define MDS_TRAVERSE_DISCOVER      2    // skips permissions checks etc.
#define MDS_TRAVERSE_DISCOVERXLOCK 3    // succeeds on (foreign?) null, xlocked dentries.
#define MDS_TRAVERSE_FAIL          4


struct metareqid_t {
  int client;
  tid_t tid;
  metareqid_t() : client(-1), tid(0) {}
  metareqid_t(int c, tid_t t) : client(c), tid(t) {}
};

inline ostream& operator<<(ostream& out, const metareqid_t& r) {
  return out << "client" << r.client << ":" << r.tid;
}

inline bool operator==(const metareqid_t& l, const metareqid_t& r) {
  return (l.client == r.client) && (l.tid == r.tid);
}
inline bool operator!=(const metareqid_t& l, const metareqid_t& r) {
  return (l.client != r.client) || (l.tid != r.tid);
}
inline bool operator<(const metareqid_t& l, const metareqid_t& r) {
  return (l.client < r.client) || 
    (l.client == r.client && l.tid < r.tid);
}
inline bool operator<=(const metareqid_t& l, const metareqid_t& r) {
  return (l.client < r.client) ||
    (l.client == r.client && l.tid <= r.tid);
}
inline bool operator>(const metareqid_t& l, const metareqid_t& r) { return !(l <= r); }
inline bool operator>=(const metareqid_t& l, const metareqid_t& r) { return !(l < r); }

namespace __gnu_cxx {
  template<> struct hash<metareqid_t> {
    size_t operator()(const metareqid_t &r) const { 
      hash<__uint64_t> H;
      return H(r.client) ^ H(r.tid);
    }
  };
}




// ================================================================
// dir frag

struct dirfrag_t {
  inodeno_t ino;
  frag_t    frag;

  dirfrag_t() { }
  dirfrag_t(inodeno_t i, frag_t f) : ino(i), frag(f) { }
};

inline ostream& operator<<(ostream& out, const dirfrag_t& df) {
  return out << df.ino << "#" << df.frag;
}
inline bool operator<(dirfrag_t l, dirfrag_t r) {
  if (l.ino < r.ino) return true;
  if (l.ino == r.ino && l.frag < r.frag) return true;
  return false;
}
inline bool operator==(dirfrag_t l, dirfrag_t r) {
  return l.ino == r.ino && l.frag == r.frag;
}


// ================================================================

/* meta_load_t
 * hierarchical load for an inode/dir and it's children
 */
#define META_POP_IRD    0
#define META_POP_IWR    1
#define META_POP_DWR    2
//#define META_POP_LOG   3
//#define META_POP_FDIR  4
//#define META_POP_CDIR  4
#define META_NPOP      3

class meta_load_t {
 public:
  DecayCounter pop[META_NPOP];

  double meta_load() {
    return pop[META_POP_IRD].get() + 2*pop[META_POP_IWR].get();
  }

  void take(meta_load_t& other) {
    for (int i=0; i<META_NPOP; i++) {
      pop[i] = other.pop[i];
      other.pop[i].reset();
    }
  }
};

inline ostream& operator<<( ostream& out, meta_load_t& load )
{
  return out << "<rd " << load.pop[META_POP_IRD].get()
             << ", wr " << load.pop[META_POP_IWR].get()
             << ">";
}


inline meta_load_t& operator-=(meta_load_t& l, meta_load_t& r)
{
  for (int i=0; i<META_NPOP; i++)
    l.pop[i].adjust(- r.pop[i].get());
  return l;
}

inline meta_load_t& operator+=(meta_load_t& l, meta_load_t& r)
{
  for (int i=0; i<META_NPOP; i++)
    l.pop[i].adjust(r.pop[i].get());
  return l;
}



/* mds_load_t
 * mds load
 */

// popularity classes
#define MDS_POP_JUSTME  0   // just me (this dir or inode)
#define MDS_POP_NESTED  1   // me + children, auth or not
#define MDS_POP_CURDOM  2   // me + children in current auth domain
#define MDS_POP_ANYDOM  3   // me + children in any (nested) auth domain
//#define MDS_POP_DIRMOD  4   // just this dir, modifications only
#define MDS_NPOP        4

class mds_load_t {
 public:
  meta_load_t root;

  double req_rate;
  double cache_hit_rate;
  double queue_len;

  mds_load_t() : 
    req_rate(0), cache_hit_rate(0), queue_len(0) { }    

  double mds_load() {
    switch(g_conf.mds_bal_mode) {
    case 0: 
      return root.meta_load()
        + req_rate
        + 10.0*queue_len;

    case 1:
      return req_rate + 10.0*queue_len;
    }
    assert(0);
    return 0;
  }

};


inline ostream& operator<<( ostream& out, mds_load_t& load )
{
  return out << "mdsload<" << load.root
             << ", req " << load.req_rate 
             << ", hr " << load.cache_hit_rate
             << ", qlen " << load.queue_len
             << ">";
}

/*
inline mds_load_t& operator+=( mds_load_t& l, mds_load_t& r ) 
{
  l.root_pop += r.root_pop;
  l.req_rate += r.req_rate;
  l.queue_len += r.queue_len;
  return l;
}

inline mds_load_t operator/( mds_load_t& a, double d ) 
{
  mds_load_t r;
  r.root_pop = a.root_pop / d;
  r.req_rate = a.req_rate / d;
  r.queue_len = a.queue_len / d;
  return r;
}
*/




// ================================================================

#define MDS_PIN_REPLICATED     1
#define MDS_STATE_AUTH     (1<<0)

class MLock;
class Context;
class SimpleLock;

class MDSCacheObject {
 protected:
  unsigned state;     // state bits
  
  int      ref;       // reference count
  multiset<int> ref_set;

  map<int,int> replicas;      // [auth] mds -> nonce
  int          replica_nonce; // [replica] defined on replica

 public:
  MDSCacheObject() :
	state(0),
	ref(0),
	replica_nonce(0) {}
  virtual ~MDSCacheObject() {}
  
  // --------------------------------------------
  // state
  unsigned get_state() { return state; }
  void state_clear(unsigned mask) { state &= ~mask; }
  void state_set(unsigned mask) { state |= mask; }
  unsigned state_test(unsigned mask) { return state & mask; }
  void state_reset(unsigned s) { state = s; }

  bool is_auth() { return state & MDS_STATE_AUTH; }

  // --------------------------------------------
  // pins
  int get_num_ref() { return ref; }
  bool is_pinned_by(int by) { return ref_set.count(by); }
  multiset<int>& get_ref_set() { return ref_set; }
  virtual const char *pin_name(int by) = 0;

  virtual void last_put() {}
  virtual void bad_put(int by) {
	assert(ref_set.count(by) > 0);
	assert(ref > 0);
  }
  void put(int by) {
    if (ref == 0 || ref_set.count(by) == 0) {
	  bad_put(by);
    } else {
	  ref--;
	  ref_set.erase(ref_set.find(by));
	  assert(ref == (int)ref_set.size());
	  if (ref == 0)
		last_put();
	}
  }

  virtual void first_get() {}
  virtual void bad_get(int by) {
	assert(by < 0 || ref_set.count(by) == 0);
	assert(0);
  }
  void get(int by) {
    if (by >= 0 && ref_set.count(by)) {
	  bad_get(by);
    } else {
	  if (ref == 0) 
		first_get();
	  ref++;
	  ref_set.insert(by);
	  assert(ref == (int)ref_set.size());
	}
  }

  void print_pin_set(ostream& out) {
    multiset<int>::iterator it = ref_set.begin();
    while (it != ref_set.end()) {
      out << " " << pin_name(*it);
      int last = *it;
      int c = 1;
      do {
		it++;
		if (it == ref_set.end()) break;
      } while (*it == last);
      if (c > 1)
		out << "*" << c;
    }
  }


  // --------------------------------------------
  // replication
  bool is_replicated() { return !replicas.empty(); }
  bool is_replica(int mds) { return replicas.count(mds); }
  int num_replicas() { return replicas.size(); }
  int add_replica(int mds) {
	if (replicas.count(mds)) 
	  return ++replicas[mds];  // inc nonce
	if (replicas.empty()) 
	  get(MDS_PIN_REPLICATED);
	return replicas[mds] = 1;
  }
  void add_replica(int mds, int nonce) {
	if (replicas.empty()) 
	  get(MDS_PIN_REPLICATED);
	replicas[mds] = nonce;
  }
  int get_replica_nonce(int mds) {
	assert(replicas.count(mds));
	return replicas[mds];
  }
  void remove_replica(int mds) {
	assert(replicas.count(mds));
	replicas.erase(mds);
	if (replicas.empty())
	  put(MDS_PIN_REPLICATED);
  }
  void clear_replicas() {
	if (!replicas.empty())
	  put(MDS_PIN_REPLICATED);
	replicas.clear();
  }
  map<int,int>::iterator replicas_begin() { return replicas.begin(); }
  map<int,int>::iterator replicas_end() { return replicas.end(); }
  const map<int,int>& get_replicas() { return replicas; }

  int get_replica_nonce() { return replica_nonce;}
  void set_replica_nonce(int n) { replica_nonce = n; }


  // ---------------------------------------------
  // locking
  // noop unless overloaded.
  virtual SimpleLock* get_lock(int type) { assert(0); }
  virtual void set_mlock_info(MLock *m) { assert(0); }
  virtual void encode_lock_state(int type, bufferlist& bl) { assert(0); }
  virtual void decode_lock_state(int type, bufferlist& bl) { assert(0); }
  virtual void finish_lock_waiters(int type, int mask, int r=0) { assert(0); }
  virtual void add_lock_waiter(int type, int mask, Context *c) { assert(0); }
  virtual bool is_lock_waiting(int type, int mask) { assert(0); return false; }


  // ---------------------------------------------
  // ordering
  virtual bool is_lt(const MDSCacheObject *r) const = 0;
  struct ptr_lt {
    bool operator()(const MDSCacheObject* l, const MDSCacheObject* r) const {
      return l->is_lt(r);
    }
  };

  // printing
  virtual void print(ostream& out) = 0;
};

inline ostream& operator<<(ostream& out, MDSCacheObject& o) {
  o.print(out);
  return out;
}


#endif

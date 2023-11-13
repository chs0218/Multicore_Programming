#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <array>
#include <mutex>
#include <set>

using namespace std;
using namespace chrono;

constexpr int MAX_THREADS = 32;

class null_mutex {
public:
	void lock() {}
	void unlock() {}
};
class NODE {
	mutex n_lock;
public:
	int v;
	NODE* volatile next;
	volatile bool removed;
	NODE() : v(-1), next(nullptr), removed(false) {}
	NODE(int x) : v(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};


class SPNODE {
	mutex n_lock;
public:
	int v;
	shared_ptr <SPNODE> next;
	volatile bool removed;
	SPNODE() : v(-1), next(nullptr), removed(false) {}
	SPNODE(int x) : v(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

class LF_NODE;
class LF_PTR {
	unsigned long long next;
public:
	LF_PTR() : next(0) {}
	LF_PTR(bool marking, LF_NODE* ptr)
	{
		next = reinterpret_cast<unsigned long long>(ptr);
		if (true == marking) next = next | 1;
	}
	LF_NODE* get_ptr()
	{
		return reinterpret_cast<LF_NODE*>(next & 0xFFFFFFFFFFFFFFFE);
	}
	bool get_removed()
	{
		return (next & 1) == 1;
	}
	LF_NODE* get_ptr_mark(bool* removed)
	{
		unsigned long long cur_next = next;
		*removed = (cur_next & 1) == 1;
		return reinterpret_cast<LF_NODE*>(cur_next & 0xFFFFFFFFFFFFFFFE);
	}
	bool CAS(LF_NODE* o_ptr, LF_NODE* n_ptr, bool o_mark, bool n_mark)
	{
		unsigned long long o_next = reinterpret_cast<unsigned long long>(o_ptr);
		if (true == o_mark) o_next++;
		unsigned long long n_next = reinterpret_cast<unsigned long long>(n_ptr);
		if (true == n_mark) n_next++;
		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_uint64_t*>(&next), &o_next, n_next);
	}
	bool is_removed()
	{
		return (next & 1) == 1;
	}
};

class LF_NODE {
public:
	int v;
	LF_PTR next;
	LF_NODE() : v(-1), next(false, nullptr) {}
	LF_NODE(int x) : v(x), next(false, nullptr) {}
	LF_NODE(int x, LF_NODE* ptr) : v(x), next(false, ptr) {}
};

class SET {
	NODE head, tail;
	null_mutex ll;
public:
	SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}
	bool ADD(int x)
	{
		NODE* prev = &head;
		ll.lock();
		NODE* curr = prev->next;
		while (curr->v < x) {
			prev = curr;
			curr = curr->next;
		}
		if (curr->v != x) {
			NODE* node = new NODE{ x };
			node->next = curr;
			prev->next = node;
			ll.unlock();
			return true;
		}
		else
		{
			ll.unlock();
			return false;
		}
	}

	bool REMOVE(int x)
	{
		NODE* prev = &head;
		ll.lock();
		NODE* curr = prev->next;
		while (curr->v < x) {
			prev = curr;
			curr = curr->next;
		}
		if (curr->v != x) {
			ll.unlock();
			return false;
		}
		else {
			prev->next = curr->next;
			delete curr;
			ll.unlock();
			return true;
		}
	}

	bool CONTAINS(int x)
	{
		NODE* prev = &head;
		ll.lock();
		NODE* curr = prev->next;
		while (curr->v < x) {
			prev = curr;
			curr = curr->next;
		}
		bool res = (curr->v == x);
		ll.unlock();
		return res;
	}
	void print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};

class F_SET {
	NODE head, tail;
public:
	F_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}
	bool ADD(int x)
	{
		head.lock();
		NODE* prev = &head;
		NODE* curr = prev->next;
		curr->lock();
		while (curr->v < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->v != x) {
			NODE* node = new NODE{ x };
			node->next = curr;
			prev->next = node;
			curr->unlock();
			prev->unlock();
			return true;
		}
		else
		{
			curr->unlock();
			prev->unlock();
			return false;
		}
	}

	bool REMOVE(int x)
	{
		head.lock();
		NODE* prev = &head;
		NODE* curr = prev->next;
		curr->lock();
		while (curr->v < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		if (curr->v != x) {
			curr->unlock();
			prev->unlock();
			return false;
		}
		else {
			prev->next = curr->next;
			curr->unlock();
			prev->unlock();
			delete curr;
			return true;
		}
	}

	bool CONTAINS(int x)
	{
		head.lock();
		NODE* prev = &head;
		NODE* curr = prev->next;
		curr->lock();
		while (curr->v < x) {
			prev->unlock();
			prev = curr;
			curr = curr->next;
			curr->lock();
		}
		bool res = (curr->v == x);
		curr->unlock();
		prev->unlock();
		return res;
	}
	void print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};

class O_SET {
	NODE head, tail;
public:
	O_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}
	bool validate(NODE* prev, NODE* curr)
	{
		NODE* n = &head;
		while (n->v <= prev->v) {
			if (n == prev)
				return prev->next == curr;
			n = n->next;
		}
		return false;
	}

	bool ADD(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					NODE* node = new NODE{ x };
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					prev->next = curr->next;
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v == x) {
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	void print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};

class L_SET {
	NODE head, tail;
public:
	L_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.next = &tail;
		tail.next = nullptr;
	}

	~L_SET()
	{
		clear();
	}

	bool validate(NODE* prev, NODE* curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}

	bool ADD(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					NODE* node = new NODE{ x };
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			NODE* prev = &head;
			NODE* curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->removed = true;
					prev->next = curr->next;
					curr->unlock();
					prev->unlock();
					//delete curr;
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		NODE* curr = &head;
		while (curr->v < x)
			curr = curr->next;
		return (x == curr->v) && (false == curr->removed);
	}

	void print20()
	{
		NODE* p = head.next;
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		NODE* p = head.next;
		while (p != &tail) {
			NODE* t = p;
			p = p->next;
			delete t;
		}
		head.next = &tail;
	}
};

class LSP_SET {
	shared_ptr <SPNODE> head, tail;
public:
	LSP_SET()
	{
		head = make_shared<SPNODE>(0x80000000);
		tail = make_shared<SPNODE>(0x7FFFFFFF);
		head->next = tail;
	}

	~LSP_SET()
	{
		//clear();
	}

	bool validate(const shared_ptr<SPNODE>& prev, const shared_ptr<SPNODE>& curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}

	bool ADD(int x)
	{
		while (true) {
			shared_ptr<SPNODE> prev = head;
			shared_ptr<SPNODE> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					auto node = make_shared<SPNODE>(x);
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			auto prev = head;
			auto curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->removed = true;
					prev->next = curr->next;
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		auto curr = head;
		while (curr->v < x)
			curr = curr->next;
		return (x == curr->v) && (false == curr->removed);
	}

	void print20()
	{
		auto p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		head->next = tail;
	}
};

#if 0
class LASP_SET {
	shared_ptr <SPNODE> head, tail;
public:
	LASP_SET()
	{
		head = make_shared<SPNODE>(0x80000000);
		tail = make_shared<SPNODE>(0x7FFFFFFF);
		head->next = tail;
	}

	~LASP_SET()
	{
		//clear();
	}

	bool validate(const shared_ptr<SPNODE>& prev, const shared_ptr<SPNODE>& curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}

	bool ADD(int x)
	{
		while (true) {
			shared_ptr<SPNODE> prev = head;
			shared_ptr<SPNODE> curr = std::atomic_load(&prev->next);
			while (curr->v < x) {
				prev = curr;
				curr = atomic_load(&curr->next);
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					auto node = make_shared<SPNODE>(x);
					node->next = curr;
					atomic_exchange(&prev->next, node);
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			auto prev = head;
			auto curr = atomic_load(&prev->next);
			while (curr->v < x) {
				prev = curr;
				curr = atomic_load(&curr->next);
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->removed = true;
					atomic_exchange(&prev->next, curr->next);
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		auto curr = head;
		while (curr->v < x)
			curr = atomic_load(&curr->next);
		return (x == curr->v) && (false == curr->removed);
	}

	void print20()
	{
		auto p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		head->next = tail;
	}
};
#endif

namespace JNH {
	using namespace std;

	template <class T>

	struct atomic_shared_ptr {
	private:
		mutable mutex m_lock;
		shared_ptr<T> m_ptr;
	public:
		bool is_lock_free() const noexcept
		{
			return false;
		}

		void store(shared_ptr<T> sptr, memory_order = memory_order_seq_cst) noexcept
		{
			m_lock.lock();
			m_ptr = sptr;
			m_lock.unlock();
		}

		shared_ptr<T> load(memory_order = memory_order_seq_cst) const noexcept
		{
			m_lock.lock();
			shared_ptr<T> t = m_ptr;
			m_lock.unlock();
			return t;
		}

		operator shared_ptr<T>() const noexcept
		{
			m_lock.lock();
			shared_ptr<T> t = m_ptr;
			m_lock.unlock();
			return t;
		}

		shared_ptr<T> exchange(shared_ptr<T> sptr, memory_order = memory_order_seq_cst) noexcept
		{
			m_lock.lock();
			shared_ptr<T> t = m_ptr;
			m_ptr = sptr;
			m_lock.unlock();
			return t;
		}

		bool compare_exchange_strong(shared_ptr<T>& expected_sptr, shared_ptr<T> new_sptr, memory_order, memory_order) noexcept
		{
			bool success = false;
			m_lock.lock();
			shared_ptr<T> t = m_ptr;
			if (m_ptr.get() == expected_sptr.get()) {
				m_ptr = new_sptr;
				success = true;
			}
			expected_sptr = m_ptr;
			m_lock.unlock();
		}

		bool compare_exchange_weak(shared_ptr<T>& expected_sptr, shared_ptr<T> target_sptr, memory_order, memory_order) noexcept
		{
			return compare_exchange_strong(expected_sptr, target_sptr, memory_order);
		}

		atomic_shared_ptr() noexcept = default;

		constexpr atomic_shared_ptr(shared_ptr<T> sptr) noexcept
		{
			m_lock.lock();
			m_ptr = sptr;
			m_lock.unlock();
		}

		shared_ptr<T> operator=(shared_ptr<T> sptr) noexcept
		{
			m_lock.lock();
			m_ptr = sptr;
			m_lock.unlock();
			return sptr;
		}

		void reset()
		{
			m_lock.lock();
			m_ptr = nullptr;
			m_lock.unlock();
		}

		atomic_shared_ptr(const atomic_shared_ptr& rhs)
		{
			store(rhs);
		}

		atomic_shared_ptr& operator=(const atomic_shared_ptr& rhs)
		{
			store(rhs);
			return *this;
		}

		shared_ptr<T>& operator->() {
			std::lock_guard<mutex> tt(m_lock);
			return m_ptr;
		}

		template< typename TargetType >
		inline bool operator ==(shared_ptr< TargetType > const& rhs)
		{
			return load() == rhs;
		}

		template< typename TargetType >
		inline bool operator ==(atomic_shared_ptr<TargetType> const& rhs)
		{
			std::lock_guard<mutex> t1(m_lock);
			std::lock_guard<mutex> t2(rhs.m_lock);
			return m_ptr == rhs.m_ptr;
		}
	};

	template <class T> struct atomic_weak_ptr {
	private:
		mutable mutex m_lock;
		weak_ptr<T> m_ptr;
	public:
		bool is_lock_free() const noexcept
		{
			return false;
		}
		void store(weak_ptr<T> wptr, memory_order = memory_order_seq_cst) noexcept
		{
			m_lock.lock();
			m_ptr = wptr;
			m_lock.unlock();
		}
		weak_ptr<T> load(memory_order = memory_order_seq_cst) const noexcept
		{
			m_lock.lock();
			weak_ptr<T> t = m_ptr;
			m_lock.lock();
			return t;
		}
		operator weak_ptr<T>() const noexcept
		{
			m_lock.lock();
			weak_ptr<T> t = m_ptr;
			m_lock.unlock();
			return t;
		}
		weak_ptr<T> exchange(weak_ptr<T> wptr, memory_order = memory_order_seq_cst) noexcept
		{
			m_lock.lock();
			weak_ptr<T> t = m_ptr;
			m_ptr = wptr;
			m_lock.unlock();
			return t;
		}

		bool compare_exchange_strong(weak_ptr<T>& expected_wptr, weak_ptr<T> new_wptr, memory_order, memory_order) noexcept
		{
			bool success = false;
			lock_guard(m_lock);

			weak_ptr<T> t = m_ptr;
			shared_ptr<T> my_ptr = t.lock();
			if (!my_ptr) return false;
			shared_ptr<T> expected_sptr = expected_wptr.lock();
			if (!expected_wptr) return false;

			if (my_ptr.get() == expected_sptr.get()) {
				success = true;
				m_ptr = new_wptr;
			}
			expected_wptr = t;
			return success;
		}

		bool compare_exchange_weak(weak_ptr<T>& exptected_wptr, weak_ptr<T> new_wptr, memory_order, memory_order) noexcept
		{
			return compare_exchange_strong(exptected_wptr, new_wptr, memory_order);
		}

		atomic_weak_ptr() noexcept = default;

		constexpr atomic_weak_ptr(weak_ptr<T> wptr) noexcept
		{
			m_lock.lock();
			m_ptr = wptr;
			m_lock.unlock();
		}

		atomic_weak_ptr(const atomic_weak_ptr&) = delete;
		atomic_weak_ptr& operator=(const atomic_weak_ptr&) = delete;
		weak_ptr<T> operator=(weak_ptr<T> wptr) noexcept
		{
			m_lock.lock();
			m_ptr = wptr;
			m_lock.unlock();
			return wptr;
		}
		shared_ptr<T> lock() const noexcept
		{
			m_lock.lock();
			shared_ptr<T> sptr = m_ptr.lock();
			m_lock.unlock();
			return sptr;
		}
		void reset()
		{
			m_lock.lock();
			m_ptr.reset();
			m_lock.unlock();
		}
	};
}

class ASPNODE {
	mutex n_lock;
public:
	int v;
	JNH::atomic_shared_ptr <ASPNODE> next;
	volatile bool removed;
	ASPNODE() : v(-1), next(nullptr), removed(false) {}
	ASPNODE(int x) : v(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

class LASP2_SET {
	JNH::atomic_shared_ptr <ASPNODE> head, tail;
public:
	LASP2_SET()
	{
		head = make_shared<ASPNODE>(0x80000000);
		tail = make_shared<ASPNODE>(0x7FFFFFFF);
		head->next = tail;
	}

	~LASP2_SET()
	{
		//clear();
	}

	bool validate(const shared_ptr<ASPNODE>& prev, const shared_ptr<ASPNODE>& curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}

	bool ADD(int x)
	{
		while (true) {
			shared_ptr<ASPNODE> prev = head;
			shared_ptr<ASPNODE> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					auto node = make_shared<ASPNODE>(x);
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			shared_ptr<ASPNODE> prev = head;
			shared_ptr<ASPNODE> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->removed = true;
					prev->next = curr->next;
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		shared_ptr<ASPNODE> curr = head;
		while (curr->v < x)
			curr = curr->next;
		return (x == curr->v) && (false == curr->removed);
	}

	void print20()
	{
		auto p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		head->next = tail;
	}
};

class ASPNODE2 {
	mutex n_lock;
public:
	int v;
	atomic<shared_ptr<ASPNODE2>> next;
	volatile bool removed;
	ASPNODE2() : v(-1), next(nullptr), removed(false) {}
	ASPNODE2(int x) : v(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

// C++20의 atomic shared_ptr 사용
class LASP3_SET {
	atomic<shared_ptr <ASPNODE2>> head, tail;
public:
	LASP3_SET()
	{
		head = make_shared<ASPNODE2>(0x80000000);
		tail = make_shared<ASPNODE2>(0x7FFFFFFF);
		shared_ptr<ASPNODE2> t = head;
		shared_ptr<ASPNODE2> t2 = tail;
		t->next = t2;
	}

	~LASP3_SET()
	{
		//clear();
	}

	bool validate(const shared_ptr<ASPNODE2>& prev, const shared_ptr<ASPNODE2>& curr)
	{
		shared_ptr<ASPNODE2> t = prev->next;
		return (prev->removed == false) && (curr->removed == false) && (t == curr);
	}

	bool ADD(int x)
	{
		while (true) {
			shared_ptr<ASPNODE2> prev = head;
			shared_ptr<ASPNODE2> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					auto node = make_shared<ASPNODE2>(x);
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			shared_ptr<ASPNODE2> prev = head;
			shared_ptr<ASPNODE2> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->removed = true;
					shared_ptr<ASPNODE2> t = prev;
					shared_ptr<ASPNODE2> t2 = curr->next;
					t->next = t2;
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		shared_ptr<ASPNODE2> curr = head;
		while (curr->v < x)
			curr = curr->next;
		return (x == curr->v) && (false == curr->removed);
	}

	void print20()
	{
		shared_ptr<ASPNODE2> t = head;
		shared_ptr<ASPNODE2> t2 = tail;
		shared_ptr<ASPNODE2> p = t->next;
		for (int i = 0; i < 20; ++i) {
			if (p == t2) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		shared_ptr<ASPNODE2> t = head;
		shared_ptr<ASPNODE2> t2 = tail;
		t->next = t2;
	}
};


// Lock Free 버전의 게으른 동기화 구현
#define __LOCK_FREE_SMART_POINTER__
namespace LF {

#ifdef _WIN64
	using _p_size_ = long long;
	using _p_type_ = std::atomic_llong;
#else
	using _p_size_ = long;
	using _p_type_ = std::atomic_long;
#endif

	template<typename Tp>
	class control_block;

	template<typename Tp>
	class shared_ptr;

	template<typename Tp>
	class weak_ptr;

	template<typename Tp>
	class enable_shared_from_this;

	template<typename Tp, typename = void>
	struct _Can_enable_shared : std::false_type
	{};

	template<typename Tp>
	struct _Can_enable_shared<Tp, std::void_t<typename Tp::_Esft_type>>
		: std::is_convertible<std::remove_cv_t<Tp>*, typename Tp::_Esft_type*>::type
	{};


	class RecycleLinkedList
	{
	public:
		class Node
		{
		public:
			Node* volatile next;
			std::atomic_int active;
			void* ctr;

		public:
			Node()
			{
				next = nullptr;
				active = 0;
				ctr = nullptr;
			}

			Node(void* p)
			{
				next = nullptr;
				active = 0;
				ctr = p;
			}

			Node(int i)
			{
				next = nullptr;
				active = 2;
				ctr = nullptr;
			}
			~Node() {}
		};

	private:
		Node* head;

		bool CAS(std::atomic_int* memory, int old_data, int new_data) const
		{
			int old_value = old_data;
			int new_value = new_data;

			return std::atomic_compare_exchange_strong
			(reinterpret_cast<std::atomic_int*>(memory), &old_value, new_value);
		}

		bool CAS(Node* volatile* memory, Node* oldaddr, Node* newaddr)
		{
			_p_size_ old_addr = reinterpret_cast<_p_size_>(oldaddr);
			_p_size_ new_addr = reinterpret_cast<_p_size_>(newaddr);

			return std::atomic_compare_exchange_strong
			(reinterpret_cast<volatile _p_type_*>(memory), &old_addr, new_addr);
		}

	public:

		RecycleLinkedList()
		{
			head = new Node(2);
		}

		~RecycleLinkedList()
		{
			Node* curr = head->next;
			Node* next;

			while (curr != nullptr)
			{
				next = curr->next;
				delete curr->ctr;
				delete curr;
				curr = next;
			}

			delete head;
		}

		void* Alloc()
		{
			Node* curr = head->next;
			void* ret;

			while (curr != nullptr)
			{
				if (curr->active == 1)
					if (true == CAS(&(curr->active), 1, 2))
					{
						ret = curr->ctr;
						curr->active = 0;

						return ret;
					}

				curr = curr->next;
			}

			return nullptr;
		}

		void Add_FreeListNode(void* p)
		{
			Node* pred = head;
			Node* curr;

			while (true)
			{
				if (pred->next == nullptr)
					break;

				curr = pred->next;

				if (curr->active == 0)
				{
					if (true == CAS(&(curr->active), 0, 2))
					{
						curr->ctr = p;
						curr->active = 1;
						return;
					}
				}

				pred = pred->next;
			}

			Node* n = new Node(p);

			while (true)
			{
				if (pred->next == nullptr)
					if (true == CAS(&pred->next, nullptr, n))
						return;
				pred = pred->next;
			}
		}
	};

#ifdef __LOCK_FREE_SMART_POINTER__
#define __EXT__
#else
#define __EXT__ extern
#endif
	__EXT__ LF::RecycleLinkedList RLL;

	template<typename Tp>
	class control_block {

	private:

		mutable std::atomic_int use_count;
		mutable std::atomic_int	weak_count;

		Tp* ptr;

		template<typename Tp, typename... Args>
		friend shared_ptr<Tp> make_shared(Args&&... _Args);

		control_block(const control_block&) = delete;
		control_block& operator=(const control_block&) = delete;

		bool CAS(std::atomic_int* memory, int old_data, int new_data) const
		{
			int old_value = old_data;
			int new_value = new_data;

			return std::atomic_compare_exchange_strong
			(reinterpret_cast<std::atomic_int*>(memory), &old_value, new_value);
		}

	public:
		control_block() = delete;

		control_block(Tp* other)
			: ptr(other), use_count(1), weak_count(1)
		{}

		virtual ~control_block()
		{}

		void enable_min_weak_count()
		{
			weak_count--;
		}

		void init(Tp* new_Tp)
		{
			ptr = new_Tp;
			weak_count = 1;
			use_count = 1;
		}

		control_block<Tp>* add_use_count()
		{
			int pred_value;

			while (true)
			{
				pred_value = use_count;

				if (pred_value > 0)
				{
					if (true == CAS(&use_count, pred_value, pred_value + 1))
						return this;

					else
						continue;
				}

				else
				{
					return nullptr;
				}
			}
		}

		void destroy(std::true_type)
		{
			delete ptr;
		}

		void destroy(std::false_type)
		{
			delete ptr;

			int curr_weak_count = weak_count;

			if (curr_weak_count == 1)
			{
				if (true == CAS(&weak_count, curr_weak_count, curr_weak_count - 1))
					LF::RLL.Add_FreeListNode(this);
			}
		}

		void release() noexcept
		{
			int curr_use_count;

			while (true)
			{
				curr_use_count = use_count;

				if (true == CAS(&use_count, curr_use_count, curr_use_count - 1))
				{
					if (curr_use_count == 1)
						destroy(std::bool_constant<std::conjunction_v<_Can_enable_shared<Tp>>>{});

					return;
				}

				else
					continue;
			}
		}

		Tp* get()
		{
			int curr_use_count = use_count;

			if (curr_use_count > 0)
				return ptr;

			else
				return nullptr;
		}

		int get_use_count()
		{
			int use_cnt = use_count;

			return use_cnt;
		}

		control_block<Tp>* add_weak_count() noexcept
		{
			int pred_value;

			while (true)
			{
				pred_value = weak_count;

				if (pred_value > 0)
					if (true == CAS(&weak_count, pred_value, pred_value + 1))
						return this;

					else
					{
						return nullptr;
					}
			}
		}

		void weak_release() noexcept
		{
			int curr_weak_count;

			while (true)
			{
				curr_weak_count = weak_count;

				if (true == CAS(&weak_count, curr_weak_count, curr_weak_count - 1))
				{
					if (curr_weak_count == 1)
						LF::RLL.Add_FreeListNode(this);
					return;
				}
			}
		}
	};

	template <typename Tp>
	class shared_ptr {

	private:

		control_block<Tp>* ctr;

		friend class control_block<Tp>;
		friend class weak_ptr<Tp>;

		template<typename... Args>
		friend shared_ptr<Tp> make_shared(Args&&... _Args);

		template<typename _Tp>
		friend bool operator==(const shared_ptr<_Tp>&, const shared_ptr<_Tp>&);

		void _Enable_shared_from_this(Tp* _Ptr, std::true_type)
		{
			_Ptr->Wptr.Set_enable_shared(*this);
		}

		void _Enable_shared_from_this(Tp*, std::false_type)
		{}

	public:
		void _Set_ctr_and_enable_shared(Tp* new_ptr, control_block<Tp>* new_ctr)
		{
			ctr = new_ctr;
			_Enable_shared_from_this(new_ptr, std::bool_constant<std::conjunction_v<_Can_enable_shared<Tp>>>{});
		}

	private:
		bool CAS(control_block<Tp>** memory, control_block<Tp>* oldaddr, control_block<Tp>* newaddr)
		{
			_p_size_ old_addr = reinterpret_cast<_p_size_>(oldaddr);
			_p_size_ new_addr = reinterpret_cast<_p_size_>(newaddr);

			return std::atomic_compare_exchange_strong
			(reinterpret_cast<_p_type_*>(memory), &old_addr, new_addr);
		}

		control_block<Tp>* add_shared_copy() const
		{
			control_block<Tp>* pred;
			control_block<Tp>* curr;
			control_block<Tp>* get_ctr;

			while (true) {
				pred = ctr;
				if (pred == nullptr)
					return nullptr;

				get_ctr = pred->add_use_count();

				curr = ctr;

				if (get_ctr == curr)
					return curr;

				else
				{
					if (get_ctr != nullptr)
						pred->release();
				}
			}
		}

		control_block<Tp>* add_weak_copy() const
		{
			control_block<Tp>* pred;
			control_block<Tp>* curr;
			control_block<Tp>* get_ctr;

			while (true) {
				pred = ctr;
				if (pred == nullptr)
					return nullptr;

				get_ctr = pred->add_weak_count();

				curr = ctr;

				if (get_ctr == curr)
					return curr;

				else
				{
					if (get_ctr != nullptr)
						pred->weak_release();
				}
			}
		}
	public:
		Tp* get() const
		{
			control_block<Tp>* curr_ctr = ctr;

			if (curr_ctr != nullptr)
				return curr_ctr->get();
			else
				return nullptr;
		}

	public:

		shared_ptr()
			: ctr(nullptr)
		{}

		shared_ptr(nullptr_t)
			: shared_ptr()
		{}

		shared_ptr(Tp* other)
			: ctr(new control_block<Tp>(other))
		{}

		shared_ptr(const shared_ptr& other)
		{
			ctr = other.add_shared_copy();
		}

		shared_ptr(const weak_ptr<Tp>& other)
		{
			ctr = other.add_shared_copy();
		}

		shared_ptr& operator=(nullptr_t)
		{
			control_block<Tp>* pred_ctr;

			while (true)
			{
				pred_ctr = ctr;

				if (pred_ctr == nullptr)
					return *this;

				if (true == CAS(&ctr, pred_ctr, nullptr))
				{
					pred_ctr->release();
					return *this;
				}
			}
		}

		shared_ptr& operator=(const shared_ptr& other)
		{
			control_block<Tp>* pred_ctr;
			control_block<Tp>* other_ctr;

			while (true)
			{
				pred_ctr = ctr;
				other_ctr = other.add_shared_copy();

				if (other_ctr == nullptr)
				{
					if (true == CAS(&ctr, pred_ctr, nullptr))
					{
						if (pred_ctr != nullptr)
							pred_ctr->release();
						return *this;
					}

					continue;
				}

				if (other_ctr == pred_ctr)
				{
					other_ctr->release();
					return *this;
				}

				if (true == CAS(&ctr, pred_ctr, other_ctr))
				{
					if (pred_ctr != nullptr)
						pred_ctr->release();

					return *this;
				}
				else
				{
					other_ctr->release();
					continue;
				}
			}
		}

	private:
		shared_ptr& operator=(const weak_ptr<Tp>& other)
		{
			control_block<Tp>* pred_ctr;
			control_block<Tp>* other_ctr;

			while (true)
			{
				pred_ctr = ctr;
				other_ctr = other.add_shared_copy();

				if (other_ctr == nullptr)
				{
					if (true == CAS(&ctr, pred_ctr, nullptr))
					{
						if (pred_ctr != nullptr)
							pred_ctr->release();
						return *this;
					}

					continue;
				}

				if (other_ctr == pred_ctr)
				{
					other_ctr->release();
					return *this;
				}

				if (true == CAS(&ctr, pred_ctr, other_ctr))
				{
					if (pred_ctr != nullptr)
						pred_ctr->release();

					return *this;
				}
				else
				{
					other_ctr->release();
					continue;
				}
			}
		}

	public:

		~shared_ptr()
		{
			control_block<Tp>* curr_ctr = ctr;

			if (curr_ctr != nullptr)
				curr_ctr->release();

		}

		Tp& operator*()
		{
			return *get();
		}

		Tp* operator->() const
		{
			return get();
		}

		operator bool()
		{
			control_block<Tp>* curr_ctr = get();

			if (curr_ctr != nullptr)
				if (curr_ctr->get_use_count() > 0)
					return true;

			return false;
		}

		int use_count()
		{
			control_block<Tp>* curr_ctr = get();

			if (curr_ctr != nullptr)
				return ctr->get_use_count();

			return 0;
		}

		void reset()
		{
			control_block<Tp>* pred_ctr;

			while (true) {
				pred_ctr = get();

				if (pred_ctr == nullptr)
					return;

				if (true == CAS(&ctr, pred_ctr, nullptr)) {
					pred_ctr->release();
					return;
				}
			}
		}
	};

	template<typename _Tp>
	bool operator==(const shared_ptr<_Tp>& __a, const shared_ptr<_Tp>& __b)
	{
		return __a.get() == __b.get();
	}

	template<typename _Tp>
	bool operator==(const shared_ptr<_Tp>& __a, nullptr_t)
	{
		return (__a.get() == nullptr);
	}

	template<typename _Tp>
	bool operator==(nullptr_t, const shared_ptr<_Tp>& __a)
	{
		return (__a.get() == nullptr);
	}

	template<typename _Tp>
	bool operator!=(const shared_ptr<_Tp>& __a, const shared_ptr<_Tp>& __b)
	{
		return !(operator==(__a, __b));
	}

	template<typename _Tp>
	bool operator!=(const shared_ptr<_Tp>& __a, nullptr_t)
	{
		return (__a.get() != nullptr);
	}

	template<typename _Tp>
	bool operator!=(nullptr_t, const shared_ptr<_Tp>& __a)
	{
		return (__a.get() != nullptr);
	}

	template<class _Elem, class _Traits, class _Ty>
	std::basic_ostream<_Elem, _Traits>& operator<<(std::basic_ostream<_Elem, _Traits>& _Out, const shared_ptr<_Ty>& _Px)
	{
		return (_Out << _Px.get());
	}

	template<typename _Tp, typename... Args>
	shared_ptr<_Tp> make_shared(Args&&... _Args)
	{
		_Tp* new_Tp = new _Tp(std::forward<Args>(_Args)...);

		void* get_control_block = LF::RLL.Alloc();
		control_block<_Tp>* new_ctr;

		if (get_control_block != nullptr)
		{
			new_ctr = reinterpret_cast<control_block<_Tp>*>(get_control_block);
			new_ctr->init(new_Tp);
		}

		else
		{
			new_ctr = new control_block<_Tp>(new_Tp);
		}

		shared_ptr<_Tp> _Ret;
		_Ret._Set_ctr_and_enable_shared(new_Tp, new_ctr);

		return _Ret;
	}

	template<typename Tp>
	class weak_ptr {
	private:

		control_block<Tp>* ctr;

		friend class control_block<Tp>;
		friend class shared_ptr<Tp>;

		friend class enable_shared_from_this<Tp>;

		control_block<Tp>* add_shared_copy() const
		{
			control_block<Tp>* pred;
			control_block<Tp>* curr;
			control_block<Tp>* get_ctr;

			while (true) {
				pred = ctr;
				if (pred == nullptr)
					return nullptr;

				get_ctr = pred->add_use_count();

				curr = ctr;

				if (get_ctr == curr)
					return curr;

				else
				{
					if (get_ctr != nullptr)
						pred->release();
				}
			}
		}

		control_block<Tp>* add_weak_copy() const
		{
			control_block<Tp>* pred;
			control_block<Tp>* curr;
			control_block<Tp>* get_ctr;

			while (true) {
				pred = ctr;
				if (pred == nullptr)
					return nullptr;

				get_ctr = pred->add_weak_count();

				curr = ctr;

				if (get_ctr == curr)
					return curr;

				else
				{
					if (get_ctr != nullptr)
						pred->weak_release();
				}
			}
		}

		bool CAS(control_block<Tp>** memory, control_block<Tp>* oldaddr, control_block<Tp>* newaddr)
		{
			_p_size_ old_addr = reinterpret_cast<_p_size_>(oldaddr);
			_p_size_ new_addr = reinterpret_cast<_p_size_>(newaddr);

			return std::atomic_compare_exchange_strong
			(reinterpret_cast<_p_type_*>(memory), &old_addr, new_addr);
		}

		void Set_enable_shared(const shared_ptr<Tp>& other)
		{
			ctr = other.ctr;
		}

	public:
		Tp* get() const
		{
			control_block<Tp>* curr_ctr = ctr;

			if (curr_ctr != nullptr)
				return curr_ctr->get();
			else
				return nullptr;
		}

	public:

		weak_ptr()
			: ctr(nullptr)
		{}

		weak_ptr(nullptr_t)
			: weak_ptr()
		{}

		weak_ptr(const shared_ptr<Tp>& other)
		{
			ctr = other.add_weak_copy();
		}

		weak_ptr(const weak_ptr<Tp>& other)
		{
			ctr = other.add_weak_copy();
		}

		~weak_ptr()
		{
			control_block<Tp>* curr_ctr = ctr;

			if (curr_ctr != nullptr)
				curr_ctr->weak_release();
		}

		weak_ptr& operator=(nullptr_t)
		{
			control_block<Tp>* pred_ctr;

			while (true)
			{
				pred_ctr = ctr;
				if (true == CAS(&ctr, pred_ctr, nullptr))
				{
					if (pred_ctr != nullptr)
						pred_ctr->weak_release();
					return *this;
				}
			}
		}

		weak_ptr& operator=(const shared_ptr<Tp>& other)
		{
			control_block<Tp>* pred_ctr;
			control_block<Tp>* other_ctr;

			while (true)
			{
				pred_ctr = ctr;
				other_ctr = other.add_weak_copy();

				if (other_ctr == nullptr)
				{
					if (true == CAS(&ctr, pred_ctr, nullptr))
					{
						if (pred_ctr != nullptr)
							pred_ctr->weak_release();
						return *this;
					}
					continue;
				}

				if (other_ctr == pred_ctr)
				{
					other_ctr->weak_release();
					return *this;
				}

				if (true == CAS(&ctr, pred_ctr, other_ctr))
				{
					if (pred_ctr != nullptr)
						pred_ctr->weak_release();

					return *this;
				}
				else
				{
					other_ctr->weak_release();
					continue;
				}
			}
		}

		weak_ptr& operator=(const weak_ptr& other)
		{
			control_block<Tp>* pred_ctr;
			control_block<Tp>* other_ctr;

			while (true)
			{
				pred_ctr = ctr;
				other_ctr = other.add_weak_copy();

				if (other_ctr == nullptr)
				{
					if (true == CAS(&ctr, pred_ctr, nullptr))
					{
						if (pred_ctr != nullptr)
							pred_ctr->weak_release();
						return *this;
					}

					continue;
				}

				if (other_ctr == pred_ctr)
				{
					other_ctr->weak_release();
					return *this;
				}

				if (true == CAS(&ctr, pred_ctr, other_ctr))
				{
					if (pred_ctr != nullptr)
						pred_ctr->weak_release();

					return *this;
				}
				else
				{
					other_ctr->weak_release();
					continue;
				}
			}
		}

		shared_ptr<Tp> lock() const
		{
			return shared_ptr<Tp>(this);
		}

		int use_count()
		{
			control_block<Tp>* curr = get();

			if (curr != nullptr)
				return curr->get_use_count();

			return 0;
		}

		bool expired()
		{
			control_block<Tp>* pred_ctr = get();
			if (pred_ctr != nullptr)
				return (pred_ctr->get_use_count() == 0);

			return false;
		}

		void reset()
		{
			control_block<Tp>* pred_ctr;

			while (true) {
				pred_ctr = get();

				if (pred_ctr == nullptr)
					return;

				if (true == CAS(&ctr, pred_ctr, nullptr)) {
					pred_ctr->weak_release();
					return;
				}
			}
		}
	};


	template<typename Tp>
	class enable_shared_from_this {

	private:
		weak_ptr<Tp> Wptr;

		friend shared_ptr<Tp>;
		friend control_block<Tp>;

	public:
		using _Esft_type = enable_shared_from_this;

		shared_ptr<Tp> shared_from_this()
		{
			return (shared_ptr<Tp>(Wptr));
		}

		weak_ptr<Tp> weak_from_this()
		{
			return Wptr;
		}

	protected:
		enable_shared_from_this()
			: Wptr()
		{}

		enable_shared_from_this(const enable_shared_from_this& other)
			: Wptr()
		{}

		enable_shared_from_this& operator=(const enable_shared_from_this&)
		{
			return (*this);
		}

		~enable_shared_from_this() = default;
	};
}

class LFSPNODE {
	mutex n_lock;
public:
	int v;
	LF::shared_ptr<LFSPNODE> next;
	volatile bool removed;
	LFSPNODE() : v(-1), next(nullptr), removed(false) {}
	LFSPNODE(int x) : v(x), next(nullptr), removed(false) {}
	void lock()
	{
		n_lock.lock();
	}
	void unlock()
	{
		n_lock.unlock();
	}
};

class LLFSP_SET {
	LF::shared_ptr <LFSPNODE> head, tail;
public:
	LLFSP_SET()
	{
		head = LF::make_shared<LFSPNODE>(0x80000000);
		tail = LF::make_shared<LFSPNODE>(0x7FFFFFFF);
		head->next = tail;
	}

	~LLFSP_SET()
	{
		//clear();
	}

	bool validate(const LF::shared_ptr<LFSPNODE>& prev, const LF::shared_ptr<LFSPNODE>& curr)
	{
		return (prev->removed == false) && (curr->removed == false) && (prev->next == curr);
	}

	bool ADD(int x)
	{
		while (true) {
			LF::shared_ptr<LFSPNODE> prev = head;
			LF::shared_ptr<LFSPNODE> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					auto node = LF::make_shared<LFSPNODE>(x);
					node->next = curr;
					prev->next = node;
					curr->unlock();
					prev->unlock();
					return true;
				}
				else
				{
					curr->unlock();
					prev->unlock();
					return false;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			LF::shared_ptr<LFSPNODE> prev = head;
			LF::shared_ptr<LFSPNODE> curr = prev->next;
			while (curr->v < x) {
				prev = curr;
				curr = curr->next;
			}
			prev->lock(); curr->lock();
			if (validate(prev, curr)) {
				if (curr->v != x) {
					curr->unlock();
					prev->unlock();
					return false;
				}
				else
				{
					curr->removed = true;
					prev->next = curr->next;
					curr->unlock();
					prev->unlock();
					return true;
				}
			}
			else {
				curr->unlock();
				prev->unlock();
			}
		}
	}

	bool CONTAINS(int x)
	{
		LF::shared_ptr<LFSPNODE> curr = head;
		while (curr->v < x)
			curr = curr->next;
		return (x == curr->v) && (false == curr->removed);
	}

	void print20()
	{
		LF::shared_ptr<LFSPNODE> p = head->next;
		for (int i = 0; i < 20; ++i) {
			if (p == tail) break;
			cout << p->v << ", ";
			p = p->next;
		}
		cout << endl;
	}

	void clear()
	{
		head->next = tail;
	}
};

constexpr long long ADDR_MARK = 0xFFFFFFFFFFFFFFFE;
class LFNODE {
	LFNODE* next;
	bool CAS(long long old_value, long long new_value)
	{
		return atomic_compare_exchange_strong(
			reinterpret_cast<atomic_llong*>(&next),
			&old_value, new_value
		);
	}
public:
	int v;
	LFNODE() : v(-1), next(nullptr) {}
	LFNODE(int x) : v(x), next(nullptr) {}

	LFNODE* get_next()
	{
		return reinterpret_cast<LFNODE*>(
			reinterpret_cast<long long>(next) & ADDR_MARK);
	}

	LFNODE* get_next(bool* removed)
	{
		long long temp = reinterpret_cast<long long>(next);
		*removed = ((temp & 1) == 1);
		return reinterpret_cast<LFNODE*>(temp & ADDR_MARK);
	}

	bool get_mark()
	{
		long long temp = reinterpret_cast<long long>(next);
		return  ((temp & 1) == 1);
	}

	bool CAS(LFNODE* old_node, LFNODE* new_node, bool old_mark, bool new_mark)
	{
		long long old_value = reinterpret_cast<long long>(old_node);
		if (true == old_mark) old_value = old_value | 1;
		long long new_value = reinterpret_cast<long long>(new_node);
		if (true == new_mark) new_value = new_value | 1;
		return CAS(old_value, new_value);
	}

	void set_next(LFNODE* p)
	{
		next = p;
	}
};

class LF_SET {
	LFNODE head, tail;
public:
	LF_SET()
	{
		head.v = 0x80000000;
		tail.v = 0x7FFFFFFF;
		head.set_next(&tail);
	}

	~LF_SET()
	{
		clear();
	}

	void find(int x, LFNODE*& prev, LFNODE*& curr)
	{
	retry:
		prev = &head;
		curr = prev->get_next();
		while (true) {
			bool removed;
			LFNODE* succ = curr->get_next(&removed);
			while (removed == true) {
				if (false == prev->CAS(curr, succ, false, false))
					goto retry;
				curr = succ;
				succ = curr->get_next(&removed);
			}
			if (curr->v >= x) return;
			prev = curr;
			curr = succ;
		}
	}

	bool ADD(int x)
	{
		LFNODE* node = new LFNODE{ x };
		while (true) {
			LFNODE* prev, * curr;
			find(x, prev, curr);
			if (curr->v != x) {
				node->set_next(curr);
				if (true == prev->CAS(curr, node, false, false))
					return true;
			}
			else
			{
				delete node;
				return false;
			}
		}
	}

	bool REMOVE(int x)
	{
		while (true) {
			LFNODE* prev, * curr;
			find(x, prev, curr);
			if (curr->v == x) {
				LFNODE* succ = curr->get_next();
				if (false == curr->CAS(succ, succ, false, true))
					continue;
				prev->CAS(curr, succ, false, false);
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	bool CONTAINS(int x)
	{
		LFNODE* curr = &head;
		bool removed = false;
		while (curr->v < x)
			curr = curr->get_next(&removed);
		return (x == curr->v) && (false == removed);
	}

	void print20()
	{
		LFNODE* p = head.get_next();
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->get_next();
		}
		cout << endl;
	}

	void clear()
	{
		LFNODE* p = head.get_next();
		while (p != &tail) {
			LFNODE* t = p;
			p = p->get_next();
			delete t;
		}
		head.set_next(&tail);
	}
};

enum METHOD_TYPE { OP_ADD, OP_REMOVE, OP_CONTAINS, OP_CLEAR, OP_EMPTY, OP_GET20 };

struct Invocation {
	METHOD_TYPE  m_op;
	int		v;
};

struct Response {
	bool res;
	std::vector<int> get20;
};

class SeqObject_set {
public:
	std::set<int> m_set;
	Response apply(Invocation& inv)
	{
		Response res;
		switch (inv.m_op) {
		case OP_ADD: res.res = (m_set.count(inv.v) == 0);
			if (res.res == true) m_set.insert(inv.v);
			break;

		case OP_REMOVE: res.res = (m_set.count(inv.v) == 1);
			if (res.res == true) m_set.erase(inv.v);
			break;
		case OP_CONTAINS: res.res = (m_set.count(inv.v) == 1);
			break;
		case OP_CLEAR:m_set.clear();
			break;
		case OP_EMPTY: res.res = m_set.empty();
			break;
		case OP_GET20: {
			int count = 20;
			for (auto v : m_set) {
				res.get20.push_back(v);
				if (count-- == 0) break;
			}
			break;
		}
		default:
			cout << "Unknown Method!!\n";
			exit(-1);
		}
		return res;
	}
};

struct UNODE;
class CONSENSUS {
	UNODE* ptr;
public:
	CONSENSUS() : ptr(nullptr) {}
	void CAS(UNODE* old_p, UNODE* new_p)
	{
		atomic_compare_exchange_strong(
			reinterpret_cast<atomic_llong*>(&ptr),
			reinterpret_cast<long long*>(&old_p),
			reinterpret_cast<long long>(new_p));
	}
	UNODE* decide(UNODE* v)
	{
		CAS(nullptr, v);
		return ptr;
	}
};

struct UNODE
{
	Invocation inv;
	UNODE* next;
	CONSENSUS decide_next;
	volatile int seq;
	UNODE() : next(nullptr), seq(0) {}
	UNODE(Invocation& v) : next(nullptr), inv(v), seq(0) {}
};

class STD_SET {
	SeqObject_set std_set;
public:
	bool ADD(int x)
	{
		Invocation inv{ OP_ADD, x };
		Response a = std_set.apply(inv);
		return a.res;
	}

	bool REMOVE(int x)
	{
		Invocation inv{ OP_REMOVE, x };
		Response a = std_set.apply(inv);
		return a.res;
	}

	bool CONTAINS(int x)
	{
		Invocation inv{ OP_CONTAINS, x };
		Response a = std_set.apply(inv);
		return a.res;
	}

	void print20()
	{
		Invocation inv{ OP_GET20, 0 };
		Response a = std_set.apply(inv);
		for(auto v : a.get20) 
			cout << v << ", ";
		cout << endl;
	}

	void clear()
	{
		Invocation inv{ OP_CLEAR, 0 };
		Response a = std_set.apply(inv);
	}
};

thread_local int tl_id;

int Thread_id()
{
	return tl_id;
}

class LF_UNIV_STD_SET {
private:
	UNODE* head[MAX_THREADS];
	UNODE tail;
public:
	LF_UNIV_STD_SET() {
		tail.seq = 0;
		for (int i = 0; i < MAX_THREADS; ++i) head[i] = &tail;
	}
	UNODE* max_node()
	{
		UNODE* max_n = head[0];
		for (int i = 1; i < MAX_THREADS; ++i)
			if (head[i]->seq > max_n->seq) max_n = head[i];
		return max_n;
	}
	Response apply(Invocation invoc) {
		int  i = Thread_id();
		UNODE *prefer = new UNODE(invoc);
		while (prefer->seq == 0) {
			UNODE* before = max_node();
			UNODE* after = before->decide_next.decide(prefer);
			before->next = after; after->seq = before->seq + 1;
			head[i] = after;
		}
		SeqObject_set myObject;
		UNODE* current = tail.next;
		while (current != prefer) {
			myObject.apply(current->inv);
			current = current->next;
		}
		return myObject.apply(current->inv);
	}
};

class LF_STD_SET {
	LF_UNIV_STD_SET std_set;
public:
	bool ADD(int x)
	{
		Invocation inv{ OP_ADD, x };
		Response a = std_set.apply(inv);
		return a.res;
	}

	bool REMOVE(int x)
	{
		Invocation inv{ OP_REMOVE, x };
		Response a = std_set.apply(inv);
		return a.res;
	}

	bool CONTAINS(int x)
	{
		Invocation inv{ OP_CONTAINS, x };
		Response a = std_set.apply(inv);
		return a.res;
	}

	void print20()
	{
		Invocation inv{ OP_GET20, 0 };
		Response a = std_set.apply(inv);
		for (auto v : a.get20)
			cout << v << ", ";
		cout << endl;
	}

	void clear()
	{
		Invocation inv{ OP_CLEAR, 0 };
		Response a = std_set.apply(inv);
	}
};


constexpr int TOP_LEVEL = 9;

class SKNODE {
public:
	int v;
	SKNODE* volatile next[TOP_LEVEL +1];
	int top_level;
	SKNODE() : v(-1), top_level(0) 
	{
		for (auto& n : next) n = nullptr;
	}
	SKNODE(int x, int top) : v(x), top_level(top)
	{
		for (auto& n : next) n = nullptr;
	}
};

class C_SKSET {
	SKNODE head{ 0x80000000, TOP_LEVEL };
	SKNODE tail{ 0x7FFFFFFF, TOP_LEVEL };
	mutex ll;
public:
	C_SKSET()
	{
		for (auto& n : head.next)
			n = &tail;
	}

	void Find(int x, SKNODE *prev[], SKNODE *curr[])
	{
		for (int cl = TOP_LEVEL; cl >= 0; --cl) {
			if (cl == TOP_LEVEL)
				prev[cl] = &head;
			else
				prev[cl] = prev[cl + 1];
			curr[cl] = prev[cl]->next[cl];
			while (curr[cl]->v < x) {
				prev[cl] = curr[cl];
				curr[cl] = curr[cl]->next[cl];
			}
		}
	}

	bool ADD(int x)
	{
		SKNODE* prev[TOP_LEVEL + 1], * curr[TOP_LEVEL + 1];
		ll.lock();
		Find(x, prev, curr);
		if (curr[0]->v != x) {
			int level = 0;
			for (level = 0; level < TOP_LEVEL; ++level)
				if (rand() % 2 == 1) break;
			SKNODE* node = new SKNODE{ x, level };
			for (int i = 0; i <= level; ++i) {
				node->next[i] = curr[i];
				prev[i]->next[i] = node;
			}
			ll.unlock();
			return true;
		}
		else
		{
			ll.unlock();
			return false;
		}
	}

	bool REMOVE(int x)
	{
		return false;
	}

	bool CONTAINS(int x)
	{
		return false;
	}
	void print20()
	{
		SKNODE* p = head.next[0];
		for (int i = 0; i < 20; ++i) {
			if (p == &tail) break;
			cout << p->v << ", ";
			p = p->next[0];
		}
		cout << endl;
	}

	void clear()
	{
		SKNODE* p = head.next[0];
		while (p != &tail) {
			SKNODE* t = p;
			p = p->next[0];
			delete t;
		}
		for (auto &n:head.next) 
			n = &tail;
	}
};

//SET my_set;   // 성긴 동기화
//F_SET my_set;   // 세밀한 동기화
//O_SET my_set;	// 낙천적 동기화
//L_SET my_set;	// 게으른 동기화
//
//LASP3_SET my_set;
//LLFSP_SET my_set;
//LF_STD_SET my_set;
//LF_SET my_set;
//L_SET_SP my_set; // 게으른 동기화 shared_ptr 구현
//LF_SET my_set;
//C_SKLIST my_set;
//LF_SKLIST my_set;

#define MY_SET  C_SKSET

class HISTORY {
public:
	int op;
	int i_value;
	bool o_value;
	HISTORY(int o, int i, bool re) : op(o), i_value(i), o_value(re) {}
};

constexpr int RANGE = 1000;
constexpr int LOOP = 4000000;


void worker(MY_SET* my_set, vector<HISTORY>* history, int num_threads, int thread_id)
{
	tl_id = thread_id;
	for (int i = 0; i < LOOP / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % RANGE;
			my_set->ADD(v);
			break;
		}
		case 1: {
			int v = rand() % RANGE;
			my_set->REMOVE(v);
			break;
		}
		case 2: {
			int v = rand() % RANGE;
			my_set->CONTAINS(v);
			break;
		}
		}
	}
}

void worker_check(MY_SET* my_set, vector<HISTORY>* history, int num_threads, int thread_id)
{
	tl_id = thread_id;
	for (int i = 0; i < LOOP / num_threads; ++i) {
		int op = rand() % 3;
		switch (op) {
		case 0: {
			int v = rand() % RANGE;
			history->emplace_back(0, v, my_set->ADD(v));
			break;
		}
		case 1: {
			int v = rand() % RANGE;
			history->emplace_back(1, v, my_set->REMOVE(v));
			break;
		}
		case 2: {
			int v = rand() % RANGE;
			history->emplace_back(2, v, my_set->CONTAINS(v));
			break;
		}
		}
	}
}

void check_history(MY_SET& my_set, array <vector <HISTORY>, MAX_THREADS>& history, int num_threads)
{
	array <int, RANGE> survive = {};
	cout << "Checking Consistency : ";
	if (history[0].size() == 0) {
		cout << "No history.\n";
		return;
	}
	for (int i = 0; i < num_threads; ++i) {
		for (auto& op : history[i]) {
			if (false == op.o_value) continue;
			if (op.op == 3) continue;
			if (op.op == 0) survive[op.i_value]++;
			if (op.op == 1) survive[op.i_value]--;
		}
	}
	for (int i = 0; i < RANGE; ++i) {
		int val = survive[i];
		if (val < 0) {
			cout << "ERROR. The value " << i << " removed while it is not in the set.\n";
			exit(-1);
		}
		else if (val > 1) {
			cout << "ERROR. The value " << i << " is added while the set already have it.\n";
			exit(-1);
		}
		else if (val == 0) {
			if (my_set.CONTAINS(i)) {
				cout << "ERROR. The value " << i << " should not exists.\n";
				exit(-1);
			}
		}
		else if (val == 1) {
			if (false == my_set.CONTAINS(i)) {
				cout << "ERROR. The value " << i << " shoud exists.\n";
				exit(-1);
			}
		}
	}
	cout << " OK\n";
}

int main()
{
	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		vector <thread> threads;
		array<vector <HISTORY>, MAX_THREADS> history;
		MY_SET my_set;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker_check, &my_set,  &history[i], num_threads, i);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_set.print20();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
		check_history(my_set, history, num_threads);
	}

	cout << "======== SPEED CHECK =============\n";

	for (int num_threads = 1; num_threads <= MAX_THREADS; num_threads *= 2) {
		vector <thread> threads;
		array<vector <HISTORY>, MAX_THREADS> history;
		MY_SET my_set;
		auto start_t = high_resolution_clock::now();
		for (int i = 0; i < num_threads; ++i)
			threads.emplace_back(worker, &my_set, &history[i], num_threads, i);
		for (auto& th : threads)
			th.join();
		auto end_t = high_resolution_clock::now();
		auto exec_t = end_t - start_t;
		auto exec_ms = duration_cast<milliseconds>(exec_t).count();
		my_set.print20();
		cout << num_threads << " Threads.  Exec Time : " << exec_ms << endl;
		check_history(my_set, history, num_threads);
	}
}
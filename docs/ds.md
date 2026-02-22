# Data Structures

## Overview

These data structures provide basic containers for the kernel and subsystems:

- [`LinkedList<T>`](#linkedlist<t>) – singly-linked list with head/tail and basic operations.
- [`HashMap<K, V>`](#hashmap<k,-v,-maxsize>) – hash table with separate chaining using `LinkedList`.
- [`Map<K, V>`](#map<k,-v,-maxsize>) – simple fixed-capacity associative array.
- [`Pair<K, V>`](#pair<k,-v>) – minimal key–value struct used by other structures.

They all allocate from the kernel heap via `new`/`delete` (backed by `MemoryManager`).

---

## LinkedList<T>

Header: `utils/ds/linkedlist.h`

### Structure

```cpp
template <typename T>
class LinkedList {
 public:
  struct Node {
    T data;
    Node* next;
  };
  Node* head;
  Node* tail;
  uint32_t count;
  // ...
};
```

- Singly-linked list with:
  - `head` pointing to the first node.
  - `tail` pointing to the last node.
  - `count` tracking list size.

### Construction / Destruction

```cpp
LinkedList() : head(nullptr), tail(nullptr), count(0) {}

~LinkedList() {
  Node* temp = head;
  while (temp != nullptr) {
    Node* next = temp->next;
    delete temp;
    temp = next;
  }
}
```

- Destructor frees all nodes using `delete`, which in turn uses the kernel heap.

### Operations

- **Append** – O(1) add to end:

  ```cpp
  void Append(T val) {
    Node* node = new Node;
    node->data = val;
    node->next = 0;
    if (head == 0) {
      head = node;
      tail = node;
    } else {
      tail->next = node;
      tail = node;
    }
    count++;
  }
  ```

- **Prepend** – O(1) add to front:

  ```cpp
  void Prepend(T val) {
    Node* node = new Node;
    node->data = val;
    node->next = head;
    head = node;
    if (tail == 0) {
      tail = node;
    }
    count++;
  }
  ```

- **Insert by index** – O(n), insert before a given position:

  ```cpp
  void Insert(T val, uint32_t position) {
    if (position >= count) return;
    Node* node = new Node;
    node->data = val;
    Node* temp = head;
    for (int i = 1; i < position - 1 && temp != nullptr; i++)
      temp = temp->next;
    node->next = temp->next;
    temp->next = node;
    count++;
  }
  ```

- **Insert before node by value** – O(n):

  ```cpp
  void Insert(T val, T positionNode) {
    Node* node = new Node;
    Node* temp = head;
    while (temp != nullptr && temp->next != positionNode) {
      temp = temp->next;
    }
    Node* beforeNode = temp;
    Node* afterNode  = temp->next;

    beforeNode->next = node;
    node->next = afterNode;
    count++;
  }
  ```

  - Assumes `positionNode` is stored as `Node*` in practice; generic `T` use here is fragile and should be used carefully or refactored.

- **Remove by value** – O(n):

  ```cpp
  void Remove(T val) {
    Node* temp = FindOneBefore(val);
    temp->next = val->next;
    val->next = nullptr;
    delete val;
  }
  ```

  - Intended to remove the first occurrence; current implementation assumes `val` is actually a `Node*` in some usages. For generic `T`, this should be revisited.

- **Find** – O(n), returns first node with matching data:

  ```cpp
  Node* Find(T val) {
    Node* temp = head;
    while (temp != nullptr) {
      if (temp->data == val) return temp;
      temp = temp->next;
    }
    return 0;
  }
  ```

- **FindOneBefore** – O(n):

  ```cpp
  Node* FindOneBefore(T val) {
    Node* temp = head;
    while (temp != nullptr) {
      if (temp->next->data == val && temp->next != nullptr)
        return temp;
      temp = temp->next;
    }
    return 0;
  }
  ```

- **RemoveHead** – O(1):

  ```cpp
  void RemoveHead() {
    if (head == 0) return;
    Node* temp = head;
    head = head->next;
    delete temp;
    count--;
    if (head == 0) tail = 0;
  }
  ```

- **Size and emptiness**:

  ```cpp
  uint32_t Size()   { return count; }
  bool isEmpty()    { return count == 0; }
  ```

- **Printing**:

  ```cpp
  void printList() {
    Node* temp = head;
    while (temp != nullptr) {
      printType(temp->data);
      if (temp->next != nullptr) printf(" -> ");
      temp = temp->next;
    }
    printf("\n");
  }

  void printPairList() {
    Node* temp = head;
    if (temp == 0) {
      printf(RED_COLOR, BLACK_COLOR, "[ERROR]: CANNOT PRINT EMPTY LIST\n");
    }
    while (temp != 0) {
      printf("{");
      printType(temp->data.key);
      printf(", ");
      printType(temp->data.value);
      printf("}");
      if (temp->next != 0)
        printf(" -> ");
      temp = temp->next;
    }
    printf("\n");
  }
  ```

  - `printType` is a helper that knows how to print various types.
  - `printPairList` assumes `T` has `.key` and `.value` (e.g., `Pair<K, V>`).

### Notes / Caveats

- This list is singly-linked; no direct back-links apart from special logic in users.
- Some methods (notably `Remove` and `Insert(T, T)`) assume `T` behaves like a node pointer; users should be careful or refactor these methods for strictly value-based semantics.
- All memory allocation uses `new`/`delete` (kernel heap).

---

## HashMap<K, V, MaxSize>

Header: `utils/ds/hashmap.h`

### Structure

- HashMap()
- ~HashMap()
- uint32_t GetBucketIndex(const K& key);
- void Insert(const K& key, const V& value);
- bool Get(const K& key, V& outValue);
- void Remove(const K& key);
- bool Contains(const K& key);
- void GetKeys(LinkedList<K>& dest);
- void GetValues(LinkedList<V>& dest);
- void GetPairs(LinkedList<Pair<K, V>>& dest);

```cpp
template <typename K, typename V, uint32_t MaxSize = 1024>
class HashMap {
 private:
  struct HashNode {
    K key;
    V value;
    bool operator==(const HashNode& other) const {
      return Hasher<K>::isEqual(this->key, other.key);
    }
  };
  LinkedList<HashNode>* buckets[MaxSize];
  uint32_t capacity = MaxSize;
  uint32_t count;
  // ...
};
```

- Separate chaining hash map:
  - `buckets[i]` is a pointer to a `LinkedList<HashNode>` representing a chain.
  - Uses a `Hasher<K>` specialization (from `utils/hash.h`) for hashing and equality.

### Construction / Destruction

```cpp
HashMap() {
  for (int i = 0; i < capacity; i++)
    buckets[i] = 0;
  count = 0;
}

~HashMap() {
  for (int i = 0; i < capacity; i++)
    if (buckets[i] != 0)
      delete buckets[i];
}
```

- Destructor deletes each bucket list, which in turn deletes its nodes.

### Bucket index

```cpp
uint32_t GetBucketIndex(const K& key) {
  return Hasher<K>::Hash(key) % capacity;
}
```

- `Hasher<K>::Hash` must be provided for the key type `K`.

### Insert

```cpp
void Insert(const K& key, const V& value) {
  uint32_t index = GetBucketIndex(key);
  if (buckets[index] == 0)
    buckets[index] = (LinkedList<HashNode>*)new LinkedList<HashNode>;

  HashNode searchNode;
  searchNode.key = *key;
  auto* existingNode = buckets[index]->Find(searchNode);

  if (existingNode != 0) {
    existingNode->data.value = *value; // update existing
    return;
  }

  searchNode.value = value;
  buckets[index]->Append(searchNode);
  count++;
}
```

- Behavior:
  - If the bucket is empty, a `LinkedList<HashNode>` is created.
  - If key already exists, value is updated.
  - Otherwise, a new `HashNode` is appended.

### Lookup

```cpp
bool Get(const K& key, V& outValue) {
  uint32_t index = GetBucketIndex(key);
  if (buckets[index] == 0) return false;

  HashNode searchNode;
  searchNode.key = key;
  auto* existingNode = buckets[index]->Find(searchNode);

  if (existingNode != 0) {
    outValue = existingNode->data.value;
    return true;
  }
  return false;
}
```

- Returns `true` and writes into `outValue` if key exists, `false` otherwise.

### Remove

```cpp
void Remove(const K& key) {
  uint32_t index = GetBucketIndex(key);
  if (buckets[index] == 0) return;
  HashNode dummy;
  dummy.key = key;
  buckets[index]->Remove(dummy);
}
```

- Relies on `LinkedList<HashNode>::Remove` matching by `HashNode::operator==`.

### Other utilities

- **Contains**:

  ```cpp
  bool Contains(const K& key) {
    uint32_t index = GetBucketIndex(key);
    if (buckets[index] == 0) return false;

    HashNode searchNode;
    searchNode.key = key;
    return buckets[index]->Find(searchNode) != 0;
  }
  ```

- **Size / empty**:

  ```cpp
  uint32_t GetSize() { return count; }
  bool isEmpty()     { return count == 0; }
  ```

- **Clear**:

  ```cpp
  void Clear() {
    for (uint32_t i = 0; i < capacity; i++) {
      if (buckets[i] != 0) {
        delete buckets[i];
        buckets[i] = 0;
      }
    }
    count = 0;
  }
  ```

- **GetKeys**:

  ```cpp
  void GetKeys(LinkedList<K>& dest) {
    for (uint32_t i = 0; i < capacity; i++)
      if (buckets[i] != 0) {
        auto* temp = buckets[i]->head;
        while (temp != 0) {
          dest.Append(temp->data.key);
          temp = temp->next;
        }
      }
  }
  ```

- **GetValues**:

  ```cpp
  void GetValues(LinkedList<V>& dest) {
    for (uint32_t i = 0; i < capacity; i++)
      if (buckets[i] != 0) {
        auto* temp = buckets[i]->head;
        while (temp != 0) {
          dest.Append(temp->data.value);
          temp = temp->next;
        }
      }
  }
  ```

- **GetPairs**:

  ```cpp
  void GetPairs(LinkedList<Pair<K, V>>& dest) {
    for (uint32_t i = 0; i < capacity; i++) {
      if (buckets[i] != 0) {
        auto* temp = buckets[i]->head;
        while (temp != 0) {
          Pair<K, V> pair;
          pair.key   = temp->data.key;
          pair.value = temp->data.value;
          dest.Append(pair);
          temp = temp->next;
        }
      }
    }
  }
  ```

### Notes

- Capacity (`MaxSize`) is fixed at compile time; there is no dynamic resizing.
- Complexity:
  - Average operations are O(1) with good hashing and low load factor.
  - Worst case (all keys in one bucket) degrades to O(n) due to list traversal.
- NOTE: (2/21/2026) changed to value/reference based (K&/V&) instead of pointers (K*/V*)

---

## Map<K, V, MaxSize>

Header: `utils/ds/map.h`

### Structure

```cpp
template <typename K, typename V, uint32_t MaxSize = 512>
class Map {
 private:
  struct Pair {
    K key;
    V value;
    bool active;
  };
  Pair pairs[MaxSize];
  uint32_t count;
  // ...
};
```

- Simple associative array backed by a fixed-size array of `Pair { key, value, active }`.

### Construction / Destruction

```cpp
Map() {
  count = 0;
  for (uint32_t i = 0; i < MaxSize; i++)
    pairs[i].active = false;
}

~Map();
```

- Destructor is declared but not defined in the header; implementation can be added if needed (currently no dynamic allocation inside `Map` itself).

### put

```cpp
bool put(K key, V value) {
  // Update if key exists
  for (uint32_t i = 0; i < MaxSize; i++) {
    if (pairs[i].active && pairs[i].key == key) {
      pairs[i].value = value;
      return true;
    }
  }
  if (count >= MaxSize) return false;

  // Insert into first inactive slot
  for (uint32_t i = 0; i < MaxSize; i++) {
    if (!pairs[i].active) {
      pairs[i].key    = key;
      pairs[i].value  = value;
      pairs[i].active = true;
      count++;
      return true;
    }
  }
  return false;
}
```

- Returns:
  - `true` if insertion or update succeeded.
  - `false` if container is full and a new key cannot be added.

### get

```cpp
V get(K key) {
  for (uint32_t i = 0; i < MaxSize; i++) {
    if (pairs[i].active && pairs[i].key == key)
      return pairs[i].value;
  }
  return V();
}
```

- Performs a linear scan.
- Returns default-constructed `V` if not found.

### contains

```cpp
bool contains(K key) {
  for (uint32_t i = 0; i < MaxSize; i++) {
    if (pairs[i].active && pairs[i].key == key)
      return true;
  }
  return false;
}
```

### Accessors

```cpp
Pair* getPair(uint32_t index) {
  if (index < 0 || index >= MaxSize) return 0;
  return &pairs[index];
}

uint32_t size()         { return count; }
uint32_t max_capacity() { return MaxSize; }
```

- `getPair` returns a pointer to the internal `Pair` at an index (even if inactive).

### Notes

- `Map` is useful when:
  - Maximum number of key–value pairs is known and small.
  - Simpler semantics are desired compared to `HashMap`.
- All operations are O(MaxSize), which is acceptable for small sizes.

---

## Pair<K, V>

Header: `utils/ds/pair.h`

```cpp
template <typename K, typename V>
struct Pair {
  K key;
  V value;
};
```

- Minimal utility struct used by:
  - `HashMap::GetPairs`.
  - `LinkedList::printPairList`.
- Does not have comparison operators or methods; equality and hashing are provided elsewhere when needed.

---

## Usage Examples

### LinkedList

```cpp
os::utils::ds::LinkedList<const char*> list;
list.Append("foo");
list.Append("bar");
list.Prepend("head");
list.printList();  // head -> foo -> bar
```

### HashMap

```cpp
os::utils::ds::HashMap<const char*, int> map;
const char* key1 = "cpu_count";
int val1 = 4;
map.Insert(&key1, &val1);

int out = 0;
if (map.Get(&key1, &out)) {
  // out == 4
}
```

### Map

```cpp
os::utils::ds::Map<int, const char*> smallMap;
smallMap.put(1, "one");
smallMap.put(2, "two");

if (smallMap.contains(2)) {
  const char* v = smallMap.get(2);  // "two"
}
```

### HashMap keys/values/pairs

```cpp
os::utils::ds::LinkedList<const char*> keys;
map.GetKeys(&keys);
keys.printList();

os::utils::ds::LinkedList<int> values;
map.GetValues(&values);
values.printList();

os::utils::ds::LinkedList<Pair<const char*, int>> pairs;
map.GetPairs(&pairs);
pairs.printPairList();
```

---

## Invariants and Considerations

- All containers use `new`/`delete` and thus rely on a correctly initialized `MemoryManager`.
- None of the structures are thread-safe; they assume a single‑threaded or externally synchronized context.
- `HashMap`:
  - Capacity is fixed; there is no dynamic expansion.
  - Correctness depends on `Hasher<K>` being well-defined.
- `LinkedList`:
  - Some methods assume that `T` behaves like a pointer or struct with specific fields; future refactors should tighten the API or specialize for particular uses.
- `Map`:
  - Simple and predictable memory usage, but O(MaxSize) operations.

Future improvements to these data structures (e.g., better type safety for `LinkedList::Remove`, thread-safety, iterators) should be reflected in this document.
```

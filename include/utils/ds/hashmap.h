#ifndef __OS__UTILS__DS__HASHMAP_H
#define __OS__UTILS__DS__HASHMAP_H

#include <common/types.h>
#include <memorymanagement.h>
#include <utils/ds/linkedlist.h>
#include <utils/ds/pair.h>
#include <utils/hash.h>


namespace os {
namespace utils {
namespace ds {


template <typename K, typename V, common::uint32_t MaxSize = 1024>
class HashMap {
 private:
  struct HashNode {
    K key;
    V value;
    bool operator==(const HashNode& other) const {
      return Hasher<K>::isEqual(this->key, other.key);
    }
  };
  /** [array of pointers to heads of LinkedList objects,
   * where each index represents a bucket,
   * (bucket points to head of LinkedList) ]
   */
  LinkedList<HashNode>* buckets[MaxSize];

  common::uint32_t capacity = MaxSize;  // maximum number of buckets
  common::uint32_t count;               // current count of elements stored

 public:
  HashMap() {
    for (int i = 0; i < capacity; i++) {
      buckets[i] = 0;
    }
    count = 0;
  }
  ~HashMap() {
    for (int i = 0; i < capacity; i++) {
      if (buckets[i] != 0) {
        delete buckets[i];
      }
    }
  }


  common::uint32_t GetBucketIndex(const K* key) {
    // logic: Hash(key) % capacity;
    return Hasher<K>::Hash(*key) % capacity;
  }


  void Insert(const K* key, const V* value) {
    /* logic:
     * get bucket index
     * check to see if the key already exists in the bucket
     * if so, update the value of the key in that bucket
     * if not, create a new Node and append it to the end of the bucket and increment count
     */
    common::uint32_t index = GetBucketIndex(key);
    // create a LinkedList if it doesnt exist
    if (buckets[index] == 0) {
      buckets[index] = (LinkedList<HashNode>*)new LinkedList<HashNode>;
    }

    // check for collision
    HashNode searchNode;
    searchNode.key = *key;
    typename LinkedList<HashNode>::Node* existingNode = buckets[index]->Find(searchNode);

    if (existingNode != 0) {
      // collisionFound
      existingNode->data.value = *value;
      return;
    }

    // at this point, collision is not found,
    // so, append a new node with K and V
    searchNode.value = *value;
    buckets[index]->Append(searchNode);
    count++;
  }

  bool Get(const K* key, V* outValue) {
    common::uint32_t index = GetBucketIndex(key);
    if (buckets[index] == 0) return false;

    HashNode searchNode;
    searchNode.key = *key;

    typename LinkedList<HashNode>::Node* existingNode = buckets[index]->Find(searchNode);

    if (existingNode != 0) {
      *outValue = existingNode->data.value;
      return true;
    }
    return false;
  }

  void Remove(const K* key) {
    common::uint32_t index = GetBucketIndex(key);
    if (buckets[index] == 0) return;
    HashNode dummy;
    dummy.key = *key;
    buckets[index]->Remove(dummy);
  }

  /**
   * [returns true if the key exists within the HashMap]
   */
  bool Contains(const K* key) {
    common::uint32_t index = GetBucketIndex(key);
    if (buckets[index] == 0) return false;

    HashNode searchNode;
    searchNode.key = *key;
    // if Find returns a non-nullptr, then the key exists within the HashMap
    return buckets[index]->Find(searchNode) != 0;
  }

  /**
   * [returns the total number of key-value pairs in HashMap]
   */
  common::uint32_t GetSize() {
    return count;
  }

  /**
   * [returns true if there are 0 key-value pairs in the HashMap]
   */
  bool isEmpty() {
    return count == 0;
  }

  /**
   * [deletes all buckets and resets HashMap to empty state]
   */
  void Clear() {
    for (common::uint32_t i = 0; i < capacity; i++) {
      if (buckets[i] != 0) {
        delete buckets[i];  // calls LinkedList destructor, freeing all Nodes
        buckets[i] = 0;     // reset bucket head pointer to null
      }
    }
    count = 0;  // reset count
  }

  /**
   * [populates provided LinkedList (dest) with all keys in HashMap],
   * Usage:
   * LinkedList<const char*> myKeys;
   * myMap.GetKeys(&myKeys);
   */
  void GetKeys(LinkedList<K>* dest) {
    if (dest == 0) return;

    for (common::uint32_t i = 0; i < capacity; i++)
      if (buckets[i] != 0) {
        typename LinkedList<HashNode>::Node* temp = buckets[i]->head;
        while (temp != 0) {
          dest->Append(temp->data.key);
          temp = temp->next;
        }
      }
  }

  /**
   * [populates provided LinkedList (dest) with all values in HashMap],
   * Usage:
   * LinkedList<const char*> myValues;
   * myMap.GetValues(&myValues);
   */
  void GetValues(LinkedList<V>* dest) {
    if (dest == 0) return;

    for (common::uint32_t i = 0; i < capacity; i++)
      if (buckets[i] != 0) {
        typename LinkedList<HashNode>::Node* temp = buckets[i]->head;
        while (temp != 0) {
          dest->Append(temp->data.value);
          temp = temp->next;
        }
      }
  }

  void GetPairs(LinkedList<Pair<K, V>>* dest) {
    if (dest == 0) return;
    for (common::uint32_t i = 0; i < capacity; i++) {
      if (buckets[i] != 0) {
        typename LinkedList<HashNode>::Node* temp = buckets[i]->head;
        while (temp != 0) {
          Pair<K, V> pair;  // struture data from Node* temp into a KeyValuePair
          pair.key = temp->data.key;
          pair.value = temp->data.value;
          dest->Append(pair);  // append the KeyValuePair containing structured data

          temp = temp->next;
        }
      }
    }
  }
};
}  // namespace ds
}  // namespace utils
}  // namespace os

#endif

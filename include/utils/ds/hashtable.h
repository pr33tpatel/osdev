#ifndef __OS__UTILS__DS__HASHTABLE_H
#define __OS__UTILS__DS__HASHTABLE_H

#include <common/types.h>
#include <memorymanagement.h>
#include <utils/ds/linkedlist.h>
#include <utils/hash.h>


namespace os {
namespace utils {
namespace ds {


template <typename K, typename V, common::uint32_t MaxSize = 1024>
class HashTable {
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
  HashTable() {
    for (int i = 0; i < capacity; i++) {
      buckets[i] = 0;
    }
    count = 0;
  }
  ~HashTable() {
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

  bool DetectAndUpdateCollision(LinkedList<HashNode>* bucket, const K* key, const V* value) {
    /* logic:
     * detect a collision at specified bucket by:
     * - traversing the bucket (LinkedList)
     * - comparing the data of the temp node to the current node
     * - if collision is found, update the existing value
     */
    typename LinkedList<HashNode>::Node* temp = bucket->head;
    while (temp != 0) {
      if (Hasher<K>::isEqual(temp->data.key, *key)) {
        // collision detected
        temp->data.value = *value;
        return true;
      }
      temp = temp->next;
    }
    return false;
  }

  void Insert(const K* key, const V* value) {
    /* logic:
     * get bucket index
     * check to see if the key already exists in the bucket
     * if so, update the value of the key in that bucket
     * if not, create a new Node and append it to the end of the bucket and increment count
     */
    common::uint32_t index = Hasher<K>::Hash(*key) % MaxSize;
    // create a LinkedList if it doesnt exist
    if (buckets[index] == 0) {
      buckets[index] = (LinkedList<HashNode>*)new LinkedList<HashNode>;
    }
    // // check for collision
    // bool collisionFound = DetectAndUpdateCollision(buckets[index], key, value);
    // if (collisionFound) return;

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
    common::uint32_t index = Hasher<K>::Hash(*key) % MaxSize;
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
};
}  // namespace ds
}  // namespace utils
}  // namespace os

#endif

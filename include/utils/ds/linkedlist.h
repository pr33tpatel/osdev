#ifndef __OS__UTILS__DS__LINKEDLIST_H
#define __OS__UTILS__DS__LINKEDLIST_H

#include <common/types.h>
#include <memorymanagement.h>
#include <utils/ds/keyvaluepair.h>
#include <utils/print.h>


namespace os {
namespace utils {
namespace ds {

template <typename T>
class LinkedList {
 private:
 public:
  struct Node {
    T data;
    Node* next;
  };
  Node* head;
  Node* tail;
  common::uint32_t count;

  LinkedList() {
    head = nullptr;
    tail = nullptr;
    count = 0;
  }
  ~LinkedList() {
    Node* temp = head;
    while (temp != nullptr) {
      Node* next = temp->next;
      delete temp;
      temp = next;
    }
  }

  /**
   * [O(1) append to the end, making it the new tail]
   */
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
  };

  /**
   * [O(1) add to the front, making it the new head]
   */
  void Prepend(T val) {
    Node* node = new Node;
    node->data = val;
    node->next = head;
    head = node;
    if (tail == 0) {
      tail = node;
    }
    count++;
  };

  /**
   * [insert a node before the given integer position]
   */
  void Insert(T val, common::uint32_t position) {
    if (position >= count) return;
    Node* node = new Node;
    node->data = val;
    Node* temp = head;
    for (int i = 1; i < position - 1 && temp != nullptr; i++) {
      temp = temp->next;
    }
    node->next = temp->next;
    temp->next = node;
    count++;
  };

  /**
   * [insert a node before the first occurence of positionNode]
   */
  void Insert(T val, T positionNode) {
    Node* node = new Node;
    Node* temp = head;
    while (temp != nullptr && temp->next != positionNode) {
      temp = temp->next;
    }
    // now, temp is the node before positionNode
    Node* beforeNode = temp;
    Node* afterNode = temp->next;

    beforeNode->next = node;
    node->next = afterNode;
    count++;
  };

  /**
   * [O(n) removes the first occurence of 'val']
   */
  void Remove(T val) {
    Node* temp = FindOneBefore(val);
    temp->next = val->next;
    val->next = nullptr;
    delete val;
  };

  /**
   * [O(n), returns a pointer to the first occurence of 'val' or nullptr(0) if not found]
   */
  Node* Find(T val) {
    Node* temp = head;
    while (temp != nullptr) {
      if (temp->data == val) {
        return temp;
      }
      temp = temp->next;
    }
    return 0;
  };

  /**
   * [O(n), returns a pointer to the node before the first occurence of 'val' or nullptr(0) if not found]
   */
  Node* FindOneBefore(T val) {
    Node* temp = head;
    while (temp != nullptr) {
      if (temp->next->data == val && temp->next != nullptr) {
        return temp;
      }
      temp = temp->next;
    }
    return 0;
  };

  /**
   * [O(1) removes the current head and makes next node the new head]
   */
  void RemoveHead() {
    if (head == 0) return;
    Node* temp = head;
    head = head->next;
    delete temp;
    count--;
    if (head == 0) tail = 0;
  }

  /**
   *  [returns the number of nodes (size) of list]
   */
  common::uint32_t Size() {
    return count;
  };

  /**
   * [returns true is the list is empty]
   */
  bool isEmpty() {
    return count == 0;
  };

  /**
   * [prints contents linked list]
   */
  void printList() {
    Node* temp = head;
    while (temp != nullptr) {
      printType(temp->data);
      if (temp->next != nullptr) printf(" -> ");
      temp = temp->next;
    }
    printf("\n");
  };

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
      if (temp->next != 0) {
        printf(" -> ");
      }
      temp = temp->next;
    }
    printf("\n");
  }

  // END
};
}  // namespace ds
}  // namespace utils
}  // namespace os

#endif

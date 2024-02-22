#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

#define FULL_BOARD
#define SPIN_SIZE 6

#define HASH_TABLE_SIZE 97

#ifdef FULL_BOARD
#define START 0
#define END 100
#define LEN_SQUARES 81
#endif

#ifdef BOARD_0_10_2
#define START 0
#define END 10
#define LEN_SQUARES 8
#endif

#ifdef BOARD_70_100_8
#define START 70
#define END 100
#define LEN_SQUARES 22
#endif

#ifdef BOARD_50_100_10
#define START 50
#define END 100
#define LEN_SQUARES 40
#endif

int jump(int land) {
  switch (land) {
    #ifdef FULL_BOARD
    case   1: return  38;
    case   4: return  14;
    case   9: return  31;
    case  21: return  42;
    case  28: return  84;
    case  36: return  44;
    case  51: return  67;
    case  71: return  91;
    case  80: return 100;
    case  98: return  78;
    case  95: return  75;
    case  93: return  73;
    case  87: return  24;
    case  64: return  60;
    case  62: return  19;
    case  56: return  53;
    case  49: return  11;
    case  48: return  26;
    case  16: return   6;
    #endif
    #ifdef BOARD_0_10_2
    case   4: return   7;
    case   9: return   2;
    #endif
    #ifdef BOARD_70_100_8
    case  71: return  91;
    case  74: return  77;
    case  76: return  84;
    case  80: return 100;
    case  98: return  78;
    case  95: return  75;
    case  93: return  73;
    case  87: return  82;
    #endif
    #ifdef BOARD_50_100_10
    case  51: return  67;
    case  59: return  74;
    case  71: return  91;
    case  80: return 100;
    case  98: return  78;
    case  95: return  75;
    case  93: return  73;
    case  87: return  54;
    case  64: return  60;
    case  56: return  53;
    #endif
    default: return land;
  }
}

typedef struct Node Node;
struct Node {
  int   key;
  mpq_t value;
  Node* next;
};

Node* initNode(int key, const mpq_t value) {
  Node* node = malloc(sizeof(Node));
  node->key = key;
  mpq_init(node->value);
  mpq_set(node->value, value);
  return node;
}

void freeNode(Node* node) {
  mpq_clear(node->value);
  free(node);
}

void printNode(Node* node) {
  if (node == NULL) {
    printf("NULL\n");
  } else {
    gmp_printf("Node (%d): %Qd\n", node->key, node->value);
  }
}

typedef struct SparseVector SV;
struct SparseVector {
  Node* head;
};

SV* initSV() {
  SV* sv = malloc(sizeof(SV));
  sv->head = NULL;
  return sv;
}

void freeSV(SV* sv) {
  Node* current = sv->head;
  Node* next;
  while (current != NULL) {
    next = current->next;
    freeNode(current);
    current = next;
  }
  free(sv);
}

Node* getSV(SV* sv, int key) {
  Node* current = sv->head;
  while (current != NULL) {
    if (current->key == key) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

void setSV(SV* sv, int key, const mpq_t value) {
  Node* current = getSV(sv, key);
  if (current == NULL) {
    Node* node = initNode(key, value);
    node->next = sv->head;
    sv->head = node;
  } else {
    mpq_set(current->value, value);
  }
}

int deleteSV(SV* sv, int key) {
  Node* previous;
  Node* current = sv->head;
  while (current != NULL) {
    if (current->key == key) {
      if (previous != NULL) {
        previous->next = current->next;
      } else {
        sv->head = current->next;
      }
      freeNode(current);
      return 0;
    }
    previous = current;
    current = current->next;
  }
  return 1;
}

Node* getDeleteSV(SV* sv, int key) {
  Node* previous = NULL;
  Node* current = sv->head;
  while (current != NULL) {
    if (current->key == key) {
      if (previous != NULL) {
        previous->next = current->next;
      } else {
        sv->head = current->next;
      }
      return current;
    }
    previous = current;
    current = current->next;
  }
  return NULL;
}

Node* popSV(SV* sv) {
  Node* head = sv->head;
  if (head == NULL) {
    return NULL;
  }
  sv->head = head->next;
  return head;
}

typedef struct HashTable HT;
struct HashTable {
  Node** table;
};

HT* initHT() {
  HT* ht = malloc(sizeof(HT*));
  ht->table = malloc(sizeof(Node*) * HASH_TABLE_SIZE);
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    ht->table[i] = NULL;
  }
  return ht;
}

void freeHT(HT* ht) {
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    Node* current = ht->table[i];
    Node* next;
    while (current != NULL) {
      next = current->next;
      freeNode(current);
      current = next;
    }
  }
  free(ht);
}

int hashFunction(int key) {
  return key % HASH_TABLE_SIZE;
}

Node* getHT(HT* ht, int key) {
  unsigned int hash = hashFunction(key);
  Node* current = ht->table[hash];
  while (current != NULL) {
    if (current->key == key) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

Node* getHThashed(HT* ht, int hash, int key) {
  Node* current = ht->table[hash];
  while (current != NULL) {
    if (current->key == key) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

void setHT(HT* ht, int key, const mpq_t value) {
  int hash = hashFunction(key);
  Node* current = getHThashed(ht, hash, key);
  if (current == NULL) {
    Node* node = initNode(key, value);
    node->next = ht->table[hash];
    ht->table[hash] = node;
  } else {
    mpq_set(current->value, value);
  }
}

int deleteHT(HT* ht, int key) {
  unsigned int hash = hashFunction(key);
  Node* previous = NULL;
  Node* current = ht->table[hash];
  while (current != NULL) {
    if (current->key == key) {
      if (previous != NULL) {
        previous->next = current->next;
      } else {
        ht->table[hash] = current->next;
      }
      freeNode(current);
      return 0;
    }
    previous = current;
    current = current->next;
  }
  return 1;
}

Node* getDeleteHT(HT* ht, int key) {
  unsigned int hash = hashFunction(key);
  Node* previous = NULL;
  Node* current = ht->table[hash];
  while (current != NULL) {
    if (current->key == key) {
      if (previous != NULL) {
        previous->next = current->next;
      } else {
        ht->table[hash] = current->next;
      }
      return current;
    }
    previous = current;
    current = current->next;
  }
  return NULL;
}

Node* popHT(HT* ht) {
  int i = 0;
  Node* current = ht->table[i];
  while (i < HASH_TABLE_SIZE && current == NULL) {
    current = ht->table[++i];
  }
  if (current == NULL) return NULL;
  ht->table[i] = current->next;
  return current;
}

void processColumn(HT* rows[], HT* columns[], int rowCount, int index) {
  HT* column = columns[index];
  HT* rowFrom = rows[index];
  mpq_t diagonalValue, factor, adjustment;
  mpq_inits(diagonalValue, factor, adjustment, NULL);
  Node* diagonalNode = getDeleteHT(rowFrom, index);
  mpq_set(diagonalValue, diagonalNode->value);

  Node* columnNode;
  Node* rowFromNode;
  Node* rowToNode;
  Node* columnToNode;
  int rowIndex;
  int columnIndex;
  HT* rowTo;
  while (columnNode = popHT(column), columnNode != NULL) {
    mpq_div(factor, columnNode->value, diagonalValue);
    rowIndex = columnNode->key;
    rowTo = rows[rowIndex];
    deleteHT(rowTo, index);  // deletes from the row the copy of the node we popped from the column
    for (int bucket = 0; bucket < HASH_TABLE_SIZE; bucket++) {
      rowFromNode = rowFrom->table[bucket];
      while (rowFromNode != NULL) {
        columnIndex = rowFromNode->key;
        mpq_mul(adjustment, factor, rowFromNode->value);
        rowToNode = getHT(rowTo, columnIndex);
        if (rowToNode == NULL) {
          mpq_neg(adjustment, adjustment);
          setHT(rowTo, rowFromNode->key, adjustment);
          if (rowIndex < columnIndex && columnIndex < rowCount) {
            setHT(columns[columnIndex], rowIndex, adjustment);
          }
        } else {
          mpq_sub(rowToNode->value, rowToNode->value, adjustment);
          if (rowIndex < columnIndex && columnIndex < rowCount) {
            Node* columnToNode = getHT(columns[columnIndex], rowIndex);
            mpq_set(columnToNode->value, rowToNode->value);
          }
        }
        rowFromNode = rowFromNode->next;
      }
    }
    freeNode(columnNode);
  }
  mpq_clears(diagonalValue, factor, adjustment, NULL);
  freeHT(column);
  freeHT(rowFrom);  // won't reference again
}

int computeBigIndex(int i, int j) {
  return i * LEN_SQUARES + j;
}

int getIndex(int array[], int value) {
  for (int i = 0; i < LEN_SQUARES; i++) {
    if (array[i] == value) {
      return i;
    }
  }
  return -1;
}

int solve(int squares[], mpq_t result) {
  int rowCount = LEN_SQUARES * LEN_SQUARES;
  HT* rows[rowCount];
  HT* columns[rowCount];
  for (int i = 0; i < rowCount; i++) {
    rows[i] = initHT();
    columns[i] = initHT();
  }

  // create matrix
  mpq_t full, one;
  mpq_init(full);
  mpq_init(one);
  mpq_set_ui(full, SPIN_SIZE, 1);
  mpq_set_ui(one, 1, 1);
  for (int i = 0; i < LEN_SQUARES; i++) {
    int square = squares[i];
    for (int j = 0; j < LEN_SQUARES; j++) {
      int rowIndex = computeBigIndex(i, j);
      setHT(rows[rowIndex], rowIndex, full);  // diagonal entry

      for (int spin = 1; spin <= SPIN_SIZE; spin++) {
        int newSquare = square + spin;
        int newi;
        if (newSquare > END) {
          newSquare = square;
          newi = i;
        } else {
          newSquare = jump(newSquare);
          if (newSquare == END) goto continueNextSpin;  // no change to matrix
          newi = getIndex(squares, newSquare);
        }
        int columnIndex = computeBigIndex(j, newi);
        Node* current = getHT(rows[rowIndex], columnIndex);
        if (current == NULL) {
          setHT(rows[rowIndex], columnIndex, one);
          if (rowIndex < columnIndex) {
            setHT(columns[columnIndex], rowIndex, one);
          }
        } else {
          mpq_add(current->value, current->value, one);
          if (rowIndex < columnIndex) {
            Node* currentC = getHT(columns[columnIndex], rowIndex);
            mpq_add(currentC->value, currentC->value, one);
          }
        }
        continueNextSpin:;
      }

      setHT(rows[rowIndex], rowCount, full);  // results vector entry
    }
  }

  // Gaussian elimination
  for (int i = rowCount - 1; i > 0; i--) {
    processColumn(rows, columns, rowCount, i);
  }
  mpq_div(result, getHT(rows[0], rowCount)->value, getHT(rows[0], 0)->value);
  return 0;
}

int main() {
  int squares[LEN_SQUARES];
  for (int i = 0, n = START; n < END; n++) {
    if (jump(n) == n) {
      squares[i] = n;
      i++;
    }
  }

  mpq_t result;
  mpq_init(result);
  solve(squares, result);
  gmp_printf("%Qd\n", result);
  printf("%f\n", mpq_get_d(result));

  // *** TESTS ***

  // mpq_t a, b;
  // mpq_inits(a, b, NULL);
  // mpq_set_ui(a, 14, 10);
  // mpq_canonicalize(a);
  // mpq_set_ui(b, 3, 2);

  // printf("*** SV tests ***\n");
  // SV* sv = initSV();
  // setSV(sv, 0, a);
  // setSV(sv, 97, b);
  // printNode(getSV(sv, 0));
  // printNode(getSV(sv, 97));
  // deleteSV(sv, 0);
  // printNode(getSV(sv, 0));
  // printNode(getSV(sv, 97));
  // setSV(sv, 97, a);
  // printNode(getDeleteSV(sv, 97));
  // printNode(getSV(sv, 97));
  // setSV(sv, 0, a);
  // setSV(sv, 97, b);
  // printNode(popSV(sv));
  // printNode(popSV(sv));
  // printNode(popSV(sv));
  // freeSV(sv);

  // printf("*** HT tests ***\n");
  // HT* ht = initHT();
  // setHT(ht, 0, a);
  // setHT(ht, 97, b);
  // printNode(getHT(ht, 0));
  // printNode(getHT(ht, 97));
  // deleteHT(ht, 0);
  // printNode(getHT(ht, 0));
  // printNode(getHT(ht, 97));
  // setHT(ht, 97, a);
  // printNode(getDeleteHT(ht, 97));
  // printNode(getHT(ht, 97));
  // setHT(ht, 0, a);
  // setHT(ht, 97, b);
  // printNode(popHT(ht));
  // printNode(popHT(ht));
  // printNode(popHT(ht));
  // freeHT(ht);
}

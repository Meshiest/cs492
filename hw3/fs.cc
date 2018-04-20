#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <vector>

using namespace std;

typedef struct disk {
  unsigned long id;
  unsigned long num_blocks;
  bool used;
} disk;

typedef struct file {
  unsigned long addr;
  int bytes_used;
} file;

typedef enum {
  DIR_NODE,
  FILE_NODE
} NodeType;

template <class Type>
class Node {

public:
  Node(string name);
  Node(string name, Node<Type>* parent);

private:
  string name;
  Node<Type>* parent;
  vector<Node<Type> > children;
};

template <class Type>
Node<Type>::Node(string name) {
  this->name = name;
  this->parent = NULL;
}

template <class Type>
Node<Type>::Node(string name, Node<Type>* parent) {
  this->name = name;
  this->parent = parent;
}

int main(int argc, char* argv[]) {
  // create files to open
  ifstream file_list, dir_list;
  istringstream iss;

  // disk size and block size
  unsigned long disk_size = 0;
  unsigned block_size = 0;

  int opt;
  // getopt is c function to do parameter checking easier
  // would use c++'s boost but we on linux lab sadness
  while((opt = getopt(argc, argv, "f:d:s:b:")) != -1) {
    switch(opt) {
      case 'f':
        file_list.open(optarg);
        if(!file_list) {
          cerr << "Error: Invalid file list path." << endl;
          return -1;
        }
        break;
      case 'd':
        dir_list.open(optarg);
        if(!dir_list) {
          cerr << "Error: Invalid directory list path." << endl;
          return -1;
        }
        break;
      case 's':
        iss.clear();
        iss.str(optarg);
        if(!(iss >> disk_size)) {
          cerr << "Error: Invalid disk size." << endl;
          return -1;
        }
        break;
      case 'b':
        iss.clear();
        iss.str(optarg);
        if(!(iss >> block_size)) {
          cerr << "Error: Invalid block size." << endl;
          return -1;
        }
        break;
      default:
        cerr << "Error Invalid usage: <" << *argv <<  "> -f <file list> -d <directory list> -s <disk size> -b <block size>" << endl;
        return 1;
    }
  }

  if(!file_list || !dir_list || !disk_size || !block_size) {
    cerr << "Error Invalid usage: <" << *argv <<  "> -f <file list> -d <directory list> -s <disk size> -b <block size>" << endl;
    return -1;
  }

  disk idisk = {.id = 0, .num_blocks = disk_size/block_size, .used = false};

  return 0;
}

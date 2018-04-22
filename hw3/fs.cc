#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <vector>


using namespace std;

// disk struct to use with linked list
typedef struct disk {
  struct disk* next;
  unsigned long id;
  unsigned long num_blocks;
  bool used;
} disk;

// file struct to use with linked list
typedef struct file {
  unsigned long addr;
  int bytes_used;
} file;

// enums for diff node types
typedef enum { DIRN } DIR_NODE;
typedef enum { FILEN } FILE_NODE;

// to get current local time
struct tm now() {
  time_t x = time(NULL);
  struct tm *t = localtime(&x);
  return *t;
}

// node that takes a typename so we can use with our enums
template <typename Type>
class Node {

public:
  Node(string name);
  Node(string name, Node<Type>* parent);
  // honestly would be better to make two more constructors
  // but eh
  void SetTime(struct tm made);
  Node<Type>* find_child(string path);

  string name;
  // below is for differnet types of nodes
  // this is gross but linux lab sucks
  // so I don't really hava
  vector<Node<Type>* > children;
  unsigned long size;
  list<file*> blocks;
  Node<Type>* parent;
private:
  struct tm time;
};

// if no parent
template <typename Type>
Node<Type>::Node(string name) {
  this->name = name;
  this->time = now();
  this->parent = NULL;
}

// if parent
template <typename Type>
Node<Type>::Node(string name, Node<Type>* parent) {
  this->name = name;
  this->time = now();
  this->parent = parent;
}

template <typename Type>
void Node<Type>::SetTime(struct tm made) {
  this->time = made;
}

template <typename Type>
Node<Type>* Node<Type>::find_child(string path) {
  for(auto it = begin(this->children); it != end(this->children); ++it) {
    // not sure how to check the type of a generic in c++
  }
}

void mkdir(string path, Node<DIR_NODE>* root) {
  int split = path.find('/');

  if(split != string::npos) {
    int plen = split - path.length();
    string sub = path.substr(0, split);

    if(sub.compare(".") == 0) {
      mkdir(sub.substr(1), root);
      return;
    }

    // if directory exists put new director in
    for(int i = 0; i < root->children.size(); i++) {
      Node<DIR_NODE>* child = root->children[i];
      if(child->name.compare(sub) == 0) {
        mkdir(sub.substr(1), child);
        return;
      }
    }

    // doesn't exist make it
    Node<DIR_NODE>* create = new Node<DIR_NODE>(sub, root);
    root->children.push_back(create);
    mkdir(sub.substr(1), create);

  } else if(split == string::npos) {
    Node<DIR_NODE>* create = new Node<DIR_NODE>(path, root);
    root->children.push_back(create);
  }
}

void ldisk_merge(disk* d) {
  disk* next;

  while (next = d->next) {
    if (d->used == next->used) {
      d->num_blocks += next->num_blocks;
      d->next = next->next;

      free(next);
    } else {
      d = next;
    }
  }
}

template <typename Type>
void insert_file_node(Node<DIR_NODE>* root, string path, Node<Type>* file) {
  auto index = path.find('/');
  if(index != string::npos) {
    auto dir = path.substr(0, index);

    if(dir.compare(".") == 0) {
      insert_file_node(root, path.substr(index+1, string::npos), file);
      return;
    }

   // todo: loop through `root->children` to find a
   // directory that matches `dir`
   // Then insert_file_node on that directory
   // I started this with Node::find_child


  } else {
    file->parent = root;
    root->children.push_back(file);
  }
}

// parse dir file
Node<DIR_NODE>* parse_dirs(ifstream& dir_list) {
  // create linked list
  Node<DIR_NODE>* root = new Node<DIR_NODE>("/");

  // read line by line making directories
  string line;
  while(dir_list >> line) {
    //cout << line << endl;
    mkdir(line, root);
  }

  return root;
}

void parse_file_list(ifstream& file_list, Node<DIR_NODE>* root, disk* dsk, int block_size) {
  unsigned long size;
  string dump, day, month, timestamp, datetime, file_path;

  // read the things we need
  // c++ doesn't do this well
  while(file_list >>
      dump >> dump >> dump >> dump >> dump >> dump >>
      size >> month >> day >> timestamp >> file_path) {
    datetime = month + " " +  day + " " + timestamp;
    struct tm date = {0};
    int timeoryear = timestamp.find(":");
    strptime(datetime.c_str(), timeoryear != string::npos ? "%b %d %H:%M" : "%b %d %Y", &date);

    if(date.tm_year == 0) {
      date.tm_year = now().tm_year;
    }

    int last_slash = file_path.find_last_of("/");
    Node<FILE_NODE>* file = new Node<FILE_NODE>(file_path.substr(last_slash));
    file->SetTime(date);

    // todo: re-implement alloc blocks here
    // don't actually need to allocate but need to mark
    // blocks with proper data and used or not

    // todo: implement ldisk merge
    // todo: implement insert file node
  }
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

  Node<DIR_NODE>* root = parse_dirs(dir_list);

  parse_file_list(file_list, root, &idisk, block_size);

  return 0;
}

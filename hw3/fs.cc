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
class Disk {
public:
  Disk(Disk* next, unsigned long id, unsigned long num_blocks, bool used);
  Disk* next;
  unsigned long id;
  unsigned long num_blocks;
  bool used;
};

Disk::Disk(Disk* next, unsigned long id, unsigned long num_blocks, bool used) {
  this->next = next;
  this->id = id;
  this->num_blocks = num_blocks;
  this->used = used;
}

// file struct to use with linked list
class File {
public:
  File(unsigned long addr, int bytes_used);
  unsigned long addr;
  int bytes_used;
};

File::File(unsigned long addr, int bytes_used) {
  this->addr = addr;
  this->bytes_used = bytes_used;
}

// enums for diff node types
typedef enum {
  DIR_NODE,
  FILE_NODE
} NodeType;

// to get current local time
struct tm now() {
  time_t x = time(NULL);
  struct tm *t = localtime(&x);
  return *t;
}

// node that takes a typename so we can use with our enums
class Node {

public:
  Node(string name, NodeType type);
  Node(string name,  NodeType type, Node* parent);
  // honestly would be better to make two more constructors
  // but eh
  void SetTime(struct tm made);
  Node* find_child(string path);

  string name;
  // below is for differnet types of nodes
  // this is gross but linux lab sucks
  // so I don't really hava
  vector<Node* > children;
  unsigned long size;
  File* blocks;
  Node* parent;
  NodeType type;
private:
  struct tm time;
};

// if no parent
Node::Node(string name, NodeType type) {
  this->name = name;
  this->type = type;
  this->time = now();
  this->parent = NULL;
}

// if parent
Node::Node(string name, NodeType type, Node* parent) {
  this->name = name;
  this->type = type;
  this->time = now();
  this->parent = parent;
}

void Node::SetTime(struct tm made) {
  this->time = made;
}

/*Node* Node::find_child(string path) {
  for(auto it = begin(this->children); it != end(this->children); ++it) {
    // not sure how to check the type of a generic in c++
  }
}*/

void mkdir(string path, Node* root) {
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
      Node* child = root->children[i];
      if(child->name.compare(sub) == 0) {
        mkdir(sub.substr(1), child);
        return;
      }
    }

    // doesn't exist make it
    Node* create = new Node(sub, DIR_NODE, root);
    root->children.push_back(create);
    mkdir(sub.substr(1), create);

  } else if(split == string::npos) {
    Node* create = new Node(path, DIR_NODE, root);
    root->children.push_back(create);
  }
}

void disk_merge(Disk* d) {
  Disk* next;

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

void insert_file_node(Node* root, string path, Node* file) {
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
Node* parse_dirs(ifstream& dir_list) {
  // create linked list
  Node* root = new Node("/", DIR_NODE);

  // read line by line making directories
  string line;
  while(dir_list >> line) {
    //cout << line << endl;
    mkdir(line, root);
  }

  return root;
}

void parse_file_list(ifstream& file_list, Node* root, Disk* dsk, int block_size) {
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
    Node* file = new Node(file_path.substr(last_slash), FILE_NODE);
    file->SetTime(date);
    // todo: re-implement alloc blocks here
    //file->blocks = alloc_blocks(dsk, size, block_size);

    // todo: implement ldisk merge
    disk_merge(dsk);

    // todo: implement insert file node
    //insert_file_node(root, file_path, file);
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

  Disk* idisk = new Disk(NULL, 0, disk_size/block_size, false);

  Node* root = parse_dirs(dir_list);

  parse_file_list(file_list, root, idisk, block_size);

  return 0;
}

#include <algorithm>
#include <cassert>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
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
  File(File* next, unsigned long addr, int bytes_used);
  File* next;
  unsigned long addr;
  int bytes_used;
};

File::File(File* next, unsigned long addr, int bytes_used) {
  this->next = next;
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
  assert(parent == NULL || parent->type == type);
  this->name = name;
  this->type = type;
  this->time = now();
  this->parent = NULL;
}

// if parent
Node::Node(string name, NodeType type, Node* parent) {
  assert(parent == NULL || parent->type == type);
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
  assert(root->type == DIR_NODE);
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
  assert(root->type == DIR_NODE);
  assert(file->type == FILE_NODE);

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
    for(auto item : root->children) {
      Node* child = item;
      if(child->type == DIR_NODE && (child->name.compare(dir) == 0)) {
        insert_file_node(child, path.substr(index+1), file);
        return;
      }
    }

    assert(false && "Couldn't find directory for file. Were your file_list.txt and dir_list.txt generated together?");
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

File* make_files(int from, int num, int block_size, File** last) {
  if(num == 0) {
    return NULL;
  }

  File* first = new File(NULL, from * block_size, block_size);
  File* prev = first;
  for(int i = 1; i < num; ++i) {
    File* block = new File(NULL, (from + i) * block_size, block_size);
    prev->next = block;
    prev = block;
  }

  if(last) {
    *last = prev;
  }

  return first;
}

File* alloc_blocks(Disk* dsk, unsigned long size, int block_size) {
  if(size == 0) {
    return NULL;
  }

  while(dsk && dsk->used) {
    dsk = dsk->next;
  }

  if(!dsk) {
    cout << "Out of space" << endl;
    return NULL;
  }

  unsigned long blocks_needed = size/block_size;
  if(size % block_size) {
    blocks_needed++;
  }

  if(blocks_needed >= dsk->num_blocks) {
    dsk->used = true;

    File* last;
    File* list = make_files(dsk->id, dsk->num_blocks, block_size, &last);

    last->next = alloc_blocks(dsk->next, size - dsk->num_blocks *block_size, block_size);

    return list;
  } else {
    int used_node_size = blocks_needed;
    int free_node_size = dsk->num_blocks - blocks_needed;

    assert(used_node_size != 0);
    assert(free_node_size != 0);

    Disk* next = dsk->next;
    Disk* free_blocks = new Disk(next, dsk->id + used_node_size, free_node_size, false);
    dsk->next = free_blocks;
    free_blocks->next = next;

    dsk->used = true;

    if(size % block_size != 0) {
      File* last = NULL;
      File* list = make_files(dsk->id, used_node_size-1, block_size, &last);
      File* partial = new File(NULL, (dsk->id + dsk->num_blocks -1) * block_size, size % block_size);

      if(last) {
        last->next = partial;
      }
      return list? list : partial;
    } else {
      File* list = make_files(dsk->id, used_node_size, block_size, NULL);
      return list;
    }
  }
}

void parse_file_list(ifstream& file_list, Node* root, Disk* dsk, int block_size) {
  assert(root->type == DIR_NODE);
  assert(root->parent == NULL);

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
    cout << last_slash << endl;
    cout << file_path << " " << file_path.substr(last_slash) << endl;
    Node* file = new Node(file_path.substr(last_slash+1), FILE_NODE);
    file->SetTime(date);
    cout << file->name << endl;
    // todo: re-implement alloc blocks here
    //file->blocks = alloc_blocks(dsk, size, block_size);
    // this seg faults

    // todo: implement ldisk merge
    //disk_merge(dsk);

    // todo: implement insert file node
    insert_file_node(root, file_path, file);
  }
}

vector<Node*> dir_node_path(Node* dir) {
  vector<Node*> res;

  Node* cur = dir;
  while(cur) {
    res.push_back(cur);
    cur = cur->parent;
  }

  reverse(res.begin(), res.end());

  return res;
}

void print_dir_path(Node* cur) {
  cout << "/";

  vector<Node*> path = dir_node_path(cur);
  path.erase(path.begin());

  for(auto p : path) {
    cout << p->name << "/";
  }
}

void ls(Node* dir) {
  assert(dir->type == DIR_NODE);
  for(auto item : dir->children) {
    cout << item->name << endl;
  }
}

Node* find_node_from_path(string path, Node* root, Node* org) {
  int split = path.find('/');

  if(split == string::npos) {

    for(auto item : root->children) {
      Node* child = item;
      if(child->name.compare(path) == 0) {
        return child;
      }
    }

  }

  cout << "No such file or directory " << path << endl;
  return org;
}

Node* cd(string path, Node* cur) {
  if(path.compare("..") == 0) {
    int split = path.find('/');

    if(split == string::npos && (cur->name.compare("/") != 0)) {
      return cur->parent;
    } else {
      cout << "You are in root and cannot cd back a directory." << endl;
      return cur;
    }
  }
  Node* found = find_node_from_path(path, cur, cur);

  if(found->type == FILE_NODE) {
    cout << found->name << " is a file, cannot cd into a file" << endl;
  }

  return found;
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

  // input loop
  Node* curr_dir = root;
  string command;
  size_t len = 0;
  ssize_t nread;

  print_dir_path(curr_dir);
  cout << " > ";

  while(getline(cin, command)) {

    if(command.compare("exit") == 0) {
      cout << "goodbye" << endl;
      break;
    } else if(command.compare("dir") == 0) {
      cout << "dir" << endl;
    } else if(command.compare("ls") == 0) {
      ls(curr_dir);
    } else if(command.compare("cd..") == 0) {
      curr_dir = cd("..", curr_dir);
    } else if(command.compare(0, 2, "cd") == 0) {
      curr_dir = cd(command.substr(3), curr_dir);
    } else if(command.compare(0, 5, "mkdir") == 0) {
      mkdir(command.substr(6), curr_dir);
    } else if(command.compare(0, 6, "create") == 0) {
      cout << "create" << endl;
    } else if(command.compare(0, 6, "delete") == 0) {
      cout << "delete" << endl;
    } else if(command.compare("append") == 0) {
      cout << "append" << endl;
    } else if(command.compare("remove") == 0) {
      cout << "remove" << endl;
    } else if(command.compare("prdisk") == 0) {
      cout << "prdisk" << endl;
    } else if(command.compare("prfiles") == 0) {
      cout << "prfiles" << endl;
    } else {
      cout << "Unknown command: " << command << endl;
    }


    assert(curr_dir->type == DIR_NODE);
    print_dir_path(curr_dir);
    cout << " > ";
  }

  return 0;
}

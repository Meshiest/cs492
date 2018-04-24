#include <algorithm>
#include <cassert>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <queue>
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
  //bool alloc;
};

File::File(File* next, unsigned long addr, int bytes_used) {
  this->next = next;
  this->addr = addr;
  this->bytes_used = bytes_used;
  //this->alloc = false;
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

void mkdir(string path, Node* root) {
  assert(root->type == DIR_NODE);
  if(path.compare(0, 2, "./") == 0) {
    path = path.substr(2);
  }
  int split = path.find('/');

  if(split != string::npos) {
    string sub = path.substr(0, split);

  if(sub.compare(".") == 0 && (sub.substr(1).compare("") != 0)) {
    mkdir(sub.substr(1), root);
    return;
  }

    // if directory exists put new director in
    for(auto child : root->children) {
      if(child->name.compare(sub) == 0) {
        mkdir(path.substr(split+1), child);
        return;
      }
    }

    // doesn't exist make it
    Node* create = new Node(sub, DIR_NODE, root);
    root->children.push_back(create);
    mkdir(path.substr(split+1), create);

  } else if(split == string::npos) {
    if(path.compare(".") == 0) {
      return;
    }
    Node* create = new Node(path, DIR_NODE, root);
    root->children.push_back(create);
  }
}

void disk_merge(Disk* d) {
  Disk* next;

  while ((next = d->next)) {
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

    Node* child;
    for(auto item : root->children) {
      child = item;
      if(child->type == DIR_NODE && (child->name.compare(dir) == 0)) {
        insert_file_node(child, path.substr(index+1), file);
        return;
      }
    }
    //cout << child->name;
    //assert(false && "Couldn't find directory for file. Were your file_list.txt and dir_list.txt generated together?");
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

Disk* free_block(Disk* dsk, unsigned block) {
  while(!(dsk->id <= block && block < dsk->id + dsk->num_blocks)) {
    dsk = dsk->next;
    assert(dsk);
  }

  assert(dsk->used);
  Disk *oldnext = dsk->next;
  unsigned long oldnblocks = dsk->num_blocks;

  Disk* before = dsk;
  Disk* free_node = new Disk(NULL, 0, 0, false);
  Disk* after = new Disk(NULL, 0, 0, false);

  before->next = free_node;
  free_node->next = after;
  after->next = oldnext;

  free_node->id = block;
  after->id = block+1;

  before->num_blocks = block - dsk->id;
  free_node->num_blocks = 1;
  after->num_blocks = oldnblocks - before->num_blocks - free_node->num_blocks;

  if(before->num_blocks == 0) {
    *before = *free_node;
    delete(free_node);
    free_node = NULL;
  }

  if(after->num_blocks == 0) {
    assert(!after->next || after->id == after->next->id);
    if(free_node) {
      free_node->next = after->next;
    } else {
      before->next = after->next;
    }
    delete(after);
  }

  return before;
}

File* alloc_blocks(Disk* dsk, unsigned long size, int block_size) {
  if(size == 0) {
    return NULL;
  }

  while(dsk && dsk->used) {
    dsk = dsk->next;
  }

  if(!dsk) {
    cout << "Out of space please alloc more disk space" << endl;
    exit(EXIT_FAILURE);
    /*File* alloc_err = new File(NULL, 0, 0);
    alloc_err->alloc = true;
    return alloc_err;*/
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
    /*if(last->next->alloc) {
      for(int i = dsk->num_blocks - 1; i >= 0; --i) {
        free_block(dsk, dsk->id + i);
      }
      File* freeme = list;
      while(!freeme->next->alloc) { // go through list freeing blocks
        File* next = freeme->next;
        delete(freeme);
        freeme = next;
      }
      File* alloc_err = new File(NULL, 0, 0);
      alloc_err->alloc = true;
      return alloc_err;
    }*/

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
    //cout << datetime << endl;
    strptime(datetime.c_str(), timeoryear != string::npos ? "%b %d %H:%M" : "%b %d %Y", &date);

    if(date.tm_year == 0) {
      date.tm_year = now().tm_year;
    }

    int last_slash = file_path.find_last_of("/");
    string name = file_path.substr(last_slash+1);
    //cout << name << endl;
    Node* file = new Node(name, FILE_NODE); // this is seg faulting?
    file->SetTime(date);
    // todo: re-implement alloc blocks here
    file->blocks = alloc_blocks(dsk, size, block_size);

    disk_merge(dsk);

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
    return cur;
  }

  return found;
}

void append(Node* dir, string filename, int size, Disk* d, unsigned long block_size) {
  assert(dir->type == DIR_NODE);
  Node* f = find_node_from_path(filename, dir, NULL);
  if(!f || f->type == DIR_NODE) {
    cout << filename << ": no such file" << endl;
    return;
  }

  File* last = f->blocks;
  if(!last) {
    f->blocks = alloc_blocks(d, size, block_size);
    disk_merge(d);

    /*if(!f->blocks->alloc) {
      f->blocks = NULL;
      return;
    }*/
  } else {
    while(last->next != NULL) last = last->next;
    int free_space = block_size - last->bytes_used;
    int fill_space = free_space > size ? size : free_space;
    last->next = alloc_blocks(d, size - fill_space, block_size);

    /*if(last->alloc || last->next && last->next->alloc) {
      last->next = NULL;
      return;
    }*/

    last->bytes_used += fill_space;
  }

  f->size += size;
  f->SetTime(now());
}

void create(string path, Node* curdir) {
  Node* file = new Node(path, FILE_NODE);
  file->SetTime(now());
  insert_file_node(curdir, path, file);
}

void deletec(Node* file, Disk* dsk, int block_size) {
  if(!file) {
    return;
  }

  if(file->type == DIR_NODE) {
    if(file->children.size() != 0) {
      cout << "Error: directory is not empty" << endl;
      return;
    }
  } else {
    Disk* last = dsk;
    for(File* block = file->blocks; block != NULL; block = block->next) {
      assert(block->addr % block_size == 0);
      last = free_block(last, block->addr / block_size);
    }
    disk_merge(dsk);
  }

  Node* parent = file->parent;
  assert(parent->type == DIR_NODE);
  int child_index = -1;
  int iter = 0;
  for(auto child : parent->children) {
    if(child == file) {
      child_index = iter;
      break;
    }
    iter++;
  }

  assert(child_index != -1);
  parent->children.erase(parent->children.begin() + child_index);

  if(file->type == FILE_NODE) {
    parent->SetTime(now());
  }

  if(file->type == DIR_NODE) {
    file->children.clear();
  }

  delete(file);
}

void shrink(Disk* disk, Node* file, int size, unsigned long block_size) {
  if(!size)
    return;

  File* prev;
  File* last = file->blocks;
  while(last->next) {
    prev = last;
    last = last->next;
  }

  if(last->bytes_used > size) {
    last->bytes_used -= size;
  } else {
    int new_size = size - last->bytes_used;
    free_block(disk, last->addr / block_size);
    disk_merge(disk);

    if(prev)
      prev->next = NULL;
    delete(last);

    shrink(disk, file, new_size, block_size);
  }
}

void remove(Node* dir, string filename, int size, Disk* disk, unsigned long block_size) {
  Node* file = find_node_from_path(filename, dir, NULL);
  if(!file || file->type == DIR_NODE) {
    cout << filename << ": no such file" << endl;
    return;
  }

  File* blocks = file->blocks;
  if(!blocks) {
    cout << filename << " is empty" << endl;
    return;
  }

  if(file->size < size) {
    cout << filename << " is smaller than " << size;
    return;
  }

  shrink(disk, file, size, block_size);

  file->size -= size;
  if(!file->size) {
    file->blocks = NULL;
  }
}

unsigned long calc_fragmentation(Node* node, unsigned long block_size) {
  if(node->type == FILE_NODE) {
    File* last = node->blocks;
    if(!last)
      return 0;

    while(last->next)
      last = last->next;

    return block_size - last->bytes_used;
  } else {
    unsigned long sum = 0;

    for(auto item : node->children)
      sum += calc_fragmentation(item, block_size);

    return sum;
  }
}

void print_disk(Disk* disk, Node* root, unsigned long block_size) {
  while(disk) {
    cout << (disk->used ? "In use: " : "Free: ")
      << disk->id << "-" << disk->id + disk->num_blocks - 1
      << endl;
    disk = disk->next;
  }

  cout << "fragmentation: " << calc_fragmentation(root, block_size) << " bytes" << endl;
}

void show_prfiles(Node* file, unsigned long block_size) {
  print_dir_path(file->parent);
  cout << file->name << endl;
  cout << "\tsize = " << file->size << endl;

  cout << "\tblocks = ";
  if(file->blocks) {
    vector<unsigned long> blocks;

    for(File* block = file->blocks; block != NULL; block = block->next) {
      blocks.push_back(block->addr / block_size);
    }

    if(blocks.size() == 0) {
      cout << "none";
    } else {
      int from = blocks[0];
      int to = from;
      for(auto next : blocks) {
        if(next - to == 1) {
          to = next;
        } else {
          cout << from << "-" << to;
        }
      }
      if(from != to) {
        cout << from << "-" << to;
      } else {
        cout << from;
      }
    }

  } else {
    cout << "none";
  }
  cout << endl;
}

void prfiles(Node* root, unsigned long block_size) {
  for(auto child : root->children) {
    if(child->type == DIR_NODE) {
      prfiles(child, block_size);
    } else {
      show_prfiles(child, block_size);
    }
  }
}

void dir(Node* root, int space) {
  for(auto child : root->children) {
    cout << string(space, '-') << child->name << endl;
    if(child->type == DIR_NODE) {
      dir(child, space+2);
    }
  }

  return;
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
      cout << "/" << endl;
      dir(root, 2);
    } else if(command.compare("ls") == 0) {
      ls(curr_dir);
    } else if(command.compare("cd..") == 0) {
      curr_dir = cd("..", curr_dir);
    } else if(command.compare(0, 2, "cd") == 0) {
      curr_dir = cd(command.substr(3), curr_dir);
    } else if(command.compare(0, 5, "mkdir") == 0) {
      mkdir(command.substr(6), curr_dir);
    } else if(command.compare(0, 6, "create") == 0) {
      create(command.substr(7), curr_dir);
    } else if(command.compare(0, 6, "delete") == 0) {
      deletec(find_node_from_path(command.substr(7), curr_dir, NULL), idisk, block_size);
    } else if(command.compare(0, 6, "append") == 0) {
      string after = command.substr(7);
      int spacePos = after.find(' ');
      if(spacePos == string::npos) {
        cout << "Usage: append <filename> <bytes>" << endl;
      } else {
        string filename = after.substr(0, spacePos);
        int space = atoi(after.substr(spacePos + 1).c_str());
        append(curr_dir, filename, space, idisk, block_size);
      }
    } else if(command.compare(0, 6, "remove") == 0) {
      string after = command.substr(7);
      int spacePos = after.find(' ');
      if(spacePos == string::npos) {
        cout << "Usage: remove <filename> <bytes>" << endl;
      } else {
        string filename = after.substr(0, spacePos);
        int space = atoi(after.substr(spacePos + 1).c_str());
        remove(curr_dir, filename, space, idisk, block_size);
      }
    } else if(command.compare("prdisk") == 0) {
      print_disk(idisk, root, block_size);
    } else if(command.compare("prfiles") == 0) {
      prfiles(root, block_size);
    } else {
      cout << "Unknown command: " << command << endl;
    }

    print_dir_path(curr_dir);
    cout << " > ";
  }

  return 0;
}

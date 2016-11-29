/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <cstdlib>
#include <map>
#include <list>
#include <libgen.h>
#include "ramdisk.h"
#include "ramnode.h"

using namespace std;

static int ramfs_rem_size = 0;
static int current_id = 0;

//static const char *hello_str = "Hello World!\n";
//static const char *hello_path = "/hello";
static struct fuse_operations fuse_oper;

map<int,ramnode* > node_map;

map<string, int> path_map;

string getFileName(const string& s) {

   char sep = '/';

   size_t i = s.rfind(sep, s.length());
   if (i != string::npos) {
      return(s.substr(i+1, s.length() - i));
   }

   return("");
}
string getDirName(const string& s) {

   char sep = '/';

   size_t i = s.rfind(sep, s.length());
   if (i != string::npos) {
      return(s.substr(i+1, s.length() - i));
   }

   return("");
}
static void print_rem(){
  printf("Remaining size of FS: %d\n",ramfs_rem_size);
}
static int cb_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	printf("start of getattr\n");
	memset(stbuf, 0, sizeof(struct stat));
  if(path == NULL) {
    printf("path is empty!\n");
    return -ENOENT;
  }
  string spath = path;
  if(path_map.count(spath) == 0) {
    printf("Doesn't exist!\n");
    return -ENOENT;
  }
  int id = path_map[spath];
  ramnode *node = node_map[id];
  stbuf->st_uid = node->uid;
  stbuf->st_gid = node->gid;
  stbuf->st_atime = node->accessTime;
  stbuf->st_ctime = node->changeTime;
  stbuf->st_mtime = node->modifyTime;
	if (node->type == 1) {
		stbuf->st_mode = node->mode;
		stbuf->st_nlink = 2;
    printf("in dir\n");
	} else if (node->type == 0) {
		stbuf->st_mode = node->mode;
		stbuf->st_nlink = 1;
		stbuf->st_size = node->size;
    printf("in file\n");
	} else
		res = -ENOENT;

  printf("end of getattr\n");
	return res;
}

static int cb_rmdir(const char* path) {
  printf("start of rmdir\n");
  string spath = path;
  if(path == NULL) {
    printf("path is empty!\n");
    return -ENOENT;
  }
  if(path_map.count(spath) == 0) {
    printf("Doesn't exist!\n");
    return -ENOENT;
  }

  int id = path_map[spath];

  ramnode *node = node_map[id];
  if(node->type != 1) {
    printf("Not a directory!\n");
    return -ENOENT;
  }
  if (!node->child.empty()){
    printf("failed to remove %s: Directory not empty",path);
    return -ENOENT;
  }
  string p(node->name);
  char* path_dup = strdup(p.c_str());
  char *dir = dirname(path_dup);
  string dirname(dir);

  int pid = path_map[dirname];
  ramnode *parent = node_map[pid];

  parent->child.remove(node->id);
  time_t currentTime;
  time(&currentTime);
  parent->accessTime = currentTime;
  parent->changeTime = currentTime;
  parent->modifyTime = currentTime;


  free(node->data);
  ramfs_rem_size += sizeof(node);
  delete node;

  node_map.erase(id);
  path_map.erase(spath);

  print_rem();
  printf("end of rmdir\n");
  return 0;
}

static int cb_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;
	printf("start of readdir\n");
  if(path == NULL) {
    printf("path is empty!\n");
    return -ENOENT;
  }
  string spath = path;
  if(path_map.count(spath) == 0) {
    printf("Doesn't exist!\n");
    return -ENOENT;
  }
  int id = path_map[spath];
  ramnode *node = node_map[id];

  if(node->type != 1) {
    printf("not a directory!\n");
    return -ENOENT;
  }

  //if not root directory
  if(spath.compare("/")!=0) {
    filler(buf, ".", NULL, 0);
  	filler(buf, "..", NULL, 0);
  }

  list<int>::const_iterator it;
  for (it=node->child.begin(); it != node->child.end(); ++it) {
    ramnode *each = node_map[*it];
    string p(each->name);
    char* path_dup = strdup(p.c_str());
    char *fname = basename(path_dup);
    string filename(fname);

    filler(buf, fname, NULL, 0);
  }
  printf("end of readdir\n");
	return 0;
}




static int cb_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	int len;
	(void) fi;
	printf("start of read\n");

  if(path == NULL) {
    printf("path is empty!\n");
    return -ENOENT;
  }
  string spath = path;
  if(path_map.count(spath) == 0) {
    printf("Doesn't exist!\n");
    return -ENOENT;
  }
  int id = path_map[spath];
  ramnode *node = node_map[id];

//	if(strcmp(path, hello_path) != 0)
//		return -ENOENT;

//	len = strlen(hello_str);
  if (node->type == 1) {
    printf("is a directory! \n");
    return -ENOENT;
  }
  len = node->size;
	if (offset < len) {
		if ((int)(offset + size) > len)
			size = len - offset;
		memcpy(buf, node->data + offset, size);
	} else
		size = 0;
  time_t currentTime;
  time(&currentTime);
  node->accessTime = currentTime;
  printf("end of read\n");
	return size;
}

static int cb_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  //size_t len;
	//(void) fi;

  printf("start of write\n");
  if(path == NULL) {
    printf("path is empty!\n");
    return -ENOENT;
  }
  string spath = path;
  if(path_map.count(spath) == 0) {
    printf("Doesn't exist!\n");
    return -ENOENT;
  }

  int id = path_map[spath];
  ramnode *node = node_map[id];

  if (node->type == 1) {
    printf("is a directory! \n");
    return -ENOENT;
  }

  if (ramfs_rem_size - size < 0) {
    printf("filesystem out of memory !\n");
    return -ENOSPC;
  }
  time_t currentTime;
  time(&currentTime);
  node->accessTime = currentTime;
  node->changeTime = currentTime;
  node->modifyTime = currentTime;
  printf("here\n");
  printf("node->size: %d\n",node->size);
  //printf("offset: %d\n",offset);
  //printf("size: %d\n",size);
  if(node->data == NULL) {
    node->data = (char*)malloc(size);
    memcpy(node->data,buf,size);
    node->size = size;
    ramfs_rem_size-=size;
    //printf("remaining filesystem size: %d", ramfs_rem_size);
  }
  else {
    //int len = node->size + size;

    node->data = (char*)realloc(node->data, offset+size);
    memcpy(node->data+offset,buf,size);

    int change = offset+size - node->size;
    node->size = offset+size;
    printf("change: %d\n",change);
    ramfs_rem_size -= change;
  }

  print_rem();
  return size;
}

static int cb_mkdir(const char *path, mode_t mt)
{
  printf("start of mkdir\n");
  char *path_dup = strdup(path);
  string p(path_dup);
  if(p.compare("/")!=0) {
    //char  = strdup(path);
    char *dire = dirname(path_dup);
    char *fname = basename(path_dup);
    string dir(dire);
    string filename(fname);
    if(path_map.count(dir)==0) {
      return -ENOENT;
    }
    int pathID = path_map[dir];
    ramnode* pnode = node_map[pathID];
    if(pnode->type != 1) {
      printf("parent not a directory!\n");
      return -ENOENT;
    }
    pnode->child.push_back(current_id);
    time_t currentTime;
    time(&currentTime);
    // only modify and change flag of parent dir
    pnode->changeTime = currentTime;
    pnode->modifyTime = currentTime;
  }
  ramnode *node;
  node = new ramnode;
  node->id = current_id;
  ramfs_rem_size = ramfs_rem_size - sizeof(node);
  print_rem();
  if(ramfs_rem_size < 0) {
    printf("exceeded filesize\n");
    return -ENOSPC;
  }

  //string r = "/";
  node->name = path;
  time_t currentTime;
  time(&currentTime);
  node->changeTime = currentTime;
  node->modifyTime = currentTime;
  node->accessTime = currentTime;

  node->gid = fuse_get_context()->gid;
  node->uid = fuse_get_context()->uid;
  node->mode = S_IFDIR | 0755;
  node->type = 1;
  node->data = NULL;
  path_map[p] = node->id;
  node_map[current_id] = node;
  current_id++;
  printf("end of mkdir\n");
  return 0;
}

//Implemented this for touch command
static int cb_utimens (const char * path, const struct timespec tv[2]) {
  printf("utimens\n");
  return 0;
}

static int cb_create(const char * path, mode_t mode, struct fuse_file_info * fi) {
  printf("create: path: %s mode : %d", path,mode);
  if(path == NULL) {
    printf("no path to create");
    return -ENOENT;
  }
  char *path_dup = strdup(path);
  string p(path_dup);
  char *dire = dirname(path_dup);
  char *fname = basename(path_dup);
  string dir(dire);
  string filename(fname);

  if(path_map.count(dir)==0) {
    return -ENOENT;
  }
  int pathID = path_map[dir];
  ramnode* pnode = node_map[pathID];
  if(pnode->type != 1) {
    printf("parent not a directory!\n");
    return -ENOENT;
  }
  pnode->child.push_back(current_id);

  ramnode *node;
  node = new ramnode;
  node->id = current_id;
  ramfs_rem_size = ramfs_rem_size - sizeof(node);
  print_rem();
  if(ramfs_rem_size < 0) {
    printf("exceeded filesize\n");
    return -ENOSPC;
  }

  //string r = "/";
  node->name = path;
  time_t currentTime;
  time(&currentTime);
  node->accessTime = currentTime;
  node->changeTime =currentTime;
  node->modifyTime = currentTime;


  node->gid = fuse_get_context()->gid;
  node->uid = fuse_get_context()->uid;
  node->mode = S_IFREG | mode;
  node->type = 0;
  node->data = NULL;
  path_map[p] = node->id;
  node_map[current_id] = node;
  current_id++;
  printf("create: mode %d",mode);
  return 0;


}

static int cb_open(const char *path, struct fuse_file_info *fi) {
  printf("open: path: %s\n", path);
  if(path == NULL) {
    printf("no path to create");
    return -ENOENT;
  }
  string spath = path;
  if(path_map.count(spath) == 0) {
    printf("Doesn't exist!\n");
    return -ENOENT;
  }
/*  if ((fi->flags & 3) != O_RDONLY) {
    printf("open eacess\n" );
    return -EACCES;

  }*/


  printf("end of open\n");
  return 0;

}

static int cb_opendir(const char *path, struct fuse_file_info *fi) {
  printf("opendir: path: %s", path);
  if(path == NULL) {
    printf("no path to create");
    return -ENOENT;
  }
  string spath = path;
  if(path_map.count(spath) == 0) {
    printf("Doesn't exist!\n");
    return -ENOENT;
  }
  int id = path_map[spath];
  ramnode *node = node_map[id];
  if(node->type != 1) {
    printf("not a directory");
    return -ENOENT;
  }
  time(&node->accessTime);

  //if ((fi->flags & 3) != O_RDONLY)
	//	return -EACCES;

  return 0;

}

static int cb_unlink(const char *path) {
  printf("start of unlink\n");
  string spath = path;
  if(path == NULL) {
    printf("path is empty!\n");
    return -ENOENT;
  }
  if(path_map.count(spath) == 0) {
    printf("Doesn't exist!\n");
    return -ENOENT;
  }

  int id = path_map[spath];

  ramnode *node = node_map[id];
  if(node->type != 0) {
    printf(" not a file!\n");
    return -ENOENT;
  }

  string p(node->name);
  char* path_dup = strdup(p.c_str());
  char *dir = dirname(path_dup);
  string dirname(dir);

  int pid = path_map[dirname];
  ramnode *parent = node_map[pid];

  parent->child.remove(node->id);
  time_t currentTime;
  time(&currentTime);
  parent->modifyTime = currentTime;
  parent->changeTime = currentTime;

  free(node->data);
  delete node;

  node_map.erase(id);
  path_map.erase(spath);
  ramfs_rem_size += sizeof(node);
  printf("end of unlink\n");
  return 0;
}



int main(int argc, char *argv[])
{
	//printf("mainn\n");
  if(argc != 3) {
    cout << "Usage: ./ramdisk <dir> < size(MB)>" << endl;
    exit(-1);

  }
  ramfs_rem_size = 1024*1024*atoi(argv[2]);
	//fflush(stdout);
  fuse_oper.getattr	= cb_getattr;
	fuse_oper.readdir	= cb_readdir;
  fuse_oper.opendir = cb_opendir;
	fuse_oper.open = cb_open;
	fuse_oper.mkdir	= cb_mkdir;
  fuse_oper.rmdir	= cb_rmdir;
	fuse_oper.read	= cb_read;
  fuse_oper.write = cb_write;
	fuse_oper.create		= cb_create;
	fuse_oper.unlink		= cb_unlink;
  fuse_oper.utimens = cb_utimens;

  string root_path="/";
  ramnode *root;
  root = new ramnode;
  root->id = current_id;
  path_map[root_path] = root->id;
  string r = "/";
  root->name = (char*)root_path.c_str();

  time(&root->accessTime);
  time(&root->changeTime);
  time(&root->modifyTime);
  root->gid = fuse_get_context()->gid;
  root->uid = fuse_get_context()->uid;
  root->mode = S_IFDIR | 0755; //0777
  root->type = 1;
  root->data = NULL;
  node_map[current_id] = root;
  current_id++;
  char *av[2];
  av[0] = argv[0];
  av[1] = argv[1];
	return fuse_main(argc-1, av, &fuse_oper, NULL);
}

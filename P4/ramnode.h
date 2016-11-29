#ifndef __RAMNODE__
#define __RAMNODE__

#include <string>
#include <list>
using namespace std;

//same as inode struct without indirection
typedef struct _ramnode  {
  int id;

  int size;
  string name;
  int type; //0 for File , 1 for Dir
  time_t accessTime;
  time_t changeTime;
  time_t modifyTime;
  gid_t gid;
  uid_t uid;
  mode_t mode;

  char * data;
  list<int> child; // list of child id in case of directories

} ramnode;
#endif

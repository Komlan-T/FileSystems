#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "../libWad/Wad.h"

static int getattr_callback(const char *path, struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));
  if(strcmp(path, ".") == 0){
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  if(((Wad*) fuse_get_context()->private_data)->isDirectory(path)){
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  if(((Wad*) fuse_get_context()->private_data)->isContent(path)){
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = ((Wad*) fuse_get_context()->private_data)->getSize(path);
    return 0;
  }

  return -ENOENT;
}

static int do_mknod(const char *path, mode_t mode, dev_t rdev){
  ((Wad*) fuse_get_context()->private_data)->createFile(path);
  return 0;
}

static int do_mkdir(const char *path, mode_t mode){
  ((Wad*) fuse_get_context()->private_data)->createDirectory(path);
  return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
  int val = ((Wad*) fuse_get_context()->private_data)->getContents(path, buf, size, offset);
  if(val == -1){
    return -ENOENT; 
  }
  return val;
}

static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info){
  int val = ((Wad*) fuse_get_context()->private_data)->writeToFile(path, buffer, size, offset);
  if(val == -1){
    return -ENOENT;
  }
  return val;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
  std::vector<std::string> direcs;
  int val = ((Wad*) fuse_get_context()->private_data)->getDirectory(path, &direcs);
  if(val == -1){
    return -ENOENT;
  }
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  for(auto dir : direcs){
    filler(buf, dir.c_str(), NULL, 0);
  }
  return 0;
}

static struct fuse_operations operations = {
  .getattr = getattr_callback,
  .mknod = do_mknod,
  .mkdir = do_mkdir,
  .read = read_callback,
  .write = do_write,
  .readdir = readdir_callback,
};

int main(int argc, char* argv[]){
  if(argc < 3){
    std::cout << "Not enough arguements." << std::endl;
    exit(EXIT_SUCCESS);
  }
  std::string wadPath = argv[argc - 2];
  if(wadPath.at(0) != '/'){
    wadPath = std::string(get_current_dir_name()) + "/" + wadPath;
  }
  Wad* myWad = Wad::loadWad(wadPath);
  argv[argc - 2] = argv[argc - 1];
  argc--;
  return fuse_main(argc, argv, &operations, myWad);
}
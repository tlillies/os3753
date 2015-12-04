/*
FUSE: Filesystem in Userspace
Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

More modifications by Thomas Lillis

Source: fuse-2.8.7.tar.gz examples directory
http://sourceforge.net/projects/fuse/files/fuse-2.X/

This program can be distributed under the terms of the GNU GPL.
See the file COPYING.

gcc -Wall `pkg-config fuse --cflags` encfs.c -o encfs `pkg-config fuse --libs`

Note: This implementation is largely stateless and does not maintain
open file handels between open and release calls (fi->fh).
Instead, files are opened and closed as necessary inside read(), write(),
etc calls. As such, the functions that rely on maintaining file handles are
not implmented (fgetattr(), etc). Those seeking a more efficient and
more complete implementation may wish to add fi->fh support to minimize
open() and close() calls and support fh dependent functions.

*/

#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#include "aes-crypt.h"

#define MAX_PATH 4096
#define ENCRYPTED 1
#define ATTR_NAME "user.pa5-encfs.encrypted"

typedef struct {
    char *key;
    char *root_dir;
} encfs_options;

static int encfs_checkenc(char fpath[MAX_PATH])
{
    char * buffer;
    size_t buffer_size = getxattr(fpath,ATTR_NAME,NULL,0);
    fprintf(stderr, "buffer size: %zu\n",buffer_size);
    int res = 0;
    buffer = malloc(buffer_size+1);
    buffer_size = getxattr(fpath,ATTR_NAME,buffer,buffer_size);
    buffer[buffer_size] = 0;

    fprintf(stderr, "buffer: %s\n",buffer);
    if(!strcmp(buffer,"true")){
        res = 1; 
    }    
    free(buffer);
    return res;
}

static int encfs_encrypt(char fpath[MAX_PATH])
{
    fprintf(stderr, "HEY MADE IT TO ENCRYPTION!!! %s\n",fpath);
    fprintf(stderr, "temp path: ???\n");
    int check = 0;
    check = encfs_checkenc(fpath);
    fprintf(stderr, "check: %d\n",check);
    if(check != ENCRYPTED) return 0;
    encfs_options *options = (encfs_options *) (fuse_get_context()->private_data);

    FILE* in;
    FILE* out;
    FILE* temp;
    char temp_path[MAX_PATH];

    // Copy to a temp file
    in = fopen(fpath, "rb");                                                                                            
    if(!in){
        perror("infile fopen error");
        return EXIT_FAILURE;
    }
    strcpy(temp_path, fpath);
    strcat(temp_path, ".tmp");
    fprintf(stderr, "temp path: %s\n",temp_path);
    temp = fopen(temp_path, "wb+");
    fprintf(stderr, "temp path: %s\n",temp_path);
    if(!temp){
        perror("outfile fopen error");
        return EXIT_FAILURE;
    }
    if(!do_crypt(in, temp, -1, options->key)) // actual copy
    {                                                                              
        fprintf(stderr, "do_crypt failed\n");
        return -1;
    }
    fclose(in);

    // Encrypt back to origonal file
    out = fopen(fpath, "wb+");
    if(!out){
        perror("outfile fopen error");
        return EXIT_FAILURE;
    }

    if(!do_crypt(temp, out, 1, options->key)){                                                                              
        fprintf(stderr, "do_crypt failed\n");
        return -1;
    }
    fclose(out);
    fclose(temp);

    fprintf(stderr, "HEY MADE IT TO END OF ENCRYPTON!!! %s\n",fpath);
    return 0;
}

static int encfs_decrypt(char fpath[MAX_PATH])
{
    int check = 0;
    check = encfs_checkenc(fpath);
    if(check != ENCRYPTED) return 0;
    encfs_options *options = (encfs_options *) (fuse_get_context()->private_data);

    FILE* in;
    FILE* out;
    FILE* temp;
    char temp_path[MAX_PATH];

    // Copy to a temp file
    in = fopen(fpath, "rb");                                                                                            
    if(!in){
        perror("infile fopen error");
        return EXIT_FAILURE;
    }
    strcpy(temp_path, fpath);
    strcat(temp_path, ".tmp");
    temp = fopen(fpath, "wb+");
    if(!temp){
        perror("outfile fopen error");
        return EXIT_FAILURE;
    }
    if(!do_crypt(in, temp, -1, options->key)) // actual copy
    {                                                                              
        fprintf(stderr, "do_crypt failed\n");
        return -1;
    }
    fclose(in);

    // Decrypt back to origonal file
    out = fopen(fpath, "wb+");
    if(!out){
        perror("outfile fopen error");
        return EXIT_FAILURE;
    }

    if(!do_crypt(temp, out, 0, options->key)){                                                                              
        fprintf(stderr, "do_crypt failed\n");
        return -1;
    }
    fclose(out);
    fclose(temp);
    return 0;
}

static void encfs_mkpath(char fpath[MAX_PATH], const char *path)
{
    encfs_options *options = (encfs_options *) (fuse_get_context()->private_data);
    strcpy(fpath, options->root_dir);
    strncat(fpath, path, MAX_PATH);
}

static int encfs_getattr(const char *path, struct stat *stbuf)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    res = lstat(path, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_access(const char *path, int mask)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    res = access(path, mask);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_readlink(const char *path, char *buf, size_t size)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    res = readlink(path, buf, size - 1);
    if (res == -1)
        return -errno;

    buf[res] = '\0';
    return 0;
}


static int encfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    dp = opendir(path);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int encfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    /* On Linux this could just be 'mknod(path, mode, rdev)' but this
       is more portable */
    if (S_ISREG(mode)) {
        res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close(res);
    } else if (S_ISFIFO(mode))
        res = mkfifo(path, mode);
    else
        res = mknod(path, mode, rdev);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_mkdir(const char *path, mode_t mode)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    res = mkdir(path, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_unlink(const char *path)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    res = unlink(path);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_rmdir(const char *path)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    res = rmdir(path);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_symlink(const char *from, const char *to)
{
    int res;

    res = symlink(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_rename(const char *from, const char *to)
{
    int res;

    res = rename(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_link(const char *from, const char *to)
{
    int res;

    res = link(from, to);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_chmod(const char *path, mode_t mode)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    res = chmod(path, mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_chown(const char *path, uid_t uid, gid_t gid)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    res = lchown(path, uid, gid);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_truncate(const char *path, off_t size)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    res = truncate(path, size);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_utimens(const char *path, const struct timespec ts[2])
{
    int res;
    struct timeval tv[2];

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    tv[0].tv_sec = ts[0].tv_sec;
    tv[0].tv_usec = ts[0].tv_nsec / 1000;
    tv[1].tv_sec = ts[1].tv_sec;
    tv[1].tv_usec = ts[1].tv_nsec / 1000;

    res = utimes(path, tv);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_open(const char *path, struct fuse_file_info *fi)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;


    res = open(path, fi->flags);
    if (res == -1)
        return -errno;

    close(res);

    return 0;
}

static int encfs_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi)
{
    int fd;
    int res;
    //int enc;
    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    encfs_decrypt(full_path);

    (void) fi;
    fd = open(path, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);

    //if(enc == ENCRYPTED) {
        //return 
    encfs_encrypt(full_path);
    //}

    return res;
}

static int encfs_write(const char *path, const char *buf, size_t size,
        off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int res;
    //int enc;
    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    encfs_decrypt(full_path);

    (void) fi;
    fd = open(path, O_WRONLY);
    if (fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);

    //if(enc == ENCRYPTED) {
        //return 
    encfs_encrypt(full_path);
    //}

    return res;
}

static int encfs_statfs(const char *path, struct statvfs *stbuf)
{
    int res;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    res = statvfs(path, stbuf);
    if (res == -1)
        return -errno;

    return 0;
}

static int encfs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    int res;
    res = creat(path, mode);
    if(res == -1)
        return -errno;

    close(res);
    encfs_encrypt(full_path);
    setxattr(full_path,ATTR_NAME,"true",4,0);
    return 0;
}


static int encfs_release(const char *path, struct fuse_file_info *fi)
{
    /* Just a stub.	 This method is optional and can safely be left
       unimplemented */

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    (void) path;
    (void) fi;
    return 0;
}

static int encfs_fsync(const char *path, int isdatasync,
        struct fuse_file_info *fi)
{
    /* Just a stub.	 This method is optional and can safely be left
       unimplemented */

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    (void) path;
    (void) isdatasync;
    (void) fi;
    return 0;
}

#ifdef HAVE_SETXATTR
static int encfs_setxattr(const char *path, const char *name, const char *value,
        size_t size, int flags)
{

    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    int res = lsetxattr(path, name, value, size, flags);
    if (res == -1)
        return -errno;
    return 0;
}

static int encfs_getxattr(const char *path, const char *name, char *value,
        size_t size)
{   
    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    int res = lgetxattr(path, name, value, size);
    if (res == -1)
        return -errno;
    return res;
}

static int encfs_listxattr(const char *path, char *list, size_t size)
{ 
    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    int res = llistxattr(path, list, size);
    if (res == -1)
        return -errno;
    return res;
}

static int encfs_removexattr(const char *path, const char *name)
{ 
    char full_path[MAX_PATH];
    encfs_mkpath(full_path,path);
    path = full_path;

    int res = lremovexattr(path, name);
    if (res == -1)
        return -errno;
    return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations encfs_oper = {
    .getattr	= encfs_getattr,
    .access		= encfs_access,
    .readlink	= encfs_readlink,
    .readdir	= encfs_readdir,
    .mknod		= encfs_mknod,
    .mkdir		= encfs_mkdir,
    .symlink	= encfs_symlink,
    .unlink		= encfs_unlink,
    .rmdir		= encfs_rmdir,
    .rename		= encfs_rename,
    .link		= encfs_link,
    .chmod		= encfs_chmod,
    .chown		= encfs_chown,
    .truncate	= encfs_truncate,
    .utimens	= encfs_utimens,
    .open		= encfs_open,
    .read		= encfs_read,
    .write		= encfs_write,
    .statfs		= encfs_statfs,
    .create         = encfs_create,
    .release	= encfs_release,
    .fsync		= encfs_fsync,
#ifdef HAVE_SETXATTR
    .setxattr	= encfs_setxattr,
    .getxattr	= encfs_getxattr,
    .listxattr	= encfs_listxattr,
    .removexattr	= encfs_removexattr,
#endif
};

int main(int argc, char *argv[])
{
    umask(0);
    if (argc < 4) {
        fprintf(stderr, "Usage: %s %s \n", argv[0], "<Key> <Mirror Directory> <Mount Point>");
        return EXIT_FAILURE;
    }

    encfs_options *options; 

    options = malloc(sizeof(encfs_options));

    options->root_dir = realpath(argv[argc-2], NULL);
    options->key = argv[argc-3];
    argv[argc-3] = argv[argc-1];
    argv[argc-2] = NULL;
    argv[argc-1] = NULL;
    argc -= 2;

    int ret = fuse_main(argc, argv, &encfs_oper, options);

    free(options);

    return ret;
}

/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  Minor modifications and note by Andy Sayler (2012) <www.andysayler.com>

  More modifications by Thomas Lillis

  Source: fuse-2.8.7.tar.gz examples directory
  http://sourceforge.net/projects/fuse/files/fuse-2.X/

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags` efs.c -o efs `pkg-config fuse --libs`

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
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#define MAX_PATH 4096

typedef struct {
    char *root_dir;
    char *key;
} efs_options;

static void efs_mkpath(char fpath[MAX_PATH], const char *path)
{
    efs_options *options = (efs_options *) (fuse_get_context()->private_data);
    strcpy(fpath, options->root_dir);
    strncat(fpath, path, MAX_PATH);
}

static int efs_getattr(const char *path, struct stat *stbuf)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_access(const char *path, int mask)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_readlink(const char *path, char *buf, size_t size)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int efs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
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

static int efs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
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

static int efs_mkdir(const char *path, mode_t mode)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_unlink(const char *path)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_rmdir(const char *path)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_symlink(const char *from, const char *to)
{
	int res;
    
	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_rename(const char *from, const char *to)
{
	int res;
    
	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_link(const char *from, const char *to)
{
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_chmod(const char *path, mode_t mode)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_chown(const char *path, uid_t uid, gid_t gid)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_truncate(const char *path, off_t size)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_utimens(const char *path, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
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

static int efs_open(const char *path, struct fuse_file_info *fi)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int efs_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	(void) fi;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int efs_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	(void) fi;
	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int efs_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int efs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {

    (void) fi;
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

    int res;
    res = creat(path, mode);
    if(res == -1)
	return -errno;

    close(res);

    return 0;
}


static int efs_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	(void) path;
	(void) fi;
	return 0;
}

static int efs_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
static int efs_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
    
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int efs_getxattr(const char *path, const char *name, char *value,
			size_t size)
{   
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int efs_listxattr(const char *path, char *list, size_t size)
{ 
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int efs_removexattr(const char *path, const char *name)
{ 
    char full_path[MAX_PATH];
    efs_mkpath(full_path,path);
    path = full_path;

	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations efs_oper = {
	.getattr	= efs_getattr,
	.access		= efs_access,
	.readlink	= efs_readlink,
	.readdir	= efs_readdir,
	.mknod		= efs_mknod,
	.mkdir		= efs_mkdir,
	.symlink	= efs_symlink,
	.unlink		= efs_unlink,
	.rmdir		= efs_rmdir,
	.rename		= efs_rename,
	.link		= efs_link,
	.chmod		= efs_chmod,
	.chown		= efs_chown,
	.truncate	= efs_truncate,
	.utimens	= efs_utimens,
	.open		= efs_open,
	.read		= efs_read,
	.write		= efs_write,
	.statfs		= efs_statfs,
	.create         = efs_create,
	.release	= efs_release,
	.fsync		= efs_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= efs_setxattr,
	.getxattr	= efs_getxattr,
	.listxattr	= efs_listxattr,
	.removexattr	= efs_removexattr,
#endif
};

int main(int argc, char *argv[])
{
	umask(0);
	return fuse_main(argc, argv, &efs_oper, NULL);
}

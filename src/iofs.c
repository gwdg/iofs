/*
  This module is based on a FUSE example module but allows mounting
	a subtree instead of the full file system.


  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>
	Copyright (C) 2016       Julian Kunkel <kunkel@dkrz.de>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 * @tableofcontents
 *
 * \section section_compile compiling this example
 *
 * gcc -Wall iofs.c `pkg-config fuse3 --cflags --libs` -lulockmgr -o iofs
 *
 * ./iofs -o kernel_cache -o max_write=$((1024*1024*10)) -o big_writes -o allow_other  <TARGET> <SRC>
 *  ./iofs -o allow_other,entry_timeout=360,ro,attr_timeout=360,ac_attr_timeout=360,negative_timeout=360,kernel_cache -o max_idle_threads=16 -f /dev/test $PWD/src
 */

// Use if you want to use the kernel cache...
#define USE_KERNEL_CACHE

#define FUSE_USE_VERSION 36
#define HAVE_UTIMENSAT

#define debug(...)

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fuse.h>

#ifdef HAVE_LIBULOCKMGR
#include <ulockmgr.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <sys/file.h> /* flock(2) */

#include <iofs-monitor.h>

#define START_TIMER() monitor_activity_t activity;  monitor_start_activity(& activity)
#define END_TIMER(name, count) monitor_end_activity(& activity, & counter[COUNTER_ ## name], count)

static char * prefix;
typedef char name_buffer[PATH_MAX];


static void prepare_path(const char * path, char * out){
//	debug("%s\n", __PRETTY_FUNCTION__);
	snprintf(out, PATH_MAX, "%s/%s", prefix, path);
}

static int cache_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
  START_TIMER();
//	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
	name_buffer name_buf;
	prepare_path(path, name_buf);
	res = lstat(name_buf, stbuf);
  END_TIMER(GETATTR, 1);
	if (res == -1){
    return -errno;
  }
	return 0;
}

static int cache_access(const char *path, int mask)
{
  START_TIMER();
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
	name_buffer name_buf;
	prepare_path(path, name_buf);

	res = access(name_buf, mask);
  END_TIMER(ACCESS, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_readlink(const char *path, char *buf, size_t size)
{
  START_TIMER();
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
	name_buffer name_buf;
	prepare_path(path, name_buf);

	res = readlink(name_buf, buf, size - 1);
  END_TIMER(READLINK, 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}

struct cache_dirp {
	DIR *dp;
	struct dirent *entry;
	off_t offset;
};

static int cache_opendir(const char *path, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
  START_TIMER();
	name_buffer name_buf;
	prepare_path(path, name_buf);

	int res;
	struct cache_dirp *d = malloc(sizeof(struct cache_dirp));
	if (d == NULL)
		return -ENOMEM;

	d->dp = opendir(name_buf);
  END_TIMER(OPENDIR, 1);
	if (d->dp == NULL) {
		res = -errno;
		free(d);
		return res;
	}
	d->offset = 0;
	d->entry = NULL;

	fi->fh = (unsigned long) d;
	return 0;
}

static inline struct cache_dirp *get_dirp(struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	return (struct cache_dirp *) (uintptr_t) fi->fh;
}

static int cache_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{
	debug("%s\n", __PRETTY_FUNCTION__);
  START_TIMER();
	struct cache_dirp *d = get_dirp(fi);

	(void) path;
	if (offset != d->offset) {
		seekdir(d->dp, offset);
		d->entry = NULL;
		d->offset = offset;
	}
	while (1) {
		struct stat st;
		off_t nextoff;
		enum fuse_fill_dir_flags fill_flags = 0;

		if (!d->entry) {
			d->entry = readdir(d->dp);
			if (!d->entry)
				break;
		}
#ifdef HAVE_FSTATAT
		if (flags & FUSE_READDIR_PLUS) {
			int res;

			res = fstatat(dirfd(d->dp), d->entry->d_name, &st,
				      AT_SYMLINK_NOFOLLOW);
			if (res != -1)
				fill_flags |= FUSE_FILL_DIR_PLUS;
		}
#endif
		if (!(fill_flags & FUSE_FILL_DIR_PLUS)) {
			memset(&st, 0, sizeof(st));
			st.st_ino = d->entry->d_ino;
			st.st_mode = d->entry->d_type << 12;
		}
		nextoff = telldir(d->dp);
		if (filler(buf, d->entry->d_name, &st, nextoff, fill_flags))
			break;

		d->entry = NULL;
		d->offset = nextoff;
	}

  END_TIMER(READDIR, 1);

	return 0;
}

static int cache_releasedir(const char *path, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
  START_TIMER();
	struct cache_dirp *d = get_dirp(fi);
	(void) path;
	closedir(d->dp);
	free(d);
  END_TIMER(RELEASEDIR, 1);
	return 0;
}

static int cache_mkdir(const char *path, mode_t mode)
{
	debug("%s\n", __PRETTY_FUNCTION__);
  START_TIMER();
	int res;
	name_buffer name_buf;
	prepare_path(path, name_buf);

	res = mkdir(name_buf, mode);
  END_TIMER(MKDIR, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_unlink(const char *path)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	res = unlink(name_buf);
  END_TIMER(UNLINK, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_rmdir(const char *path)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	res = rmdir(name_buf);
  END_TIMER(RMDIR, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_symlink(const char *from, const char *to)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
  START_TIMER();
	name_buffer name_buf1;
	prepare_path(from, name_buf1);
	name_buffer name_buf2;
	prepare_path(to, name_buf2);

	res = symlink(name_buf1, name_buf2);
  END_TIMER(SYMLINK, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_rename(const char *from, const char *to, unsigned int flags)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
  START_TIMER();
	name_buffer name_buf1;
	prepare_path(from, name_buf1);
	name_buffer name_buf2;
	prepare_path(to, name_buf2);

	/* When we have renameat2() in libc, then we can implement flags */
	if (flags)
		return -EINVAL;

	res = rename(name_buf1, name_buf2);
  END_TIMER(RENAME, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_link(const char *from, const char *to)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
  START_TIMER();
	name_buffer name_buf1;
	prepare_path(from, name_buf1);
	name_buffer name_buf2;
	prepare_path(to, name_buf2);

	res = link(name_buf1, name_buf2);
  END_TIMER(LINK, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_chmod(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	res = chmod(name_buf, mode);
  END_TIMER(CHMOD, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	res = lchown(name_buf, uid, gid);
  END_TIMER(CHOWN, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_truncate(const char *path, off_t size, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	res = truncate(name_buf, size);
  END_TIMER(TRUNCATE, 1);
	if (res == -1)
		return -errno;

	return 0;
}


#ifdef HAVE_UTIMENSAT
static int cache_utimens(const char *path, const struct timespec ts[2], struct fuse_file_info *fi){
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, name_buf, ts, AT_SYMLINK_NOFOLLOW);
  END_TIMER(UTIMENS, 1);
	if (res == -1)
		return -errno;

	return 0;
}
#endif

static int cache_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int fd;
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	fd = open(name_buf, fi->flags, mode);
  END_TIMER(CREATE, 1);
	if (fd == -1)
		return -errno;

	fi->fh = fd;
	return 0;
}

static int cache_open(const char *path, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int fd;
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	fd = open(name_buf, fi->flags);
  END_TIMER(OPEN, 1);
	if (fd == -1)
		return -errno;

	fi->fh = fd;
	return 0;
}

static int cache_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
  START_TIMER();
	int res;

	(void) path;
	res = pread(fi->fh, buf, size, offset);
  END_TIMER(READ, res);
	if (res == -1)
		res = -errno;

	return res;
}

static int cache_read_buf(const char *path, struct fuse_bufvec **bufp, size_t size, off_t offset, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
  START_TIMER();
	struct fuse_bufvec *src;

	(void) path;

	src = malloc(sizeof(struct fuse_bufvec));
	if (src == NULL)
		return -ENOMEM;

	*src = FUSE_BUFVEC_INIT(size);

	src->buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
	src->buf[0].fd = fi->fh;
	src->buf[0].pos = offset;

	*bufp = src;
  END_TIMER(READ_BUF, size);

	return 0;
}

static int cache_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
  debug("%s\n", __PRETTY_FUNCTION__);
  START_TIMER();
	int res;

	(void) path;
	res = pwrite(fi->fh, buf, size, offset);
  END_TIMER(WRITE, res);
	if (res == -1)
		res = -errno;

	return res;
}

static int cache_write_buf(const char *path, struct fuse_bufvec *buf, off_t offset, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
  START_TIMER();
  size_t size = fuse_buf_size(buf);
	struct fuse_bufvec dst = FUSE_BUFVEC_INIT(size);

	(void) path;

	dst.buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
	dst.buf[0].fd = fi->fh;
	dst.buf[0].pos = offset;

	int ret = fuse_buf_copy(&dst, buf, FUSE_BUF_SPLICE_NONBLOCK);
  END_TIMER(WRITE_BUF, size);
  return ret;
}

static int cache_statfs(const char *path, struct statvfs *stbuf)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;

  START_TIMER();
	res = statvfs(path, stbuf);
  END_TIMER(STATFS, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_flush(const char *path, struct fuse_file_info *fi)
{
	int res;

	(void) path;
	/* This is called from every close on an open file, so call the
	   close on the underlying filesystem.	But since flush may be
	   called multiple times for an open file, this must not really
	   close the file.  This is important if used on a network
	   filesystem like NFS which flush the data/metadata on close() */
  START_TIMER();
	res = close(dup(fi->fh));
  END_TIMER(FLUSH, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static int cache_release(const char *path, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	(void) path;
  START_TIMER();
	close(fi->fh);
  END_TIMER(RELEASE, 1);
	return 0;
}

static int cache_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
	(void) path;
  START_TIMER();
#ifndef HAVE_FDATASYNC
	(void) isdatasync;
#else
	if (isdatasync)
		res = fdatasync(fi->fh);
	else
#endif
		res = fsync(fi->fh);
  END_TIMER(FSYNC, 1);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int cache_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	(void) path;

	if (mode)
		return -EOPNOTSUPP;

  START_TIMER();
	int ret = -posix_fallocate(fi->fh, offset, length);
  END_TIMER(FALLOCATE, 1);
  return ret;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int cache_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	int res = lsetxattr(name_buf, name, value, size, flags);
  END_TIMER(SETXATTR, 1);
	if (res == -1)
		return -errno;
	return 0;
}

static int cache_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	int res = lgetxattr(name_buf, name, value, size);
  END_TIMER(GETXATTR, 1);
	if (res == -1)
		return -errno;
	return res;
}

static int cache_listxattr(const char *path, char *list, size_t size)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	int res = llistxattr(name_buf, list, size);
  END_TIMER(LISTXATTR, 1);
	if (res == -1)
		return -errno;
	return res;
}

static int cache_removexattr(const char *path, const char *name)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	name_buffer name_buf;
  START_TIMER();
	prepare_path(path, name_buf);

	int res = lremovexattr(name_buf, name);
  END_TIMER(REMOVEXATTR, 1);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

#ifdef HAVE_LIBULOCKMGR
static int cache_lock(const char *path, struct fuse_file_info *fi, int cmd,
		    struct flock *lock)
{
	debug("%s\n", __PRETTY_FUNCTION__);

  START_TIMER();
	int ret = ulockmgr_op(fi->fh, cmd, lock, &fi->lock_owner,
			   sizeof(fi->lock_owner));
  END_TIMER(LOCK, 1);
  return ret;
}
#endif

static int cache_flock(const char *path, struct fuse_file_info *fi, int op)
{
	debug("%s\n", __PRETTY_FUNCTION__);
	int res;
  START_TIMER();
	res = flock(fi->fh, op);
  END_TIMER(FLOCK, 1);
	if (res == -1)
		return -errno;

	return 0;
}

static void *cache_init (struct fuse_conn_info *conn, struct fuse_config *cfg){

  // see documentation of options in fuse.h
  //cfg->direct_io = 1;
  //cfg->kernel_cache = 1;
  cfg->auto_cache = 0;

  printf("IOFS init\n");
  printf("intr: %d\n", cfg->intr);
  printf("remember: %d\n", cfg->remember);
  printf("intr: %d\n", cfg->intr);
  printf("hard_remove: %d\n", cfg->hard_remove);
  printf("use_ino: %d\n", cfg->use_ino);
  printf("readdir_ino: %d\n", cfg->readdir_ino);
  printf("direct_io: %d\n", cfg->direct_io);
  printf("kernel_cache: %d\n", cfg->kernel_cache);
  printf("auto_cache: %d\n", cfg->auto_cache);
  printf("ac_attr_timeout_set: %d\n", cfg->ac_attr_timeout_set);
  printf("nullpath_ok: %d\n", cfg->nullpath_ok);

  printf("ac_attr_timeout: %f\n", cfg->ac_attr_timeout);
  printf("entry_timeout: %f\n", cfg->entry_timeout);
  printf("negative_timeout: %f\n", cfg->negative_timeout);
  printf("attr_timeout: %f\n", cfg->attr_timeout);


  monitor_options_t options = {
    .logfile = "/tmp/iofs.log",
    .verbosity = 10,
    .interval = 1,
    .es_server = "localhost",
    .es_server_port = "8080",
    .es_uri = "iofs/",
    .detailed_logging = 1,
  };

  monitor_init(& options);
  return NULL;
}

static void cache_destroy (void *private_data){
  monitor_finalize();
}

static struct fuse_operations cache_oper = {
	.getattr	= cache_getattr,
	.access		= cache_access,
	.readlink	= cache_readlink,
	.opendir	= cache_opendir,
	.readdir	= cache_readdir,
	.releasedir	= cache_releasedir,
	.mkdir		= cache_mkdir,
	.symlink	= cache_symlink,
	.unlink		= cache_unlink,
	.rmdir		= cache_rmdir,
	.rename		= cache_rename,
	.link		= cache_link,
	.chmod		= cache_chmod,
	.chown		= cache_chown,
	.truncate	= cache_truncate,
#ifdef HAVE_UTIMENSAT
	.utimens	= cache_utimens,
#endif
	.create		= cache_create,
	.open		= cache_open,
	.read		= cache_read,
	.write		= cache_write,
#ifdef USE_KERNEL_CACHE
	.read_buf	= cache_read_buf,
	.write_buf	= cache_write_buf,
#endif
	.statfs		= cache_statfs,
	.flush		= cache_flush,
	.release	= cache_release,
	.fsync		= cache_fsync,
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate	= cache_fallocate,
#endif
#ifdef HAVE_SETXATTR
	.setxattr	= cache_setxattr,
	.getxattr	= cache_getxattr,
	.listxattr	= cache_listxattr,
	.removexattr	= cache_removexattr,
#endif
#ifdef HAVE_LIBULOCKMGR
	.lock		= cache_lock,
#endif
	.flock		= cache_flock,
  .init     = cache_init,
  .destroy  = cache_destroy
};

int main(int argc, char *argv[])
{
	if (argc < 3){
		printf("Synopsis: %s [options] <TARGET> <SOURCE PATH>\n", argv[0]);
		exit(1);
	}
	printf("IOFS-trace version 0.8 %s on %s\n", argv[0], argv[argc -1]);

	umask(0);
	prefix = argv[argc -1];

  int create_table = strcmp(argv[1], "--create-table") == 0;
  //register_iofs_counters(create_table);

	int ret = fuse_main(argc -1, argv, &cache_oper, NULL);
  return 0;
}

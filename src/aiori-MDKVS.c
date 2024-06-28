#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include "ior.h"
#include "aiori.h"
#include "mdkvs_api.h"

#define		MDKVS_VERSTR	"0.1"
#define 	OPSSD_PATH  	"/dev/nvme4n1p2"

static aiori_xfer_hint_t *hints = NULL;

struct MDKVS_File {
	char fpath[512];
};

typedef struct mdkvs_option {
	char *kvdb_path;
	unsigned reset_db;
} mdkvs_option_t;

option_help *
MDKVS_options(aiori_mod_opt_t **init_backend_options,
	aiori_mod_opt_t *init_values)
{
	mdkvs_option_t *o = malloc(sizeof(*o));

	if (init_values != NULL)
		memcpy(o, init_values, sizeof(*o));
	else
		memset(o, 0, sizeof(*o));

	// if (o->chunk_size > 0)
	// 	finchfs_set_chunk_size(o->chunk_size);

	*init_backend_options = (aiori_mod_opt_t *)o;

	option_help h[] = {
		{0, "mdkvs.kvdb_path", "Path for uDepot KV database",
			OPTION_REQUIRED_ARGUMENT, 's', &o->kvdb_path},
	    {0, "mdkvs.reset_db", "Reset uDepot database",
			OPTION_FLAG, 'd', &o->reset_db},
	    LAST_OPTION
	};
	option_help *help = malloc(sizeof(h));
	memcpy(help, h, sizeof(h));

	return (help);
}

void
MDKVS_xfer_hints(aiori_xfer_hint_t *params)
{
	hints = params;
}

void
MDKVS_initialize(aiori_mod_opt_t * param)
{
	mdkvs_option_t * opt = (mdkvs_option_t *) param;
	INFOF("MDKVS start using %suDepot at %s",
		opt->reset_db ? "NEW " : "", opt->kvdb_path);
	mdkvs_init(opt->kvdb_path, opt->reset_db);
}

void
MDKVS_finalize()
{
	INFO("MDKVS finished");
	mdkvs_deinit();
}

aiori_fd_t *
MDKVS_create(char *fn, int flags, aiori_mod_opt_t *param)
{
	struct MDKVS_File *fd;
	if (hints->dryRun) return NULL;

	int rv = mdkvs_create(fn);
	if (rv < 0) ERRF("mdkvs_create(%s) failed: %s", fn, strerror(-rv));
	fd = malloc(sizeof(*fd));
	strncpy(fd->fpath, fn, sizeof(fd) - 1);
	return ((aiori_fd_t *)fd);
}

aiori_fd_t *
MDKVS_open(char *fn, int flags, aiori_mod_opt_t *param)
{
	struct MDKVS_File *fd;
	if (hints->dryRun) return NULL;

	int rv = mdkvs_open(fn);
	if (rv < 0) ERRF("mdkvs_open(%s) failed: %s", fn, strerror(-rv));
	fd = malloc(sizeof(*fd));
	strncpy(fd->fpath, fn, sizeof(fd) - 1);
	return ((aiori_fd_t *)fd);
}

IOR_offset_t
MDKVS_xfer(int access, aiori_fd_t *fd, IOR_size_t *buffer,
	IOR_offset_t len, IOR_offset_t offset, aiori_mod_opt_t *param)
{
	// struct FINCHFS_File *bf = (struct FINCHFS_File *)fd;
	// ssize_t r;

	// if (hints->dryRun)
	// 	return (len);

	// switch (access) {
	// case WRITE:
	// 	r = finchfs_pwrite(bf->fd, buffer, len, offset);
	// 	break;
	// default:
	// 	r = finchfs_pread(bf->fd, buffer, len, offset);
	// 	break;
	// }
	// return (r);
	return 0;
}

void
MDKVS_close(aiori_fd_t *fd, aiori_mod_opt_t *param)
{
	struct MDKVS_File *mfd = (struct MDKVS_File *)fd;
	if (hints->dryRun) return;
	mdkvs_close(mfd->fpath);
	free(mfd);
}

void
MDKVS_delete(char *fn, aiori_mod_opt_t *param)
{
	if (hints->dryRun) return;
	mdkvs_delete(fn);
}

char *
MDKVS_version()
{
	return ((char *)MDKVS_VERSTR);
}

void
MDKVS_fsync(aiori_fd_t *fd, aiori_mod_opt_t *param)
{
	/*
	 * No need to do fsync, as we are not writing files 
	 *	and inodes are persisted/synced upon write.
	 */
}

IOR_offset_t
MDKVS_get_file_size(aiori_mod_opt_t *param, char *fn)
{
	struct stat st;
	if (hints->dryRun) return 0;

	int rv = mdkvs_stat(fn, &st);
	if (rv < 0) return rv;
	return (st.st_size);
}

int
MDKVS_statfs(const char *fn, ior_aiori_statfs_t *st, aiori_mod_opt_t *param)
{
	// if (hints->dryRun) return (0);
	return 0;
}

int
MDKVS_mkdir(const char *fn, mode_t mode, aiori_mod_opt_t *param)
{
	if (hints->dryRun) return 0;
	int rv = mdkvs_mkdir(fn);
	INFOF("mkdir %s: %s", fn, rv < 0 ? strerror(-rv) : "OK");
	return rv;
}

int
MDKVS_rename(const char *oldfile, const char *newfile, aiori_mod_opt_t * param)
{
	if (hints->dryRun) return (0);
	int rv = mdkvs_rename(oldfile, newfile);
	if (rv == -EISDIR)
		ERR("Renaming dir is not implemented.");
	return rv;
}

int
MDKVS_rmdir(const char *fn, aiori_mod_opt_t *param)
{
	if (hints->dryRun) return 0;
	int rv = mdkvs_rmdir(fn);
	INFOF("rmdir %s: %s", fn, rv < 0 ? strerror(-rv) : "OK");
	return rv;
}

int
MDKVS_access(const char *fn, int mode, aiori_mod_opt_t *param)
{
	struct stat sb;
	if (hints->dryRun) return 0;
	return (mdkvs_stat(fn, &sb));
}

int
MDKVS_stat(const char *fn, struct stat *buf, aiori_mod_opt_t *param)
{
	if (hints->dryRun) return 0;
	return (mdkvs_stat(fn, buf));
}

void
MDKVS_sync(aiori_mod_opt_t *param)
{
	if (hints->dryRun) return;
}

ior_aiori_t mdkvs_aiori = {
	.name = "MDKVS",
	.name_legacy = NULL,
	.create = MDKVS_create,
	.open = MDKVS_open,
	.xfer_hints = MDKVS_xfer_hints,
	.xfer = MDKVS_xfer,
	.close = MDKVS_close,
	.remove = MDKVS_delete,
	.get_version = MDKVS_version,
	.fsync = MDKVS_fsync,
	.get_file_size = MDKVS_get_file_size,
	.statfs = MDKVS_statfs,
	.mkdir = MDKVS_mkdir,
	.rename = MDKVS_rename,
	.rmdir = MDKVS_rmdir,
	.access = MDKVS_access,
	.stat = MDKVS_stat,
	.initialize = MDKVS_initialize,
	.finalize = MDKVS_finalize,
	.get_options = MDKVS_options,
	.sync = MDKVS_sync,
	.enable_mdtest = true,
};

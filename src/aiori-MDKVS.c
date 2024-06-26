#include <sys/types.h>
#include <sys/stat.h>
#include "ior.h"
#include "aiori.h"
#include "mdkvs_api.h"

#define		MDKVS_VERSTR	"0.1"
#define 	OPSSD_PATH  	"/dev/nvme4n1p2"

static aiori_xfer_hint_t *hints = NULL;

struct MDKVS_File {
	int fd;
};

struct mdkvs_option {
	size_t chunk_size;
};

option_help *
MDKVS_options(aiori_mod_opt_t **init_backend_options,
	aiori_mod_opt_t *init_values)
{
	struct mdkvs_option *o = malloc(sizeof(*o));

	if (init_values != NULL)
		memcpy(o, init_values, sizeof(*o));
	else
		memset(o, 0, sizeof(*o));

	// if (o->chunk_size > 0)
	// 	finchfs_set_chunk_size(o->chunk_size);

	*init_backend_options = (aiori_mod_opt_t *)o;

	option_help h[] = {
	    {0, "mdkvs.chunk_size", "chunk size", OPTION_FLAG, 'd',
		    &o->chunk_size},
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
MDKVS_initialize()
{
	// finchfs_init(NULL);
	INFO("MDKVS start");
	mdkvs_init(OPSSD_PATH, 0);
}

void
MDKVS_finalize()
{
	// finchfs_term();
	INFO("MDKVS finished");
	mdkvs_deinit();
}

aiori_fd_t *
MDKVS_create(char *fn, int flags, aiori_mod_opt_t *param)
{
	// struct FINCHFS_File *bf;
	// int fd;

	// if (hints->dryRun)
	// 	return (NULL);

	// fd = finchfs_create(fn, flags, 0664);
	// if (fd < 0)
	// 	ERR("finchfs_create failed");
	// bf = malloc(sizeof(*bf));
	// bf->fd = fd;

	// return ((aiori_fd_t *)bf);
	return NULL;
}

aiori_fd_t *
MDKVS_open(char *fn, int flags, aiori_mod_opt_t *param)
{
	// struct FINCHFS_File *bf;
	// int fd;

	// if (hints->dryRun)
	// 	return (NULL);

	// fd = finchfs_open(fn, flags);
	// if (fd < 0)
	// 	ERR("finchfs_open failed");
	// bf = malloc(sizeof(*bf));
	// bf->fd = fd;

	// return ((aiori_fd_t *)bf);
	return NULL;
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
	// struct FINCHFS_File *bf = (struct FINCHFS_File *)fd;

	// if (hints->dryRun)
	// 	return;

	// finchfs_close(bf->fd);
	// free(bf);
}

void
MDKVS_delete(char *fn, aiori_mod_opt_t *param)
{
	// if (hints->dryRun)
	// 	return;

	// finchfs_unlink(fn);
}

char *
MDKVS_version()
{
	return ((char *)MDKVS_VERSTR);
}

void
MDKVS_fsync(aiori_fd_t *fd, aiori_mod_opt_t *param)
{
	// struct FINCHFS_File *bf = (struct FINCHFS_File *)fd;

	// if (hints->dryRun)
	// 	return;

	// finchfs_fsync(bf->fd);
}

IOR_offset_t
MDKVS_get_file_size(aiori_mod_opt_t *param, char *fn)
{
	struct stat st;
	// int r;

	// if (hints->dryRun)
	// 	return (0);

	// r = finchfs_stat(fn, &st);
	// if (r < 0)
	// 	return (r);

	return (st.st_size);
}

int
MDKVS_statfs(const char *fn, ior_aiori_statfs_t *st, aiori_mod_opt_t *param)
{
	// if (hints->dryRun)
	// 	return (0);

	return (0);
}

int
MDKVS_mkdir(const char *fn, mode_t mode, aiori_mod_opt_t *param)
{
	// if (hints->dryRun)
	// 	return (0);

	// return (finchfs_mkdir(fn, mode));
	return 0;
}

int
MDKVS_rename(const char *oldfile, const char *newfile, aiori_mod_opt_t * param)
{
	// if (hints->dryRun)
	// 	return (0);
	// return (finchfs_rename(oldfile, newfile));
	return 0;
}

int
MDKVS_rmdir(const char *fn, aiori_mod_opt_t *param)
{
	// if (hints->dryRun)
	// 	return (0);

	// return (finchfs_rmdir(fn));
	return 0;
}

int
MDKVS_access(const char *fn, int mode, aiori_mod_opt_t *param)
{
	// struct stat sb;

	// if (hints->dryRun)
	// 	return (0);

	// return (finchfs_stat(fn, &sb));
	return 0;
}

int
MDKVS_stat(const char *fn, struct stat *buf, aiori_mod_opt_t *param)
{
	// if (hints->dryRun)
	// 	return (0);

	// return (finchfs_stat(fn, buf));
	return 0;
}

void
MDKVS_sync(aiori_mod_opt_t *param)
{
	if (hints->dryRun)
		return;

	return;
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

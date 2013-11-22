#ifndef DMAPI_H
#define DMAPI_H

/*
  This software is licensed under the
    GPLv3 license.
*/

#include <fuse.h>

/*
 * A structure for command line options
 */ 
typedef struct {
	char * email;
	char * keyfile;
} dm_opts;

void 	api_free(); 
int 	api_init(dm_opts * opts);

int 	api_readdir(const char *, fuse_fill_dir_t, void * buf);
void	api_getusage(long long *, long long * );

int	api_open(const char *, const char *);
int 	api_getattr(const char *path, struct stat *stbuf);
void	api_upload(const char *,  const char *);
int	api_create(const char *);
int	api_createdir(const char *);
int	api_removefile(const char *);
int	api_removedir(const char *);
int     api_rename_v2(const char *, const char *);

/* Some constant and utility define */
#define FALSE 0
#define TRUE  1
#define MIN(A,B) (A<B ? A : B)

#endif
// DMAPI_H



/***************************************

  DR: Interazione con il sito
  
  This software is licensed under the 
  GPLv3 license.

***************************************/

#include "dmapi.h"
#include "dmcollection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>

//#include <libxml/hash.h>

#include <libapp/app.h>
#include <libapp/list.h>
#include <rfjson.h>
#include <rfhttp.h>

/* Building blocks for Dropmark API endpoints
   and return codes
*/
#define DM_ENDPOINT "https://api.dropmark.com/v1/"
#define DM_LOGIN    DM_ENDPOINT "auth"
#define DM_DIRS     DM_ENDPOINT "collections"
#define DM_FILES    DM_DIRS "/%s/items"

//    CALLS
#define API_ENDPOINT	"https://api.box.com/2.0/"
#define API_UPLOAD      "https://upload.box.com/api/2.0/files/content"
#define API_UPLOAD_VER  "https://upload.box.com/api/2.0/files/%s/content"
//    UTILS
#define BUFSIZE 1024

#define LOCKDIR(dir) pthread_mutex_lock(&dir->cmux);
#define UNLOCKDIR(dir) pthread_mutex_unlock(&dir->cmux); 

#define is_root(path) (!strcmp(path,"/"))
#define is_dir(path) (strstr(path+1,"/")==NULL)
#define DM_DIRPERM (S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define DM_FILEPERM (S_IFREG | S_IRUSR | S_IXUSR | S_IRGRP | S_IROTH)

/* globals, written during initialization */
char *auth_token = NULL, *api_key = NULL;
long long tot_space, used_space;
list * collections = NULL;
//struct box_options_t options;

void api_free()
{
	if(auth_token) free(auth_token);
	syslog(LOG_INFO, "Unmounting Dropmarkfs");
	closelog();
  
}

/* 
 * Handle authentication to Dropmark
 *
 * the API call returns also account info (i.e. storage quota)
 * that we read and use
 */
int authorize_user()
{
	int res = 0;
	char * buf = NULL, * mail = NULL, * passwd = NULL;
	jobj * account, * usage;
	FILE * tf;
	postdata_t postpar=post_init();
	char * header = malloc(128);

	printf("Email: "); mail = app_term_readline();
	passwd = app_term_askpass("Password:");
	mail[strlen(mail)-1] = 0;

	tf = fopen("_api_token", "r");
	if(tf) api_key = app_term_readline_from(tf);
	fclose(tf);
	snprintf(header, 128, "X-API-Key: %s", api_key);
	set_auth_header(header);

	post_add(postpar, "email", mail);
	post_add(postpar, "password", passwd);
	buf = http_post(DM_LOGIN, postpar);
	
	//printf("Response: %s\n", buf);
	account = jobj_parse(buf);
	if(account) {
		char * id = jobj_getval(account, "id");
		char * token = jobj_getval(account, "token");
		
                if(!id || !token) {
                	fprintf(stderr, "Authentication failed: %s\n", jobj_getval(account, "message"));
                	res = 1;
                } else {
                	auth_token = malloc(strlen(id)+strlen(token)+2);
                	sprintf(auth_token, "%s:%s", id, token);
                	set_auth_info(auth_token);
                	syslog(LOG_DEBUG,"Auth token is %s", auth_token);
                	
                	// read storage space info from "usage" object
                	usage = jobj_get(account, "usage");
                	tot_space = jobj_getlong(usage, "quota");
                	used_space = jobj_getlong(usage, "used");
                	
                	free(id);
                	free(token);
                }                
                
		jobj_free(account);
	} else {
        	fprintf(stderr, "Unable to parse server response.\n");
        	res = 1;
	}

	post_free(postpar);
	if(buf)    free(buf);
	if(mail)   free(mail);
	if(passwd)   free(passwd);
	return res;
}

void api_getusage(long long * tot_sp, long long * used_sp)
{
  *tot_sp = tot_space;
  *used_sp = used_space;
}

int api_createdir(const char * path)
{
	int res = 0;
	//boxpath * bpath;
	//boxdir *newdir;
	//boxfile * aFile;
	char * dirid = NULL, *buf = NULL;
	jobj * folder = NULL;
	char fields[BUFSIZE]="";
/*
	bpath = boxpath_from_string(path);
	if(bpath->dir) {
		snprintf(fields, BUFSIZE, POST_CREATEDIR,
		        bpath->base, bpath->dir->id);

		buf = http_post_fields(API_LS, fields);

		if(buf) { 
			folder = jobj_parse(buf); 
			free(buf);
		}
		if(folder) {
		        dirid = jobj_getval(folder, "id");
        		free(folder);
                }

		if(!dirid) {
			res = -EINVAL;
			boxpath_free(bpath);
			return res;
		}

		// add 1 entry to the hash table
		newdir = boxdir_create();
		newdir->id = dirid;
		//xmlHashAddEntry(allDirs, path, newdir);
		// upd parent
		aFile = boxfile_create(bpath->base);
		aFile->id = strdup(dirid);
		LOCKDIR(bpath->dir);
		list_append(bpath->dir->folders, aFile);
		UNLOCKDIR(bpath->dir);    
		// invalidate cached parent entry
		cache_rm(bpath->dir->id);
	} else {
		syslog(LOG_WARNING, "UH oh... wrong path %s",path);
		res = -EINVAL;
	}
	boxpath_free(bpath);
*/
	return res;
}


int api_create(const char * path)
{
  int res = 0;
  //boxpath * bpath = boxpath_from_string(path);
  //boxfile * aFile;
/*
  if(bpath->dir) {
    aFile = boxfile_create(bpath->base);
    LOCKDIR(bpath->dir);
    list_append(bpath->dir->files,aFile);
    UNLOCKDIR(bpath->dir);
  } else {
    syslog(LOG_WARNING, "UH oh... wrong path %s",path);
    res = -ENOTDIR;
  }
  boxpath_free(bpath);
*/  
  return res;
}


jobj * get_collections( )
{
	char * buf = NULL;
	jobj * res = NULL;
	
	buf = http_fetch(DM_DIRS);
	if(buf) {
	        res = jobj_parse(buf);
	        free(buf);
	}
	return res;
}

jobj * get_collection(const char * id )
{
	char * buf = NULL;
	jobj * res = NULL;
	
	buf = http_fetchf(DM_FILES, id);
	if(buf) {
	        res = jobj_parse(buf);
	        free(buf);
	}
	return res;
}

int api_open(const char * path, const char * pfile)
{
	collection * c;
	item * f;
        pathtype type = parse_path(path, collections, &c, &f);

        if(type!=PATH_FILE) return -ENOENT;
        return http_fetch_file(f->url, pfile, FALSE);
}

int api_readdir(const char * path, fuse_fill_dir_t filler, void * buf)
{
	list_iter it;
	collection * c;
	item * f;
	pathtype type;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	type = parse_path(path, collections, &c, &f);	

	switch(type) {
	        case PATH_ROOT:
        		it = list_get_iter(collections);
        		for(; it; it = list_iter_next(it)) {
        			c = (collection*) list_iter_getval(it);
        			filler(buf, c->name, NULL, 0);
        		}
        		return 0;
        		
                case PATH_DIR:
		        LOCKDIR(c);
		        it = list_get_iter(c->files);
	                for(; it; it = list_iter_next(it)) {
				f = (item*) list_iter_getval(it);
				filler(buf, f->name, NULL, 0);
			}
			UNLOCKDIR(c);
			return 0;
                
                default: return -EINVAL;	        
	}
}

int api_getattr(const char *path, struct stat *stbuf)
{
	collection * c;
	item * f;
	pathtype type;
	
	memset(stbuf, 0, sizeof(struct stat));
	stbuf->st_uid = getuid();
	stbuf->st_gid = getgid();
	
	type = parse_path(path, collections, &c, &f);	
	switch(type) {
	        case PATH_ROOT:
	                stbuf->st_nlink = 2 + list_size(collections);
	                stbuf->st_mode = DM_DIRPERM;
	                stbuf->st_size = 0;
	                return 0;
                case PATH_DIR:
        		stbuf->st_nlink = 2 + list_size(c->files);
        		stbuf->st_mode = DM_DIRPERM;
        		stbuf->st_size = 0;
        		stbuf->st_ctime = c->ctime;
        		stbuf->st_mtime = c->mtime;
        		stbuf->st_atime = c->mtime;
        		return 0;
                case PATH_FILE:
                	stbuf->st_mode = DM_FILEPERM;
                	stbuf->st_size = f->size;
                	stbuf->st_ctime = c->ctime;
                	stbuf->st_mtime = c->mtime;
                	stbuf->st_atime = c->mtime;
                	stbuf->st_nlink = 1;
                	return 0;		                

	        default: return -ENOENT;
	}
}

int api_removedir(const char * path)
{
  int res = 0;
  char url[BUFSIZE]="";
  long sc;
  /*boxpath * bpath = boxpath_from_string(path);

  /* check that is a dir */
  /*
  if(!boxpath_getfile(bpath)) return -EINVAL;  
  if(!bpath->dir && !bpath->is_dir) return -ENOENT;

  snprintf(url, BUFSIZE, API_LS "%s", bpath->file->id);
  sc = http_delete(url);
  
  if(sc != 204) {
    res = -EPERM;
  }

  if(!res) {
    //remove it from parent's subdirs...
    LOCKDIR(bpath->dir);
    boxpath_removefile(bpath);
    UNLOCKDIR(bpath->dir);
    //...and from dir list
//    xmlHashRemoveEntry(allDirs, path, NULL);
    // invalidate parent cache entry
    cache_rm(bpath->dir->id);
  }
  
  boxpath_free(bpath);  
  */
  return res;
}

/*
 * Internal func to call the delete api on a file id.
 * api_removefile will call it several times if
 * splitfiles is on and the file has parts
 */
/*
int do_removefile_id(const char * id)
{
	int res = 0;
	long sc;
	char url[BUFSIZE]="";
	
	snprintf(url, BUFSIZE, API_FILES, id);
	sc = http_delete(url);
	if(sc != 204) res = -ENOENT;
	
	return res;
}
*/

int api_removefile(const char * path)
{
	int res = 0;
	/*
	boxpath * bpath = boxpath_from_string(path);

	if(!bpath->dir) res = -ENOENT;
	else {
		//remove it from box.net
		boxpath_getfile(bpath);
		res = do_removefile_id(bpath->file->id);

		if(res==0) {
        		used_space -= bpath->file->size;

			//remove it from the list
			LOCKDIR(bpath->dir);
			boxpath_removefile(bpath);
			UNLOCKDIR(bpath->dir);
			//invalidate cache entry
			cache_rm(bpath->dir->id);
		}
	}
	
	boxpath_free(bpath);
	*/
	return res;
}

/*
 * Move and rename funcs, new version
 */

//predeclaration
int do_api_move_id(int is_dir, const char * id, const char * dest, int is_rename);
/*
int do_api_move(boxpath * bsrc, boxpath * bdst)
{
	int res = 0;
	list_iter it;

	LOCKDIR(bsrc->dir);
	res = do_api_move_id(bsrc->is_dir, bsrc->file->id, bdst->dir->id, FALSE);
	if(!res) {
		boxfile * part;

		boxpath_removefile(bsrc);
		LOCKDIR(bdst->dir);
		list_append((bsrc->is_dir ? bdst->dir->folders : bdst->dir->files),
			bsrc->file);
		UNLOCKDIR(bdst->dir);
	}
	UNLOCKDIR(bsrc->dir);
	
	return res;
}

int do_api_move_id(int is_dir, const char * id, const char * dest, int is_rename)
{
        char url[BUFSIZE], fields[BUFSIZE], * buf, * type;
        int res = 0;
        jobj * obj;
        
        if(is_dir) {
                snprintf(url, BUFSIZE, API_LS "%s", id);
        } else {
                snprintf(url, BUFSIZE, API_FILES, id);
        }
        if(is_rename) {
		snprintf(fields, BUFSIZE, POST_RENAME, dest);
	} else {
		snprintf(fields, BUFSIZE, POST_MOVE, dest);
	}

        buf = http_put_fields(url, fields);
        obj = jobj_parse(buf);
        type = jobj_getval(obj, "type");
        if(strcmp(type, is_dir ? "folder" : "file")) res = -EINVAL;
        
        if(type) free(type);
        free(buf);
        jobj_free(obj);
        return res;
}

int do_api_rename(boxpath * bsrc, boxpath * bdst)
{
	int res;
	
	LOCKDIR(bsrc->dir);
	res = do_api_move_id(bsrc->is_dir, bsrc->file->id, bdst->base, TRUE);
	if(!res) {
		boxfile * part;
		char * newname;
		list_iter it;
		int ind=1;

		boxpath_renamefile(bsrc, bdst->base);
	}
	UNLOCKDIR(bsrc->dir);

	return res;
}
*/

int api_rename_v2(const char * from, const char * to)
{
	int res = 0;
	/*
	boxpath * bsrc = boxpath_from_string(from);
	boxpath * bdst = boxpath_from_string(to);
	if(!boxpath_getfile(bsrc)) return -EINVAL; 
	boxpath_getfile(bdst);

	if(bsrc->dir!=bdst->dir) {
		res=do_api_move(bsrc, bdst);
	}
	if(!res && strcmp(bsrc->base, bdst->base)) {
		res = do_api_rename(bsrc,bdst);
	}
	if(!res && bsrc->is_dir) {
	    boxtree_movedir(from, to);
	}

	// invalidate cache entries
	cache_rm(bsrc->dir->id);
	if(bsrc->dir!=bdst->dir) cache_rm(bdst->dir->id);
	boxpath_free(bsrc);
	boxpath_free(bdst);
	*/
	return res;
}

void api_upload(const char * path, const char * tmpfile)
{
	postdata_t buf = post_init();
	char * res = NULL, * pr = NULL;
	off_t fsize, oldsize = 0;
	collection * c = NULL;
	item * f;
	int oldver;
	pathtype type;
	
	/* this could give us PATH_BAD, but fill c */
	type = parse_path(path, collections, &c, &f);
	if(!c) {
		post_free(buf);
		syslog(LOG_ERR, "Can't upload file %s", path);
		return;
	}
	//...
	
	post_free(buf);
/*
  boxpath * bpath = boxpath_from_string(path);

  if(bpath->dir) {
    post_add(buf, "parent_id", bpath->dir->id);
    fsize = filesize(tmpfile);
    oldver = (boxpath_getfile(bpath) && bpath->file->size);
    if(oldver) oldsize = bpath->file->size;
    
    if(fsize) {
    	//normal upload
    	post_addfile(buf, bpath->base, tmpfile);
    	if(oldver) {
    	        char url[BUFSIZE]="";
    	        snprintf(url, BUFSIZE, API_UPLOAD_VER, bpath->file->id);
    	        res = http_postfile(url, buf);
        } else {
                res = http_postfile(API_UPLOAD, buf);
        }

	set_filedata(bpath ,res, fsize);
	free(res);
    }

    // update used space    
    used_space = used_space - oldsize + fsize;
    
  } else {
    syslog(LOG_ERR,"Couldn't upload file %s",bpath->base);
  }
  post_free(buf);
  boxpath_free(bpath);
  */
}

/*
 * Login to Dropmark and fetch collections info
 */
int api_init(int* argc, char*** argv) {

	int res = 0;

	/* parse command line arguments */
	//if (parse_options (argc, argv, &options))
	//	return 1;  
  
	openlog("dropmarkfs", LOG_PID, LOG_USER);
	//cache_init(options.cache_dir, options.expire_time);

       	//set_conn_reuse(TRUE);
	res = authorize_user();
        
  	if(!res) {  	        
  		jobj * o;
  		list_iter it;
  		collection * c;

  		o = get_collections();
  		if(o) {
  			collections = load_collections(o);
	  		jobj_free(o);
	  		
	  		it = list_get_iter(collections);
	  		for(; it; it = list_iter_next(it))
	  		{
				c = (collection*)list_iter_getval(it);
				o = get_collection(c->id);
				init_collection(c, o);
				jobj_free(o);
	  		}
	  		syslog(LOG_INFO, "Dropmarkfs init done succesfully");
		} else res = 1;     
	}

	//set_conn_reuse(FALSE);
	return res;
}

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
#include <rfutils.h>

/* Building blocks for Dropmark API endpoints
   and return codes
*/
#define DM_ENDPOINT "https://api.dropmark.com/v1/"
#define DM_LOGIN    DM_ENDPOINT "auth"
#define DM_DIRS     DM_ENDPOINT "collections"
#define DM_FILES    DM_DIRS "/%s/items"

//    UTILS
#define BUFSIZE 1024

#define LOCKDIR(dir) pthread_mutex_lock(&dir->cmux);
#define UNLOCKDIR(dir) pthread_mutex_unlock(&dir->cmux); 

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
int authorize_user(char * mail, char * key_file)
{
	int res = 0;
	char * buf = NULL, * passwd = NULL;
	jobj * account, * usage;
	FILE * tf;
	postdata_t postpar=post_init();
	char * header = malloc(128);

	tf = fopen(key_file, "r");
	if(!tf) {
	        fprintf(stderr, "Can't open key file %s\n", key_file);
	        return 1;
	}
	if(tf) api_key = app_term_readline_from(tf);
	fclose(tf);

	if(!mail) {
	        printf("Email: "); 
	        mail = app_term_readline();
	        mail[strlen(mail)-1] = 0;

        }
	passwd = app_term_askpass("Password:");

	snprintf(header, 128, "X-API-Key: %s", api_key);
	set_auth_header(header);

	post_add(postpar, "email", mail);
	post_add(postpar, "password", passwd);
	buf = http_post(DM_LOGIN, postpar);
	
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
	jobj * folder = NULL;
	char * name, *res;
	objtype type;
	collection * c;
	postdata_t postpar=post_init();
        
        name = path+1;
        if(strchr(name, '/')) {
                syslog(LOG_INFO, "Invalid path (nested dirs not allowed): %s", path);
                return -EINVAL;
        }
        c = create_collection(collections, name);
        if(!c) {
		syslog(LOG_WARNING, "Cannot create dir %s", path);
		return -EINVAL;
	}
	post_add(postpar, "name", name);
	res = http_post(DM_DIRS, postpar);
	folder = jobj_parse(res);
	if(!folder) return -EINVAL;
	c->id = jobj_getval(folder, "id");
	
	post_free(postpar);
	return 0;
}


int api_create(const char * path)
{
	collection * c;
	item * f;
        pathtype type = parse_path(path, collections, &c, &f);

	if(c) {
                f = create_item(c, path);
                return 0;
	}
	
	syslog(LOG_WARNING, "UH oh... wrong path %s",path);
	return -ENOTDIR;
}


jobj * get_collections( )
{
	char * buf = NULL;
	jobj * res = NULL;
	
	buf = http_fetch(DM_DIRS "?count=1000");
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
	
	buf = http_fetchf(DM_FILES "?count=1000", id);
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
	        default:
	                return -ENOENT;
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
 * Move and rename:
 * not yet implemented because API calls are missing
 */

int api_rename(const char * from, const char * to)
{
	int res = 0;
	return res;
}

void api_upload(const char * path, const char * tmpfile)
{
	postdata_t buf = post_init();
	char * res = NULL/*, * pr = NULL*/;
	//off_t fsize, oldsize = 0;
	collection * c = NULL;
	item * f;
	//int oldver;
	char url[BUFSIZE]="";
	pathtype type;
	jobj * new_item;
	
	type = parse_path(path, collections, &c, &f);
	if(!c) {
		post_free(buf);
		syslog(LOG_ERR, "Can't upload file %s", path);
		return;
	}
	post_add(buf, "name", f->name);
	post_addfile(buf, "content", f->name, tmpfile);
	snprintf(url, BUFSIZE, DM_FILES, c->id);
	res = http_postfile(url, buf);
	new_item = jobj_parse(res);
	//TODO: CHECK_ERR
	f->id = jobj_getval(new_item, "id");
	f->url = jobj_getval(new_item, "content");
	f->size = filesize(tmpfile);
	/* update used space */
	used_space += filesize(tmpfile);
	
	post_free(buf);
	jobj_free(new_item);
}

/*
 * Login to Dropmark and fetch collections info
 */
int api_init(dm_opts * o) {

	int res = 0;
	

	openlog("dropmarkfs", LOG_PID, LOG_USER);

       	//set_conn_reuse(TRUE);
	res = authorize_user(o->email, o->keyfile);
        
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

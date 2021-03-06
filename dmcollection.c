#include "dmcollection.h"

#include <string.h>
#include <libgen.h>

#include <rfutils.h>

/*
 * Settings
 */
static int orig_names;

/*
 * Internal and helper functions
 */

/* set file name behaviour */
void use_original_names(int orig)
{
	orig_names = orig;
}

// create and initialize a new collection
collection * collection_new()
{
	collection * res = malloc(sizeof(collection));
	memset(res, 0, sizeof(collection));
	pthread_mutex_init(&res->cmux, NULL);
	res->files = list_new();

	return res;
}

// create and initialize a new item
item * item_new()
{
	item * res = malloc(sizeof(item));
	memset(res, 0, sizeof(item));
	
	return res;
}

// comparator for list_insert_sorted
int compare_name(void * n1, void * n2)
{
	dm_base * c1 = (dm_base*)n1, * c2 = (dm_base*)n2;
	return strcmp(c1->name, c2->name);
}

// find a file/collection by name
dm_base * find_in_list(list/*<dm_base>*/ * c, const char * name)
{
	dm_base * ac;
	list_iter it = list_get_iter(c);
	int sc;
	
	
	for(; it; it = list_iter_next(it)) {
		ac = (dm_base*)list_iter_getval(it);
		sc = strcmp(name, ac->name);
		if(sc==0) return ac;
		if(sc<0) break;
	}
	return NULL;
}

/*
 * free-safe basename
 * does not modify source string path
 */
char * dm_basename(const char * path)
{
	char * last_slash;
	
	if(!path) return NULL;
	last_slash = strrchr(path, '/');
	
	if(!last_slash) return strdup(path);
	return strdup(last_slash+1);	
}
  

/*
 * Public functions
 */

item *       create_item(collection * c, const char * n)
{
	item * f = item_new();
	time_t now = time(NULL);
	
	f->name = dm_basename(n);
	f->ctime = now;
	f->mtime = now;
	
	list_insert_sorted_comp(c->files, f, compare_name);
	
	return f;
}

collection * create_collection(list * c, const char * n)
{
	collection * nc = collection_new();
	time_t now = time(NULL);
	
	nc->name = dm_basename(n);
	nc->ctime = now;
	nc->mtime = now;
	list_insert_sorted_comp(c, nc, compare_name);
	
	return nc;
}

list * load_collections(jobj * collections)
{
	list * res = list_new();
	list_iter ci = list_get_iter(collections->children);
	collection * c;
	jobj * item;

	for(; ci; ci = list_iter_next(ci))
	{
		item = (jobj*) list_iter_getval(ci);
		c = collection_new();
		c->id = jobj_getval(item, "id");
		c->name = jobj_getval(item, "name");
		c->ctime = jobj_gettime(item, "created_at");
		c->mtime = jobj_gettime(item, "updated_at");
		list_insert_sorted_comp(res, c, compare_name);
	}
	return res;
}

void   init_collection(collection * c, jobj* items)
{
	list_iter it = list_get_iter(items->children);
	jobj * file;
	item * ai;

	for(; it; it = list_iter_next(it)) {
		file = (jobj*)list_iter_getval(it);
		ai = item_new();
		ai->id = jobj_getval(file, "id");
		ai->url = jobj_getval(file, "content");
		ai->name = orig_names ? dm_basename(ai->url) : jobj_getval(file, "name");
		ai->size = jobj_getlong(file, "size");
		ai->ctime = jobj_gettime(file, "created_at");
		ai->mtime = jobj_gettime(file, "updated_at");
		list_insert_sorted_comp(c->files, ai, compare_name);
	}        
}

void delete_collection(list * c, collection * obj)
{
	list_delete_item(c, obj);
}

void delete_item(collection * c, item * obj)
{
	list_delete_item(c->files, obj);
}

collection * find_collection(list * c, const char * name)
{
	return (collection*)find_in_list(c, name);
}

item * find_item(collection * c, const char * name)
{
	return (item*)find_in_list(c->files, name);
}

pathtype parse_path(const char * path, list * collections, collection ** coll, item ** f)
{
	char * dir, * base;

	if(!path) return PATH_BAD;

	if(!strcmp(path, "/")) { // is the root directory
		return PATH_ROOT;
	}
	
	if(!strstr(path+1,"/")) { // is a directory
		*coll = find_collection(collections, path+1);
		if(*coll==NULL) return PATH_BAD;
		return PATH_DIR;
	}
	
	dir = dirname(strdup(path));
	base = dm_basename(path);
	*coll = find_collection(collections, dir+1);
	/* dont't free(dir); */
	
	if(*coll==NULL)	{
		free(base);
		return PATH_BAD;
	}
	*f = find_item(*coll, base);
	if(base) free(base);
	if(*f==NULL) return PATH_BAD;
	// it's a file
	return PATH_FILE;
}

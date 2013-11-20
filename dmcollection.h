#ifndef DMCOLLECTION_H
#define DMCOLLECTION_H

/*
 *
 *  (C) 2013 Domenico Rotiroti 
 *   This software is licensed under the GPLv3 license.
 *
 *  Interface to Dropmark "Collections" and files
 */

#include <libapp/list.h>
#include <rfjson.h>

#include <time.h>
#include <pthread.h>

/* Types definition */
typedef struct {
	char * id;
	char * name;
	time_t ctime;
	time_t mtime;
} dm_base;

typedef struct {
/* struct dm_base */
	char * id;
	char * name;
	time_t ctime;
	time_t mtime;
/* -------------- */
	list * files;
	pthread_mutex_t cmux;
} collection;

typedef struct {
/* struct dm_base */
	char * id;
	char * name;
	time_t ctime;
	time_t mtime;
/* -------------- */
	char * url;
	long long size;
} item;

typedef enum { PATH_ROOT, PATH_DIR, PATH_FILE, PATH_BAD } pathtype;

/*
 * Initialize the list of collection from
 * their json representation
 */
list * load_collections(jobj * collections);
void   init_collection(collection * c, jobj* items);

/*
 * Create a new object (collection/item)
 * and add it to the parent list
 */
item *       create_item(collection * c, const char * name);
collection * create_collection(list * c, const char * name);

/*
 * Find a collection by name
 */
collection * find_collection(list * c, const char * name);

/*
 * Fine a file by name
 */
item * find_item(collection * c, const char * name);

/*
 * Analyze a path to see if it's a directory, file, etc
 * and returns the appropriate pathtype
 *
 * Fills the <coll> and <f> parameters with the
 * ojects representing the path parts, if they exist
 */
pathtype parse_path(const char * path, list * collections, collection ** coll, item ** f);

#endif
// DMCOLLECTION_H



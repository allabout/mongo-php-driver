/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2010 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* All About Co., Ltd. program created shinya furuwata(@f_prg) */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include <stdio.h>
#include <fcntl.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include <zlib.h>
#include <time.h>
#include "ext/standard/crc32.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "ext/standard/php_var.h"
#include "ext/standard/php_smart_str.h"

#include "ext/session/php_session.h"
#include "./mongo_session.h"

// ----------------------------------------------
// mongo.so extension global value extern
// ----------------------------------------------
ZEND_EXTERN_MODULE_GLOBALS(mongo);

// ----------------------------------------------
// refer global value
// ----------------------------------------------
extern zend_class_entry *mongo_ce_Mongo, 
                        *mongo_ce_DB, 
                        *mongo_ce_ConnectionException, 
                        *mongo_ce_BinData;


#ifdef HAVE_MONGO_SESSION


// ----------------------------------------------
// session module value
// ----------------------------------------------
ps_module ps_mod_mongo = {
	PS_MOD(mongo)
};

// ==============================================
// session functions
// ==============================================
// ----------------------------------------------
// session open function
//#define PS_OPEN_ARGS void **mod_data, const char *save_path, const char *session_name TSRMLS_DC
// ----------------------------------------------
/* {{{ PS_OPEN */
PS_OPEN_FUNC(mongo)
{
    php_mongo_db_session    *session;       // session
    zval                    *return_value;  // return value
    zval                    *options;       // option

    // allocate session
    session = (php_mongo_db_session*)emalloc(sizeof(php_mongo_db_session));
    if(!session){
        PS_SET_MOD_DATA(NULL);
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "session open error!");
        return FAILURE;
    }
    memset(session, 0, sizeof(php_mongo_db_session));

    // MongoDB connect value
    MAKE_STD_ZVAL(session->url);
    MAKE_STD_ZVAL(session->db);
    MAKE_STD_ZVAL(session->collection);
    MAKE_STD_ZVAL(session->expire);
    MAKE_STD_ZVAL(session->replica_name);
    ZVAL_STRING(session->url, MonGlo(session_url), 1);
    ZVAL_STRING(session->db, MonGlo(session_db), 1);
    ZVAL_STRING(session->collection, MonGlo(session_collection), 1);
    ZVAL_LONG(session->expire, MonGlo(session_expire));
    ZVAL_STRING(session->replica_name, MonGlo(session_replica_name), 1);

    // mongo instance object
    MAKE_STD_ZVAL(session->mongo);
    object_init_ex(session->mongo, mongo_ce_Mongo);
    MAKE_STD_ZVAL(return_value);
    ZVAL_NULL(return_value);
    MAKE_STD_ZVAL(options);
    array_init(options);
    if(strlen(MonGlo(session_replica_name)) != 0){
        add_assoc_string(options, "replicaSet", session->replica_name, 1);
    }
    MONGO_METHOD2(Mongo, __construct, return_value, session->mongo, session->url, options);
    FREE_ZVAL(return_value);

    // set session
    PS_SET_MOD_DATA(session);

    return SUCCESS;
}
/* }}} */

// ----------------------------------------------
// session read function
//#define PS_READ_ARGS void **mod_data, const char *key, char **val, int *vallen TSRMLS_DC
// ----------------------------------------------
/* {{{ PS_READ_FUNC */
PS_READ_FUNC(mongo)
{
    php_mongo_db_session    *session = PS_GET_MOD_DATA();   // session
    zval                    *db_z;                          // db
    zval                    *collection_z;                  // collection
    zval                    *cursor_z;                      // cursor
    zval                    *find_option_z;                 // cursor method find option
    zval                    *read_data;                     // read data
    zval                    **hash_rv;                      // hash read value
    zval                    *next;                          // cursor loop value

    // call selectDB
    MAKE_STD_ZVAL(db_z);
    MONGO_METHOD1(Mongo, selectDB, db_z, session->mongo, session->db);

    // call selectCollection
    MAKE_STD_ZVAL(collection_z);
    MONGO_METHOD1(MongoDB, selectCollection, collection_z, db_z, session->collection);

    // get document from cursor.
    MAKE_STD_ZVAL(find_option_z);
    array_init(find_option_z);
    add_assoc_string(find_option_z, "session_id", (char*)key, 1);
    MAKE_STD_ZVAL(cursor_z);
    ZVAL_NULL(cursor_z);
    MONGO_METHOD1(MongoCollection, find, cursor_z, collection_z, find_option_z);
    MAKE_STD_ZVAL(read_data);
    ZVAL_NULL(read_data);
    if(!ZVAL_IS_NULL(cursor_z)){
        // cursor loop
        MAKE_STD_ZVAL(next);
        ZVAL_NULL(next);
        MONGO_METHOD(MongoCursor, getNext, next, cursor_z);
        while (!IS_SCALAR_P(next)) {
            // hash find : session_id
            if (zend_hash_find(HASH_P(next), "session_id", strlen("session_id") + 1, (void**)&hash_rv) == FAILURE){
                MONGO_METHOD(MongoCursor, getNext, next, cursor_z);
                continue;
            }
            // session_id string compare
            if(strcmp(Z_STRVAL_PP(hash_rv), key) != 0){
                MONGO_METHOD(MongoCursor, getNext, next, cursor_z);
                continue;
            }
            // get session_data
            hash_rv = NULL;
            if (zend_hash_find(HASH_P(next), "session_data", strlen("session_data") + 1, (void**)&hash_rv) == FAILURE){
                MONGO_METHOD(MongoCursor, getNext, next, cursor_z);
                continue;
            }
            if(Z_TYPE_PP(hash_rv) != IS_OBJECT){
                MONGO_METHOD(MongoCursor, getNext, next, cursor_z);
                continue;
            }
            read_data = zend_read_property(mongo_ce_BinData, *hash_rv, "bin", strlen("bin"), NOISY TSRMLS_CC);
            break;
        }
        // cannot found session_id
        if(ZVAL_IS_NULL(read_data)){
            ZVAL_STRING(read_data, "", 1);
        }
    }
    // cannot get cursor
    else{
        ZVAL_STRING(read_data, "", 1);
    }

    // send to php session value
    *val = Z_STRVAL_P(read_data);
    *vallen = Z_STRLEN_P(read_data);
//    FREE_ZVAL(read_data);

    // finalize
//    FREE_ZVAL(find_option_z);
//    FREE_ZVAL(cursor_z);
    FREE_ZVAL(collection_z);
    FREE_ZVAL(db_z);

    return SUCCESS;
}
/* }}} */

// ----------------------------------------------
// session write function
//#define PS_WRITE_ARGS void **mod_data, const char *key, const char *val, const int vallen TSRMLS_DC
// ----------------------------------------------
/* {{{ PS_WRITE_FUNC */
PS_WRITE_FUNC(mongo)
{
    php_mongo_db_session    *session = PS_GET_MOD_DATA();   // session
    zval                    *return_value;                  // return value
    zval                    *db_z;                          // db
    zval                    *collection_z;                  // collection
    zval                    *cursor_z;                      // cursor
    zval                    *find_option_z;                 // cursor method find option
    zval                    **hash_rv;                      // hash read value
    zval                    *next;                          // cursor loop value
    zval                    *write_data;                    // insert write data
    zval                    *session_id;                    // insert session_id
    zval                    *session_bin_data;              // insert binary data
    zval                    *options;                       // option value
    int                     is_update = 0;                  // update flag
    time_t                  timer;                          // timer
    struct tm               *time_info;                     // time info
    zval                    **ctstr_u;                      // created time str
    zval                    **ctval_u;                      // created time value

    // call selectDB
    MAKE_STD_ZVAL(db_z);
    ZVAL_NULL(db_z);
    MONGO_METHOD1(Mongo, selectDB, db_z, session->mongo, session->db);

    // call selectCollection
    MAKE_STD_ZVAL(collection_z);
    ZVAL_NULL(collection_z);
    MONGO_METHOD1(MongoDB, selectCollection, collection_z, db_z, session->collection);

    // call find and get cursor
    MAKE_STD_ZVAL(find_option_z);
    array_init(find_option_z);
    add_assoc_string(find_option_z, "session_id", (char*)key, 1);
    MAKE_STD_ZVAL(cursor_z);
    ZVAL_NULL(cursor_z);
    MONGO_METHOD1(MongoCollection, find, cursor_z, collection_z, find_option_z);
    if(ZVAL_IS_NULL(cursor_z)){
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "mongodb collection cursor get error!");
        return FAILURE;
    }

    // cursor loop
    ctstr_u = NULL;
    ctval_u = NULL;
    MAKE_STD_ZVAL(next);
    ZVAL_NULL(next);
    MONGO_METHOD(MongoCursor, getNext, next, cursor_z);
    while (!IS_SCALAR_P(next)) {
        // hash find : session_id
        if (zend_hash_find(HASH_P(next), "session_id", strlen("session_id") + 1, (void**)&hash_rv) == FAILURE){
            MONGO_METHOD(MongoCursor, getNext, next, cursor_z);
            continue;
        }
        // session_id string compare check
        if(strcmp(Z_STRVAL_PP(hash_rv), key) != 0){
            MONGO_METHOD(MongoCursor, getNext, next, cursor_z);
            continue;
        }
        // get created_time_value and created_time_string
        if (zend_hash_find(HASH_P(next), "created_time_value", strlen("created_time_value") + 1, (void**)&ctval_u) == FAILURE){
            MONGO_METHOD(MongoCursor, getNext, next, cursor_z);
            continue;
        }
        if( zend_hash_find(HASH_P(next), "created_time_string", strlen("created_time_string") + 1, (void**)&ctstr_u) == FAILURE){
            MONGO_METHOD(MongoCursor, getNext, next, cursor_z);
            continue;
        }

        // set update flag
        is_update = 1;
        break;
    }

    // create write data
    MAKE_STD_ZVAL(write_data);
    array_init(write_data);

    if(vallen != 0){
        // session id
        add_assoc_string(write_data, "session_id", (char*)key, 1);

        // session binary object
        MAKE_STD_ZVAL(session_bin_data);
        object_init_ex(session_bin_data, mongo_ce_BinData);
        zend_update_property_stringl(mongo_ce_BinData, session_bin_data, "bin", strlen("bin"), (char*)val, vallen TSRMLS_CC);
        zend_update_property_long(mongo_ce_BinData, session_bin_data, "type", strlen("type"), BSON_STRING TSRMLS_CC);
        add_assoc_zval(write_data, "session_data", session_bin_data);

        // created_time, updated_time
        time(&timer);
        if(is_update){
            timer = Z_LVAL_PP(ctval_u);
            add_assoc_long(write_data, "created_time_value", timer);
            char *p = Z_STRVAL_PP(ctstr_u);
            add_assoc_string(write_data, "created_time_string", p, 1);
            time(&timer);
        }else{
            add_assoc_long(write_data, "created_time_value", timer);
            add_assoc_string(write_data, "created_time_string", ctime(&timer), 1);
        }
        add_assoc_long(write_data, "updated_time_value", timer);
        add_assoc_string(write_data, "updated_time_string", ctime(&timer), 1);
    }

    // update row data
    if(is_update){
        // option
        MAKE_STD_ZVAL(options);
        array_init(options);
        add_assoc_string(options, "session_id", (char*)key, 1);

        // update
        MAKE_STD_ZVAL(return_value);
        ZVAL_NULL(return_value);
        MONGO_METHOD2(MongoCollection, update, return_value, collection_z, options, write_data);
    }
    // insert new session data
    else{
        if(vallen != 0){
            MAKE_STD_ZVAL(options);
            ZVAL_NULL(options);
            MAKE_STD_ZVAL(return_value);
            ZVAL_NULL(return_value);
            MONGO_METHOD2(MongoCollection, insert, return_value, collection_z, write_data, options);
        }
    }

    // finalize
    FREE_ZVAL(write_data);
    FREE_ZVAL(find_option_z);
    FREE_ZVAL(cursor_z);
    FREE_ZVAL(collection_z);
    FREE_ZVAL(db_z);

    return SUCCESS;
}
/* }}} */

// ----------------------------------------------
// session destroy function
//#define PS_DESTROY_ARGS void **mod_data, const char *key TSRMLS_DC
// ----------------------------------------------
/* {{{ PS_DESTROY_FUNC */
PS_DESTROY_FUNC(mongo)
{
    php_mongo_db_session    *session = PS_GET_MOD_DATA();   // session
    zval                    *return_value;                  // return value
    zval                    *db_z;                          // db
    zval                    *collection_z;                  // collection
    zval                    *cursor_z;                      // cursor
    zval                    *find_option_z;                 // cursor method find option
    zval                    *options;                       // option value

    // call selectDB
    MAKE_STD_ZVAL(db_z);
    ZVAL_NULL(db_z);
    MONGO_METHOD1(Mongo, selectDB, db_z, session->mongo, session->db);

    // call selectCollection
    MAKE_STD_ZVAL(collection_z);
    ZVAL_NULL(collection_z);
    MONGO_METHOD1(MongoDB, selectCollection, collection_z, db_z, session->collection);

    // remove session data
    // option
    MAKE_STD_ZVAL(options);
    array_init(options);
    add_assoc_string(options, "session_id", (char*)key, 1);
    MAKE_STD_ZVAL(return_value);
    ZVAL_NULL(return_value);
    MONGO_METHOD1(MongoCollection, remove, return_value, collection_z, options);

    return SUCCESS;
}
/* }}} */

// ----------------------------------------------
// session gabarge collection function
//#define PS_GC_ARGS void **mod_data, int maxlifetime, int *nrdels TSRMLS_DC
// ----------------------------------------------
/* {{{ PS_GC_FUNC */
PS_GC_FUNC(mongo)
{
    php_mongo_db_session    *session = PS_GET_MOD_DATA();   // session
    zval                    *return_value;                  // return value
    zval                    *db_z;                          // db
    zval                    *collection_z;                  // collection
    zval                    *cursor_z;                      // cursor
    zval                    *find_option_z;                 // cursor method find option
    zval                    *options;                       // option value
    time_t                  timer;                          // timer
    char                    buf[256];                       // string buffer

    // call selectDB
    MAKE_STD_ZVAL(db_z);
    ZVAL_NULL(db_z);
    MONGO_METHOD1(Mongo, selectDB, db_z, session->mongo, session->db);

    // call selectCollection
    MAKE_STD_ZVAL(collection_z);
    ZVAL_NULL(collection_z);
    MONGO_METHOD1(MongoDB, selectCollection, collection_z, db_z, session->collection);

    // remove session data
    // option
    time(&timer);
    MAKE_STD_ZVAL(options);
    array_init(options);
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "{ $lte : %u }", timer - MonGlo(session_expire));
    add_assoc_string(options, "updated_time_value", buf, 1);
    MAKE_STD_ZVAL(return_value);
    ZVAL_NULL(return_value);
    MONGO_METHOD1(MongoCollection, remove, return_value, collection_z, options);

    return SUCCESS;
}
/* }}} */

// ----------------------------------------------
// session close function
//#define PS_CLOSE_ARGS void **mod_data TSRMLS_DC
// ----------------------------------------------
/* {{{ PS_CLOSE_FUNC */
PS_CLOSE_FUNC(mongo)
{
    // session set null
    PS_SET_MOD_DATA(NULL);
    return SUCCESS;
}
/* }}} */

#endif  /* HAVE_MONGO_SESSION */
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

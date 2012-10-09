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

#ifndef _MONGO_SESSION_H
#define _MONGO_SESSION_H
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MONGO_SESSION


#include <tcrdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>


#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_mongo.h"

#include "SAPI.h" 
#include "php_variables.h"
#include "ext/standard/info.h"
#include "Zend/zend_API.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_interfaces.h"


#include "util/rs.h"
#include "util/parse.h"
#include "util/io.h"
#include "util/hash.h"
#include "util/connect.h"
#include "util/pool.h"
#include "util/link.h"
#include "util/server.h"
#include "util/log.h"

#include "ext/session/php_session.h"
#include "../mongo.h"
#include "../bson.h"
#include "../collection.h"
#include "../cursor.h"
#include "../mongo_types.h"
#include "../db.h"
#include "../php_mongo.h"

// ==============================================
// define
// ==============================================
// debug macro
#define _PRI_L printf("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);

// ==============================================
// session functions
// ==============================================
PS_FUNCS(mongo);

// ==============================================
// session data structure
// ==============================================
// mongo db session
typedef struct _php_mongo_db_session {
    zval         *mongo;           // mongo
    zval         *url;             // url
    zval         *db;              // db
    zval         *collection;      // collection
    zval         *expire;          // expire
    zval         *replica_name;    // replica set name
} php_mongo_db_session;


#endif  /* HAVE_MONGO_SESSION */
#endif	/* _MONGO_SESSION_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

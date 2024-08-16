
/*

<https://github.com/rafagafe/tiny-json>
     
  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
  SPDX-License-Identifier: MIT
  Copyright (c) 2016-2018 Rafa Garcia <rafagarcia77@gmail.com>.

  Permission is hereby  granted, free of charge, to any  person obtaining a copy
  of this software and associated  documentation files (the "Software"), to deal
  in the Software  without restriction, including without  limitation the rights
  to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
  copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
  IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
  FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
  AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
  LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
    
*/

#ifndef _TINY_JSON_H_
#define	_TINY_JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define json_containerOf( ptr, type, member ) \
    ((type*)( (char*)ptr - offsetof( type, member ) ))

/** @defgroup tinyJson Tiny JSON parser.
  * @{ */

/** Enumeration of codes of supported JSON properties types. */
typedef enum {
    JSON_OBJ, JSON_ARRAY, JSON_TEXT, JSON_BOOLEAN,
    JSON_INTEGER, JSON_REAL, JSON_NULL
} jsonType_t;

/** Structure to handle JSON properties. */
typedef struct json_s {
    struct json_s* sibling;
    const char* name;
    union {
      const char* value;
      struct {
        struct json_s* child;
        struct json_s* last_child;
        } c;
    } u;
    jsonType_t type;
} json_t;

/** Parse a string to get a json.
  * @param str String pointer with a JSON object. It will be modified.
  * @param mem Array of json properties to allocate.
  * @param qty Number of elements of mem.
  * @retval Null pointer if any was wrong in the parse process.
  * @retval If the parser process was successfully a valid handler of a json.
  *         This property is always unnamed and its type is JSON_OBJ. */
const json_t* json_create(char* str, json_t mem[], unsigned int qty);

/** Get the name of a json property.
  * @param json A valid handler of a json property.
  * @retval Pointer to null-terminated if property has name.
  * @retval Null pointer if the property is unnamed. */
static inline const char* json_getName(const json_t* json) {
    return json->name;
}

/** Get the value of a json property.
  * The type of property cannot be JSON_OBJ or JSON_ARRAY.
  * @param json A valid handler of a json property.
  * @return Pointer to null-terminated string with the value. */
static inline const char* json_getValue(const json_t* property) {
    return property->u.value;
}

/** Get the type of a json property.
  * @param json A valid handler of a json property.
  * @return The code of type.*/
static inline jsonType_t json_getType(const json_t* json) {
    return json->type;
}

/** Get the next sibling of a JSON property that is within a JSON object or array.
  * @param json A valid handler of a json property.
  * @retval The handler of the next sibling if found.
  * @retval Null pointer if the json property is the last one. */
static inline const json_t* json_getSibling(const json_t* json) {
    return json->sibling;
}

/** Search a property by its name in a JSON object.
  * @param obj A valid handler of a json object. Its type must be JSON_OBJ.
  * @param property The name of property to get.
  * @retval The handler of the json property if found.
  * @retval Null pointer if not found. */
const json_t* json_getProperty(const json_t* obj, const char* property);


/** Search a property by its name in a JSON object and return its value.
  * @param obj A valid handler of a json object. Its type must be JSON_OBJ.
  * @param property The name of property to get.
  * @retval If found a pointer to null-terminated string with the value.
  * @retval Null pointer if not found or it is an array or an object. */
const char* json_getPropertyValue(const json_t* obj, const char* property);

/** Get the first property of a JSON object or array.
  * @param json A valid handler of a json property.
  *             Its type must be JSON_OBJ or JSON_ARRAY.
  * @retval The handler of the first property if there is.
  * @retval Null pointer if the json object has not properties. */
static inline const json_t* json_getChild(const json_t* json) {
    return json->u.c.child;
}

/** Get the value of a json boolean property.
  * @param property A valid handler of a json object. Its type must be JSON_BOOLEAN.
  * @return The value stdbool. */
static inline bool json_getBoolean(const json_t* property) {
    return *property->u.value == 't';
}

/** Get the value of a json integer property.
  * @param property A valid handler of a json object. Its type must be JSON_INTEGER.
  * @return The value stdint. */
static inline int64_t json_getInteger(const json_t* property) {
    return atoll( property->u.value );
}

/** Get the value of a json real property.
  * @param property A valid handler of a json object. Its type must be JSON_REAL.
  * @return The value. */
static inline double json_getReal(const json_t* property) {
    return atof( property->u.value );
}


/** Structure to handle a heap of JSON properties. */
typedef struct jsonPool_s jsonPool_t;
struct jsonPool_s {
    json_t* (*init)( jsonPool_t* pool );
    json_t* (*alloc)( jsonPool_t* pool );
};

/** Parse a string to get a json.
  * @param str String pointer with a JSON object. It will be modified.
  * @param pool Custom json pool pointer.
  * @retval Null pointer if any was wrong in the parse process.
  * @retval If the parser process was successfully a valid handler of a json.
  *         This property is always unnamed and its type is JSON_OBJ. */
const json_t* json_createWithPool(char* str, jsonPool_t* pool);

/** @ } */

/** @defgroup makejoson Make JSON.
 * @{ */

/** Open a JSON object in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @return Pointer to the new end of JSON under construction. */
char* json_objOpen(char* dest, const char* name);

/** Close a JSON object in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @return Pointer to the new end of JSON under construction. */
char* json_objClose(char* dest);

/** Used to finish the root JSON object. After call json_objClose().
 * @param dest Pointer to the end of JSON under construction.
 * @return Pointer to the new end of JSON under construction. */
char* json_end(char* dest);

/** Open an array in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @return Pointer to the new end of JSON under construction. */
char* json_arrOpen(char* dest, const char* name);

/** Close an array in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @return Pointer to the new end of JSON under construction. */
char* json_arrClose(char* dest);

/** Add a text property in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @param value A valid null-terminated string with the value.
 *              Backslash escapes will be added for special characters.
 * @return Pointer to the new end of JSON under construction. */
char* json_str(char* dest, const char* name, const char* value);

/** Add a boolean property in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @param value Zero for false. Non zero for true.
 * @return Pointer to the new end of JSON under construction. */
char* json_bool(char* dest, const char* name, int value);

/** Add a null property in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @return Pointer to the new end of JSON under construction. */
char* json_null(char* dest, const char* name);

/** Add an integer property in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @param value Value of the property.
 * @return Pointer to the new end of JSON under construction. */
char* json_int(char* dest, const char* name, int value);

/** Add an unsigned integer property in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @param value Value of the property.
 * @return Pointer to the new end of JSON under construction. */
char* json_uint(char* dest, const char* name, unsigned int value);

/** Add a long integer property in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @param value Value of the property.
 * @return Pointer to the new end of JSON under construction. */
char* json_long(char* dest, const char* name, long int value);

/** Add an unsigned long integer property in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @param value Value of the property.
 * @return Pointer to the new end of JSON under construction. */
char* json_ulong(char* dest, const char* name, unsigned long int value);

/** Add a long long integer property in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @param value Value of the property.
 * @return Pointer to the new end of JSON under construction. */
char* json_verylong(char* dest, const char* name, long long int value);

/** Add a double precision number property in a JSON string.
 * @param dest Pointer to the end of JSON under construction.
 * @param name Pointer to null-terminated string or null for unnamed.
 * @param value Value of the property.
 * @return Pointer to the new end of JSON under construction. */
char* json_double(char* dest, const char* name, double value);

/** @ } */

#ifdef __cplusplus
}
#endif

#endif	/* _TINY_JSON_H_ */

#include <lua.h>
#include <lauxlib.h>
#include "sha2.h"

#ifdef _WIN32
	#define DLL_EXPORT __declspec(dllexport)
#else
	#define DLL_EXPORT __attribute__ ((dllexport))
#endif
/**
*  X-Or. Does a bit-a-bit exclusive-or of two strings.
*  @param s1: arbitrary binary string.
*  @param s2: arbitrary binary string with same length as s1.
*  @return  a binary string with same length as s1 and s2,
*   where each bit is the exclusive-or of the corresponding bits in s1-s2.
*/
static int ex_or (lua_State *L) {
  size_t l1, l2;
  const char *s1 = luaL_checklstring(L, 1, &l1);
  const char *s2 = luaL_checklstring(L, 2, &l2);
  luaL_Buffer b;
  luaL_argcheck( L, l1 == l2, 2, "lengths must be equal" );
  luaL_buffinit(L, &b);
  while (l1--) luaL_putchar(&b, (*s1++)^(*s2++));
  luaL_pushresult(&b);
  return 1;
}

/**
*  @param: an arbitrary binary string.
*  @return: an SHA-256 hash string.
*/
static int sha256(lua_State *L) {
	size_t len;
	const char *data = luaL_checklstring(L, 1, &len);

	SHA256_CTX context;
	char digest[SHA256_DIGEST_LENGTH];

	SHA256_Init(&context);
	SHA256_Update(&context, (unsigned char *)data, len);
	SHA256_Final((unsigned char *)digest, &context);

	lua_pushlstring(L, digest, SHA256_DIGEST_LENGTH);
	return 1;
}

/**
*  @param: an arbitrary binary string.
*  @return: an SHA-256 hash string.
*/
static int sha256hex(lua_State *L) {
	size_t len;
	const char *data = luaL_checklstring(L, 1, &len);

	char digest[SHA256_DIGEST_STRING_LENGTH];
	SHA256_Data((unsigned char *)data, len, digest);
	lua_pushlstring(L, digest, SHA256_DIGEST_STRING_LENGTH - 1);
	return 1;
}

static int sha256base64(lua_State *L) {
	size_t len;
	const char *data = luaL_checklstring(L, 1, &len);

	char b64[SHA256_DIGEST_BASE64_LENGTH];
	SHA256_Base64((unsigned char *)data, len, (unsigned char *)b64);
	lua_pushlstring(L, b64, SHA256_DIGEST_BASE64_LENGTH - 1);
	return 1;
}

/**
*  @param: an arbitrary binary string.
*  @return: an SHA-384 hash string.
*/
static int sha384(lua_State *L) {
	size_t len;
	const char *data = luaL_checklstring(L, 1, &len);

	SHA384_CTX context;
	char digest[SHA384_DIGEST_LENGTH];

	SHA384_Init(&context);
	SHA384_Update(&context, (unsigned char *)data, len);
	SHA384_Final((unsigned char *)digest, &context);

	lua_pushlstring(L, digest, SHA384_DIGEST_LENGTH);
	return 1;
}

/**
*  @param: an arbitrary binary string.
*  @return: an SHA-384 hash string.
*/
static int sha384hex(lua_State *L) {
	size_t len;
	const char *data = luaL_checklstring(L, 1, &len);

	char digest[SHA384_DIGEST_STRING_LENGTH];
	SHA384_Data((unsigned char *)data, len, digest);
	lua_pushlstring(L, digest, SHA384_DIGEST_STRING_LENGTH - 1);
	return 1;
}

static int sha384base64(lua_State *L) {
	size_t len;
	const char *data = luaL_checklstring(L, 1, &len);

	char b64[SHA384_DIGEST_BASE64_LENGTH];
	SHA384_Base64((unsigned char *)data, len, (unsigned char *)b64);
	lua_pushlstring(L, b64, SHA384_DIGEST_BASE64_LENGTH - 1);
	return 1;
}

/**
*  @param: an arbitrary binary string.
*  @return: an SHA-512 hash string.
*/
static int sha512(lua_State *L) {
	size_t len;
	const char *data = luaL_checklstring(L, 1, &len);

	SHA512_CTX context;
	char digest[SHA512_DIGEST_LENGTH];

	SHA512_Init(&context);
	SHA512_Update(&context, (unsigned char *)data, len);
	SHA512_Final((unsigned char *)digest, &context);

	lua_pushlstring(L, digest, SHA512_DIGEST_LENGTH);
	return 1;
}

/**
*  @param: an arbitrary binary string.
*  @return: an SHA-512 hash string.
*/
static int sha512hex(lua_State *L) {
	size_t len;
	const char *data = luaL_checklstring(L, 1, &len);

	char digest[SHA512_DIGEST_STRING_LENGTH];
	SHA512_Data((unsigned char *)data, len, digest);
	lua_pushlstring(L, digest, SHA512_DIGEST_STRING_LENGTH - 1);
	return 1;
}

static int sha512base64(lua_State *L) {
	size_t len;
	const char *data = luaL_checklstring(L, 1, &len);

	char b64[SHA512_DIGEST_BASE64_LENGTH];
	SHA512_Base64((unsigned char *)data, len, (unsigned char *)b64);
	lua_pushlstring(L, b64, SHA512_DIGEST_BASE64_LENGTH - 1);
	return 1;
}

/*
** Assumes the table is on top of the stack.
*/
static void set_info (lua_State *L) {
	lua_pushliteral (L, "_VERSION");
	lua_pushliteral (L, "sha2 0.1.0");
	lua_settable (L, -3);
}

static struct luaL_reg reg[] = {
	{"exor", ex_or},
	{"sha256", sha256},
	{"sha256hex", sha256hex},
	{"sha256base64", sha256base64},
	{"sha384", sha384},
	{"sha384hex", sha384hex},
	{"sha384base64", sha384base64},
	{"sha512", sha512},
	{"sha512hex", sha512hex},
	{"sha512base64", sha512base64},
	{NULL, NULL}
};

DLL_EXPORT int luaopen_sha2(lua_State *L) {
	luaL_register(L, "sha2", reg);
	set_info(L);
	return 1;
}


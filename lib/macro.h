#ifndef JGFS2_LIB_MACRO_H
#define JGFS2_LIB_MACRO_H


#define CEIL(_x, _step) ((_x) == 0 ? 0 : ((((_x) - 1) / (_step)) + 1))

#define SWAP(_a, _b) do { \
		typeof(_a) _temp_##_a_##_b = (_a); (_a) = (_b); \
		(_b) = (_temp_##_a_##_b); \
	} while (0);

#define STRIFY(_s) _STRIFY(_s)
#define _STRIFY(_s) #_s


#endif

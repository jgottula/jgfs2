#ifndef JGFS2_LIB_MACRO_H
#define JGFS2_LIB_MACRO_H


#define CEIL(_x, _step) ((_x) == 0 ? 0 : ((((_x) - 1) / (_step)) + 1))

#define STRIFY(_s) _STRIFY(_s)
#define _STRIFY(_s) #_s


#endif

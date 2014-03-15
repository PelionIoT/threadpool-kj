#ifndef _DEBUGIT_H
#ifndef DDB_DEBUG_OFF

#define DDB_DEBUG_TXT( code ) { code }



#else

#define DDB_DEBUG_TXT( code ) { }

#endif
#endif

/* INTERN.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#undef EXT
#define EXT

#undef INIT
#ifdef xenix
#define INIT(x) =x
#else
#define INIT(x) = x
#endif

#define DOINIT

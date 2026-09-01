#include "A2XDumper.h"

#ifdef __INTELLISENSE__
extern "C" inline bool cs2_dumper() { return true; }
#else
extern "C" bool cs2_dumper();
#endif

bool A2XDumper::Dump()
{
	return cs2_dumper();
}
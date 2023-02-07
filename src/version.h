#include "windows.h"

#define EVEFILEDESC "CCP Destiny Library\0"
#ifndef _DEBUG
#define EVEINTFILENAME "destiny\0"
#define EVEFILENAME "destiny.dll\0"
#else
#define EVEINTFILENAME "destiny_d\0"
#define EVEFILENAME "destiny_d.dll\0"
#endif
#define EVEFILETYPE VFT_DLL

#include "autoversion.h"
//standard file version thing
#include <../version/evebuildver.h>

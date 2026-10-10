#include <MacTypes.r>

#define __VERSION_REZ__

#include "MacGlide3.h"
#include "rcver.h"

Resource 'vers' (1, "") 
{
	MANVERSION,
	((MANREVISION / 10) << 4) | (MANREVISION % 10),
#if DEBUG
	development,
#else /* !DEBUG */
	release,
#endif /* !DEBUG */
	0,
	verUS,
	BASE_VERSIONSTR,
	BASE_HWSTR" "BASE_VERSIONSTR" for MacOS"
};


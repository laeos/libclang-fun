#include "common.h"

const char *linkage2str(enum CXLinkageKind l)
{
    switch (l) {
	case CXLinkage_Invalid: return "CXLinkage_Invalid";
	case CXLinkage_NoLinkage: return "CXLinkage_NoLinkage";
	case CXLinkage_Internal: return "CXLinkage_Internal";
	case CXLinkage_UniqueExternal: return "CXLinkage_UniqueExternal";
	case CXLinkage_External: return "CXLinkage_External";
    }
    return "unknown";
}

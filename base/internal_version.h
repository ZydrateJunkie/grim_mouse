#ifndef INCLUDED_FROM_BASE_VERSION_CPP
#error This file may only be included by base/version.cpp
#endif

// Reads revision number from file
// (this is used when building with Visual Studio)
#ifdef RESIDUALVM_INTERNAL_REVISION
#include "internal_revision.h"
#endif

#ifdef RELEASE_BUILD
#undef RESIDUALVM_REVISION
#endif

#ifndef RESIDUALVM_REVISION
#define RESIDUALVM_REVISION
#endif

#define RESIDUALVM_VERSION "0.0.8git" RESIDUALVM_REVISION

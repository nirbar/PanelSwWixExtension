#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <strsafe.h>
#include <msiquery.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <atlbase.h>

// WiX Header Files:
#include <dutil.h>
#include <fileutil.h>
#include <strutil.h>
#include <memutil.h>
#include <pathutil.h>
#include <dirutil.h>
#include <xmlutil.h>
#include <BundleExtensionEngine.h>
#include <BundleExtension.h>

#include <IBundleExtensionEngine.h>
#include <IBundleExtension.h>
#include <bextutil.h>
#include <BextBundleExtensionEngine.h>
#include <BextBaseBundleExtension.h>

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <strsafe.h>
#include <msiquery.h>
#include <shellapi.h>
#include <combaseapi.h>
#include <atlbase.h>
#include <intsafe.h>

// WiX Header Files:
#include <dutil.h>
#include <fileutil.h>
#include <strutil.h>
#include <memutil.h>
#include <pathutil.h>
#include <dirutil.h>
#include <xmlutil.h>
#include <regutil.h>
#include <butil.h>
#include <verutil.h>
#include <BootstrapperExtensionEngineTypes.h>
#include <BootstrapperExtensionTypes.h>

#include <IBootstrapperExtensionEngine.h>
#include <IBootstrapperExtension.h>
#include <bextutil.h>
#include <BextBootstrapperExtensionEngine.h>
#include <BextBaseBootstrapperExtension.h>

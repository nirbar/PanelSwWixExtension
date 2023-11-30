#include "pch.h"

extern "C" UINT __stdcall ListProcessorFeatures(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	BOOL bRes = TRUE;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	ExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	WcaSetIntProperty(L"PF_ARM_64BIT_LOADSTORE_ATOMIC", ::IsProcessorFeaturePresent(PF_ARM_64BIT_LOADSTORE_ATOMIC) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_EXTERNAL_CACHE_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_EXTERNAL_CACHE_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_VFP_32_REGISTERS_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_VFP_32_REGISTERS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_3DNOW_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_CHANNELS_ENABLED", ::IsProcessorFeaturePresent(PF_CHANNELS_ENABLED) ? 1 : 0);
	WcaSetIntProperty(L"PF_COMPARE_EXCHANGE_DOUBLE", ::IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE_DOUBLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_COMPARE_EXCHANGE128", ::IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE128) ? 1 : 0);
	WcaSetIntProperty(L"PF_COMPARE64_EXCHANGE128", ::IsProcessorFeaturePresent(PF_COMPARE64_EXCHANGE128) ? 1 : 0);
	WcaSetIntProperty(L"PF_FASTFAIL_AVAILABLE", ::IsProcessorFeaturePresent(PF_FASTFAIL_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_FLOATING_POINT_EMULATED", ::IsProcessorFeaturePresent(PF_FLOATING_POINT_EMULATED) ? 1 : 0);
	WcaSetIntProperty(L"PF_FLOATING_POINT_PRECISION_ERRATA", ::IsProcessorFeaturePresent(PF_FLOATING_POINT_PRECISION_ERRATA) ? 1 : 0);
	WcaSetIntProperty(L"PF_MMX_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_NX_ENABLED", ::IsProcessorFeaturePresent(PF_NX_ENABLED) ? 1 : 0);
	WcaSetIntProperty(L"PF_PAE_ENABLED", ::IsProcessorFeaturePresent(PF_PAE_ENABLED) ? 1 : 0);
	WcaSetIntProperty(L"PF_RDTSC_INSTRUCTION_AVAILABLE", ::IsProcessorFeaturePresent(PF_RDTSC_INSTRUCTION_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_RDWRFSGSBASE_AVAILABLE", ::IsProcessorFeaturePresent(PF_RDWRFSGSBASE_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_SECOND_LEVEL_ADDRESS_TRANSLATION", ::IsProcessorFeaturePresent(PF_SECOND_LEVEL_ADDRESS_TRANSLATION) ? 1 : 0);
	WcaSetIntProperty(L"PF_SSE3_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_SSE3_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_SSSE3_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_SSSE3_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_SSE4_1_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_SSE4_1_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_SSE4_2_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_SSE4_2_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_AVX_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_AVX_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_AVX2_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_AVX512F_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_AVX512F_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_VIRT_FIRMWARE_ENABLED", ::IsProcessorFeaturePresent(PF_VIRT_FIRMWARE_ENABLED) ? 1 : 0);
	WcaSetIntProperty(L"PF_XMMI_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_XMMI64_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_XSAVE_ENABLED", ::IsProcessorFeaturePresent(PF_XSAVE_ENABLED) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_V8_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_V8_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE) ? 1 : 0);
	WcaSetIntProperty(L"PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE", ::IsProcessorFeaturePresent(PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE) ? 1 : 0);

LExit:
	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

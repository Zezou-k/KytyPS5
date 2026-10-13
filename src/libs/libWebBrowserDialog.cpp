#include "common/abi.h"
#include "common/logging/log.h"
#include "libs/errno.h"
#include "libs/libs.h"
#include "loader/symbolDatabase.h"

#include <cstring>
#include <mutex>

namespace Libs {

LIB_VERSION("WebBrowserDialog", 1, "WebBrowserDialog", 1, 1);

namespace WebBrowserDialog {

// Same progression every Kyty dialog uses (Signin/Login/SaveData):
// NONE -> INITIALIZED -> RUNNING -> FINISHED -> (Open again | Terminate -> NONE).
constexpr int WEB_STATUS_NONE        = 0;
constexpr int WEB_STATUS_INITIALIZED = 1;
constexpr int WEB_STATUS_RUNNING     = 2;
constexpr int WEB_STATUS_FINISHED    = 3;

// GetResult result values.
constexpr int WEB_RESULT_OK            = 0;
constexpr int WEB_RESULT_USER_CANCELED = 1;

// NID mapping taken from shadPS4's webbrowserdialog.cpp (high-confidence symbol names, the
// same ten NIDs Stray/PAW Patrol/Silksong import as unresolved stubs today).
// NOTE(unverified): the numeric error values below reuse the 0x8135xxxx CommonDialog family
// pattern of Kyty's SigninDialog. Real SceWebBrowserDialogError codes were never confirmed
// from an SDK header; games normally only branch on zero/non-zero.
constexpr int WEB_ERROR_NOT_INITIALIZED     = static_cast<int>(0x81350041u);
constexpr int WEB_ERROR_ALREADY_INITIALIZED = static_cast<int>(0x81350042u);
constexpr int WEB_ERROR_PARAM_INVALID       = static_cast<int>(0x81350043u);
constexpr int WEB_ERROR_INVALID_STATE       = static_cast<int>(0x81350045u);

std::mutex g_web_mutex;
int        g_web_status = WEB_STATUS_NONE;

int KYTY_SYSV_ABI WebBrowserDialogInitialize() {
	PRINT_NAME();

	std::scoped_lock lock(g_web_mutex);
	if (g_web_status != WEB_STATUS_NONE) {
		return WEB_ERROR_ALREADY_INITIALIZED;
	}

	g_web_status = WEB_STATUS_INITIALIZED;
	return OK;
}

int KYTY_SYSV_ABI WebBrowserDialogTerminate() {
	PRINT_NAME();

	std::scoped_lock lock(g_web_mutex);
	if (g_web_status == WEB_STATUS_NONE) {
		return WEB_ERROR_NOT_INITIALIZED;
	}

	g_web_status = WEB_STATUS_NONE;
	return OK;
}

int KYTY_SYSV_ABI WebBrowserDialogOpenForPredeterminedContent(const void* param) {
	PRINT_NAME();

	std::scoped_lock lock(g_web_mutex);
	// Same state gate as every Kyty dialog Open.
	if (g_web_status == WEB_STATUS_NONE) {
		return WEB_ERROR_NOT_INITIALIZED;
	}
	if (g_web_status != WEB_STATUS_INITIALIZED && g_web_status != WEB_STATUS_FINISHED) {
		return WEB_ERROR_INVALID_STATE;
	}

	// No host browser UI on the overlay yet; log whatever the title handed us so game logs
	// still show the requested destination, then let the dialog complete "successfully".
	LOGF("WebBrowserDialog: open for predetermined content (param = {})\n",
	     static_cast<const void*>(param));

	g_web_status = WEB_STATUS_RUNNING;
	return OK;
}

int KYTY_SYSV_ABI WebBrowserDialogOpen(const void* param) {
	PRINT_NAME();

	{
		std::scoped_lock lock(g_web_mutex);
		if (g_web_status == WEB_STATUS_NONE) {
			return WEB_ERROR_NOT_INITIALIZED;
		}
		if (g_web_status != WEB_STATUS_INITIALIZED && g_web_status != WEB_STATUS_FINISHED) {
			return WEB_ERROR_INVALID_STATE;
		}
	}

	// The guest param is SceWebBrowserDialogParam; its layout is unverified, so avoid
	// hard validation and only probe the leading size field for the message below.
	uint32_t param_size = 0;
	if (param != nullptr) {
		std::memcpy(&param_size, param, sizeof(param_size));
	}
	LOGF("WebBrowserDialog: open (param = {}, size = {})\n", static_cast<const void*>(param),
	     static_cast<uint32_t>(param_size));

	std::scoped_lock lock(g_web_mutex);
	g_web_status = WEB_STATUS_RUNNING;
	return OK;
}

int KYTY_SYSV_ABI WebBrowserDialogClose() {
	PRINT_NAME();

	std::scoped_lock lock(g_web_mutex);
	if (g_web_status == WEB_STATUS_NONE) {
		return WEB_ERROR_NOT_INITIALIZED;
	}
	if (g_web_status != WEB_STATUS_RUNNING) {
		return WEB_ERROR_INVALID_STATE;
	}

	// Without a browser UI the only sensible completion is "the user backed out".
	g_web_status = WEB_STATUS_FINISHED;
	return OK;
}

int KYTY_SYSV_ABI WebBrowserDialogUpdateStatus() {
	PRINT_NAME();

	std::scoped_lock lock(g_web_mutex);
	// Games poll this from their loop; auto-finish a RUNNING session so they are not stuck
	// waiting on UI that cannot happen yet (shadPS4 does the same, marked TODO there).
	if (g_web_status == WEB_STATUS_RUNNING) {
		LOGF("WebBrowserDialog: no browser UI available, completing dialog with cancel\n");
		g_web_status = WEB_STATUS_FINISHED;
	}
	return g_web_status;
}

int KYTY_SYSV_ABI WebBrowserDialogGetStatus() {
	PRINT_NAME();

	std::scoped_lock lock(g_web_mutex);
	return g_web_status;
}

int KYTY_SYSV_ABI WebBrowserDialogGetResult(void* result) {
	PRINT_NAME();

	std::scoped_lock lock(g_web_mutex);
	if (g_web_status == WEB_STATUS_NONE) {
		return WEB_ERROR_NOT_INITIALIZED;
	}
	if (g_web_status != WEB_STATUS_FINISHED) {
		return WEB_ERROR_INVALID_STATE;
	}
	if (result == nullptr) {
		return WEB_ERROR_PARAM_INVALID;
	}

	auto* r   = static_cast<int32_t*>(result);
	r[0]      = WEB_RESULT_USER_CANCELED;
	r[1]      = 0;
	r[2]      = 0;
	r[3]      = 0;
	return OK;
}

int KYTY_SYSV_ABI WebBrowserDialogSetCookie(void* param) {
	PRINT_NAME();

	std::scoped_lock lock(g_web_mutex);
	if (g_web_status == WEB_STATUS_NONE) {
		return WEB_ERROR_NOT_INITIALIZED;
	}
	LOGF("WebBrowserDialog: set cookie without a persistent cookie jar, ignoring (param = {})\n",
	     static_cast<const void*>(param));
	return OK;
}

int KYTY_SYSV_ABI WebBrowserDialogResetCookie() {
	PRINT_NAME();

	std::scoped_lock lock(g_web_mutex);
	if (g_web_status == WEB_STATUS_NONE) {
		return WEB_ERROR_NOT_INITIALIZED;
	}
	return OK;
}

} // namespace WebBrowserDialog

LIB_DEFINE(InitWebBrowserDialog_1) {
	// Names follow shadPS4's webbrowserdialog.cpp mapping; this block is registered last in
	// Libs::InitAll, so these entries take precedence over the libNet.cpp stubs.
	LIB_FUNC("FraP7debcdg", WebBrowserDialog::WebBrowserDialogOpen);
	LIB_FUNC("O7dIZQrwVFY", WebBrowserDialog::WebBrowserDialogOpenForPredeterminedContent);
	LIB_FUNC("PSK+Eik919Q", WebBrowserDialog::WebBrowserDialogClose);
	LIB_FUNC("h1dR-t5ISgg", WebBrowserDialog::WebBrowserDialogUpdateStatus);
	LIB_FUNC("vCaW0fgVQmc", WebBrowserDialog::WebBrowserDialogGetResult);
	LIB_FUNC("TZnDVkP91Rg", WebBrowserDialog::WebBrowserDialogSetCookie);
	LIB_FUNC("Cya+jvTtPqg", WebBrowserDialog::WebBrowserDialogResetCookie);
	LIB_FUNC("CFTG6a8TjOU", WebBrowserDialog::WebBrowserDialogGetStatus);
	LIB_FUNC("jqb7HntFQFc", WebBrowserDialog::WebBrowserDialogInitialize);
	LIB_FUNC("ocHtyBwHfys", WebBrowserDialog::WebBrowserDialogTerminate);
}

} // namespace Libs

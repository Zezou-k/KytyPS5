#include "common/abi.h"
#include "libs/dialog.h"
#include "libs/libs.h"
#include "loader/symbolDatabase.h"

namespace Libs {

LIB_VERSION("WebBrowserDialog", 1, "WebBrowserDialog", 1, 1);

LIB_DEFINE(InitWebBrowserDialog_1) {
	// NID mapping from shadPS4's webbrowserdialog.cpp. The full lifecycle, the
	// status machine, and the overlay session live in libs/dialog.cpp. Note the
	// alias below avoids clashing with the existing Libs::Dialog namespace.
	LIB_FUNC("jqb7HntFQFc", Libs::Dialog::WebBrowserDialog::Initialize);
	LIB_FUNC("ocHtyBwHfys", Libs::Dialog::WebBrowserDialog::Terminate);
	LIB_FUNC("FraP7debcdg", Libs::Dialog::WebBrowserDialog::Open);
	LIB_FUNC("O7dIZQrwVFY", Libs::Dialog::WebBrowserDialog::Open);
	LIB_FUNC("PSK+Eik919Q", Libs::Dialog::WebBrowserDialog::Close);
	LIB_FUNC("h1dR-t5ISgg", Libs::Dialog::WebBrowserDialog::UpdateStatus);
	LIB_FUNC("vCaW0fgVQmc", Libs::Dialog::WebBrowserDialog::GetResult);
	LIB_FUNC("TZnDVkP91Rg", Libs::Dialog::WebBrowserDialog::SetCookie);
	LIB_FUNC("Cya+jvTtPqg", Libs::Dialog::WebBrowserDialog::ResetCookie);
	LIB_FUNC("CFTG6a8TjOU", Libs::Dialog::WebBrowserDialog::GetStatus);
}

} // namespace Libs

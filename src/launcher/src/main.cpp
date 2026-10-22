#include "mainDialog.h"
#include "bootSplash.h"
#include "launcherTheme.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);
	// Title bars and taskbars get the bolder small-size artwork.
	const QIcon small_art(QStringLiteral(":/icons/kyty-small.svg"));
	const QIcon large_art(QStringLiteral(":/icons/kyty.svg"));
	QIcon       icon;
	for (int size: {16, 24, 32, 48, 64}) {
		icon.addPixmap(small_art.pixmap(QSize(size, size), 1.0));
	}
	for (int size: {128, 256}) {
		icon.addPixmap(large_art.pixmap(QSize(size, size), 1.0));
	}
	QApplication::setWindowIcon(icon);
	LauncherTheme::Initialize(a);

	// Read the skip-splash preference directly from Kyty.ini ([Launcher]/skip_boot_splash).
	if (QFile::exists(QStringLiteral("Kyty.ini"))) {
		QSettings settings(QStringLiteral("Kyty.ini"), QSettings::IniFormat);
		MainDialog::SetSkipBootSplash(settings.value("Launcher/skip_boot_splash", false).toBool());
	}

	auto* splash = new BootSplash;

	if (splash != nullptr && !MainDialog::SkipBootSplash()) {
		splash->show();
		splash->PresentFrame();
	}

	MainDialog w;

	w.emit Start();

	// Show the game list only after the boot animation finishes (or immediately when skipped).
	QTimer::singleShot(splash->isVisible() ? 2600 : 0, &a, [&w, splash] {
		if (splash != nullptr && splash->isVisible()) {
			splash->close();
			splash->deleteLater();
		}
		w.showMaximized();
		w.raise();
		w.activateWindow();
	});
	if (splash == nullptr || !splash->isVisible()) {
		splash->deleteLater();
		splash = nullptr;
		QTimer::singleShot(0, &a, [&w] {
			w.showMaximized();
			w.raise();
			w.activateWindow();
		});
	}

	return QApplication::exec();
}

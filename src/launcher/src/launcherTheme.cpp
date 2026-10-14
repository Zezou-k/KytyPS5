#include "launcherTheme.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QObject>
#include <QPalette>
#include <QStyleFactory>
#include <QTimer>

namespace {

// PS5 system-software tones: near-black surfaces, white text, no colored highlights.
void ApplySystemPalette(QApplication& application) {
	QPalette palette;
	palette.setColor(QPalette::Window, QColor(0, 0, 0));
	palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
	palette.setColor(QPalette::Base, QColor(16, 16, 16));
	palette.setColor(QPalette::AlternateBase, QColor(24, 24, 24));
	palette.setColor(QPalette::ToolTipBase, QColor(24, 24, 24));
	palette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
	palette.setColor(QPalette::Text, QColor(255, 255, 255));
	palette.setColor(QPalette::Button, QColor(24, 24, 24));
	palette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
	palette.setColor(QPalette::BrightText, QColor(255, 255, 255));
	// Keep links white like the PS5 shell (it never renders blue links).
	palette.setColor(QPalette::Link, QColor(224, 224, 224));
	palette.setColor(QPalette::LinkVisited, QColor(224, 224, 224));
	palette.setColor(QPalette::Highlight, QColor(255, 255, 255));
	palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
	palette.setColor(QPalette::PlaceholderText, QColor(160, 160, 160));
	for (const auto role :
	     {QPalette::Light, QPalette::Midlight, QPalette::Mid, QPalette::Dark, QPalette::Shadow}) {
		palette.setColor(role, QColor(48, 48, 48));
	}
	application.setPalette(palette);
}

class PaletteChangeFilter final: public QObject {
public:
	explicit PaletteChangeFilter(QApplication& application)
	    : QObject(&application), m_application(application) {}

protected:
	bool eventFilter(QObject* watched, QEvent* event) override {
		if (event->type() == QEvent::ApplicationPaletteChange && !m_refresh_pending) {
			m_refresh_pending = true;
			QTimer::singleShot(0, &m_application, [this] {
				m_application.setStyleSheet(m_application.styleSheet());
				m_refresh_pending = false;
			});
		}

		return QObject::eventFilter(watched, event);
	}

private:
	QApplication& m_application;
	bool          m_refresh_pending = false;
};

} // namespace

namespace LauncherTheme {

void Initialize(QApplication& application) {
	// Fusion gives every widget a flat, predictable base for the PS5 styling.
	application.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
	ApplySystemPalette(application);

	QFile style_sheet_file(QStringLiteral(":/styles/launcher.qss"));
	if (style_sheet_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		application.setStyleSheet(QString::fromUtf8(style_sheet_file.readAll()));
		application.installEventFilter(new PaletteChangeFilter(application));
	}
}
} // namespace LauncherTheme

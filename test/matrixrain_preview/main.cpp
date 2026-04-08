// Standalone visual preview for MatrixRain screensaver.
// No UC infrastructure — just the renderer in a 480x850 window with keyboard controls.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>

#include "../../src/ui/matrixrain.h"
#include "../../src/ui/glyphatlas.h"

int main(int argc, char *argv[]) {
    qputenv("QT_SCALE_FACTOR", "1");
    qputenv("QML_DISABLE_DISTANCEFIELD", "1");
    QQuickWindow::setTextRenderType(QQuickWindow::TextRenderType::NativeTextRendering);

    QGuiApplication app(argc, argv);
    app.setOrganizationName("madalone");
    app.setApplicationName("matrixrain-preview");

    qmlRegisterType<MatrixRainItem>("MatrixRain", 1, 0, "MatrixRain");
    GlyphAtlas::loadCJKFont();

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/Preview.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

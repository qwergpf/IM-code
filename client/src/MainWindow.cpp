#include "MainWindow.h"

#include <QLabel>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("IM Chat Client"));
    resize(1000, 700);

    auto* welcomeLabel = new QLabel(
        QStringLiteral("IM 客户端正在启动，登录和聊天功能将在后续实现。"),
        this
    );
    welcomeLabel->setAlignment(Qt::AlignCenter);
    setCentralWidget(welcomeLabel);

    statusBar()->showMessage(QStringLiteral("未连接服务器"));
}

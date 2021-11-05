//file: mainwindow.h

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MyModel;
class NyModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();
private:
    Ui::MainWindow *_ui;
    NyModel *_myModel;
};



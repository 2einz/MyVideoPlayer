#ifndef SRC_MY_VIDEO_PLAYER_WIDGET_H_
#define SRC_MY_VIDEO_PLAYER_WIDGET_H_

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget {
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

private:
    Ui::Widget *ui;
};
#endif  // SRC_MY_VIDEO_PLAYER_WIDGET_H_

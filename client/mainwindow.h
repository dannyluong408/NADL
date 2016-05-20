#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#define __STDC_LIMIT_MACROS

#include <QMainWindow>
#include <QSslSocket>
#include <QCoreApplication>
#include <QTimer>
#include "ui_mainwindow.h"
#include <iostream>
#include <string>
#include <QListWidget>
#include <QPoint>
#include <QMenu>
#include <QKeyEvent>
#include <QApplication>
#include <QLineEdit>
#include <QCompleter>
#include <QStringList>
#include <QDebug>
#include <QList>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "data_types.hpp"
#include <QTextBrowser>

namespace Ui {
class MainWindow;
}


struct Widget_Pointers {
    QTextBrowser *browsers;
    QListWidget *user_lists;
    QListWidget *rank_lists;
    QListWidget *games_lists;

};

struct Channel {
    char channel_name[64];
    uint16_t channel_id, n_users, n_games;
    user_info *users;
    game_state *games;
    int qt_tab_id;
    uint8_t flags;
    Widget_Pointers *channel_data;
    char rankings[100][64];
    uint16_t ratings[100];
    QStringList *completer_list;
    QList<QString> *history_list;
    int current_history;
};



class KeyboardFilter : public QObject
{
public:
  KeyboardFilter( QObject *parent = 0 ) : QObject( parent ) {}

protected:
  bool eventFilter( QObject *dist, QEvent *event )
  {
    if( event->type() == QEvent::KeyPress )
    {
      QKeyEvent *keyEvent = static_cast<QKeyEvent*>( event );
      printf("Keyevent detected\n");
      fflush(stdout);
      if( QString("1234567890").indexOf( keyEvent->text() ) != -1 )
        return true;
    }

    return false;
  }
};



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
   // void on_LeagueTabs_currentChanged(int index);
    void updateCompleter();
    void loginButton_clicked();
    void update_chat();
    void update_user_list();
    void chatbutton_clicked();
    void pushButton_clicked();
    void update();
    void setupTabs();
    void closeTab_(int index);
    void enter_chat();
    Widget_Pointers *testtab(QWidget *tab_x, int tab_index);
    void clear_chat();
    void showContextMenu(const QPoint& pos);
    //void eraseItem();
    //void addItem();
    //void compareItem();
    void mute_sounds();
    void itemDoubleClicked(QListWidgetItem*);
    void color_text(char string,Channel &browser, int tab_index);
    void fillIn();
    void skipHigh();
    void moveUp();
    void moveDown();

signals:
    void tabPressed();
    void enterPressed();
    void upArrow();
    void downArrow();

private:
    bool eventFilter(QObject *obj, QEvent *event);
    //QTabWidget *tabs[5];
    QSslSocket socket;
    Ui::MainWindow *ui;
    QTimer timer;
    int n_tabs;

    char buffer[RECV_MAXLEN];
    uint32_t buffer_pos, n_channels;
    std::map<uint32_t, std::string> users;
    Channel **channel;
    uint32_t my_id;
    int login_sent, connection_attempted;
    void redraw_game_list(const uint16_t channel_id);
    void redraw_rank_list(const uint16_t channel_id);
    void manual_connect();
    char username[64];
    char token[64];
    char use_token;
};

#endif // MAINWINDOW_H





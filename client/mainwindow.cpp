#include "mainwindow.h"
#include "ui_mainwindow.h"

QStringList empty;
bool upDown = false;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    login_sent = 0;
    connection_attempted = 0;
    buffer_pos = 0;
    users[UINT32_MAX] = "SYSTEM";
    channel = NULL;
    n_channels = 0;
    completer = NULL;

    FILE *f = fopen("user","r");
    if (!f) {
        use_token = false;
    } else {
        memset(username, 0, sizeof(username));
        memset(token, 0, sizeof(token));
        fgets(username, sizeof(username), f);
        fgets(token, sizeof(token), f);
        for (uint x = 0; x < sizeof(username); ++x) {
            if (username[x] == '\n') username[x] = '\0';
        }
        use_token = true;
        fclose(f);
    }

    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint| Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);

    n_tabs = 0;
    ui->setupUi(this);

    Channel **ptr = (Channel**)malloc(sizeof(Channel*));
    memcpy(ptr, channel, sizeof(Channel**)*0);
    free(channel);
    channel = ptr;
    channel[0] = (Channel*)malloc(sizeof(Channel));
    channel[0]->history_list = new QList<QString>; // Intialize history_list
    channel[0]->current_history = 0; // Initialize

    ui->chat_input->installEventFilter(this);

    connect(this, SIGNAL(tabPressed()),this,SLOT(fillIn()));
    connect(this, SIGNAL(enterPressed()),this,SLOT(skipHigh()));
    connect(this, SIGNAL(upArrow()),this,SLOT(moveUp()));
    connect(this, SIGNAL(downArrow()),this,SLOT(moveDown()));
    //messing around with completer
    /*
    QStringList temp;
    QCompleter *completer = new QCompleter(temp, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::InlineCompletion);
    ui->chat_input->setCompleter(completer);
    temp << "beta";
    temp.clear();
    temp << "test";
    completer = new QCompleter(temp, this);
    ui->chat_input->setCompleter(completer);
*/

    ui->actionMute_Sounds->setCheckable(true);

    connect(ui->chat_input, SIGNAL(returnPressed()),ui->chat_send,SIGNAL(clicked()));

    //connecting chat send button
    connect(ui->chat_send, SIGNAL(clicked()), this, SLOT(enter_chat()));

    //make x buttons be able to close tab
    connect(ui->tabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab_(int)));

    //read signal when tab changes
    //connect(ui->tabs, SIGNAL(currentChanged(int)), this, SLOT(updateCompleter()));

    //mute sound connection
    //connect(ui->sound_check,SIGNAL(clicked(bool)),this,SLOT(mute_sounds()));

    //hide status x box
    ui->tabs->tabBar()->tabButton(0, QTabBar::RightSide)->resize(0, 0);

    //icons for users
    //ui->nel_users->item(0)->setIcon(QIcon(":/user.png"));
    //ui->nel_users->item(1)->setIcon(QIcon(":/user.png"));

    //QTimer* timer = new QTimer(this);

    connect(&timer, SIGNAL(timeout()), this, SLOT(update()) );
    connect(&timer, SIGNAL(timeout()), this, SLOT(updateCompleter()) );

    this->timer.start(1);
    ui->tabs->setCurrentIndex(0);

}

void MainWindow::moveDown(){
    int tab_index = ui->tabs->currentIndex();
    int z = 0, size = 9;
    for (uint32_t x = 0; x < n_channels ; x++){
        if (channel[x]->qt_tab_id == tab_index){
            z = x;
        }
    }
    fflush(stdout);
    if (channel[z]->history_list->size() != 9){
        if(channel[z]->history_list->isEmpty()){ //Checks if list is empty
            return;
        }
        else{
            size = channel[z]->history_list->size(); //Sets size to list size if there has not been 10 entries yet
        }
    }

    if (channel[z]->current_history < size && size != 1){
        if (channel[z]->current_history != size ){
            if ((channel[z]->current_history + 1) == size ){
                return;
            }
            channel[z]->current_history++;
            fflush(stdout);
        }
        ui->chat_input->clear();
        ui->chat_input->setFocus();
        ui->chat_input->insert((QString)channel[z]->history_list->at(channel[z]->current_history));
        ui->chat_input->selectAll();
        ui->chat_input->end(true);
        upDown = true;
    }

    else if(channel[z]->history_list->size() == 1 && channel[z]->current_history == 0){
        //qDebug() << "2";
        ui->chat_input->clear();
        ui->chat_input->setFocus();
        ui->chat_input->insert((QString)channel[z]->history_list->at(channel[z]->current_history));
        ui->chat_input->selectAll();
        ui->chat_input->end(true);
        upDown = true;
    }
}

void MainWindow::moveUp(){
    int tab_index = ui->tabs->currentIndex();
    int z = 0;
    for (uint32_t x = 0; x < n_channels ; x++){
        if (channel[x]->qt_tab_id == tab_index){
            z = x;
        }
    }
    fflush(stdout);
    if ( z == 0 && channel[z]->history_list == NULL){
        return;
    }
    if (channel[z]->current_history > 0){
        if (channel[z]->current_history !=0){
            channel[z]->current_history--;
        }
        ui->chat_input->clear();
        ui->chat_input->setFocus();
        ui->chat_input->insert((QString)channel[z]->history_list->at(channel[z]->current_history));
        ui->chat_input->selectAll();
        ui->chat_input->end(true);
        upDown = true;
    }
}

void MainWindow::updateCompleter(){
    int tab_index = ui->tabs->currentIndex();

    if (tab_index == 0){
        return;
    }

    for (uint32_t x = 0; x < n_channels ; x++){
        if (channel[x]->qt_tab_id == tab_index){
            tab_index = channel[x]->qt_tab_id;
        }
    }

    delete completer;
    //updates the completer list every sec
    completer = new QCompleter(*channel[tab_index-1]->completer_list, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setCompletionMode(QCompleter::InlineCompletion);
    ui->chat_input->setCompleter(completer);


    /*
    QStringList test;
    for(int i = 0; i < ((channel[tab_index-1]->channel_data->user_lists->count()) ); i++){
        test << (channel[tab_index-1]->channel_data->user_lists->item(i)->text() );
    }
    qDebug() << test;
    */
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == ui->chat_input) {
     if (event->type() == QEvent::KeyPress) {
         QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab)
        {
            if(ui->chat_input->hasSelectedText() == false){
                return false;
            }
            //do what you need;
            emit tabPressed();
            return true;
        }
        else if (keyEvent->key() == Qt::Key_Up)
        {
            emit upArrow();
            return true;
        }
        else if(keyEvent->key() == Qt::Key_Down)
        {
            emit downArrow();
            return true;
        }
        if ((keyEvent->key()==Qt::Key_Enter) || (keyEvent->key()==Qt::Key_Return) )
        {
            if(ui->chat_input->hasSelectedText() == false){
                return false;
            }
            //do what you need;
            emit enterPressed();
            return true;
        }
     }
  }
     // pass the event on to the parent class
     return QMainWindow::eventFilter(obj, event);
}

void MainWindow::fillIn(){
    if (upDown == false){
        ui->chat_input->end(true);
        ui->chat_input->insert(" ");
        ui->chat_input->end(false);
    }
}

void MainWindow::skipHigh(){
    if (upDown == false){
        ui->chat_input->del();
        enter_chat();
    }
    else{
        enter_chat();
        upDown = false;
    }
}
//Unused for now
//void MainWindow::color_text(char string,Channel &browser, int tab_index){
//    QString qmsg(string);
//    channel[tab_index]->channel_data->browsers->append("<span style=\" color:#ff0000;\">" + qmsg + "</span>");
//}

void MainWindow::redraw_game_list(const uint16_t channel_id) {
    channel[channel_id]->channel_data->games_lists->clear();
    for (int x = 0; x < channel[channel_id]->n_games; ++x) {
        if (channel[channel_id]->games[x].n_users) {
            char str[1000];
            if (channel[channel_id]->games[x].start_time) {
                //time_t now = time(NULL);
                const int minutes = channel[channel_id]->games[x].start_time / 60000;
                snprintf(str,sizeof(str),"%s (%s): %im",channel[channel_id]->games[x].game_name,channel[channel_id]->games[x].host_name,minutes);
            } else {
                snprintf(str,sizeof(str),"%s (%s): %i/%i",channel[channel_id]->games[x].game_name,channel[channel_id]->games[x].host_name,channel[channel_id]->games[x].n_users,channel[channel_id]->games[x].n_max);
            }
            channel[channel_id]->channel_data->games_lists->addItem(str);
        }
    }
    channel[channel_id]->channel_data->games_lists->sortItems(Qt::AscendingOrder);
}

void MainWindow::redraw_rank_list(const uint16_t channel_id) {
     channel[channel_id]->channel_data->rank_lists->clear();
     for (uint32_t x = 0; x < 100; ++x) {
        if (channel[channel_id]->rankings[x][0] != 0) {
            char str[1000];
            snprintf(str, sizeof(str), "%u: %s - %u", x+1, channel[channel_id]->rankings[x], channel[channel_id]->ratings[x]);
            channel[channel_id]->channel_data->rank_lists->addItem(str);
        }
     }
}

void MainWindow::mute_sounds(){
    //Mutes Sounds when checked;
    //if (ui->sound_check->isChecked())
         printf("Checked\n");
    //else
        printf("Unchecked\n");
}

void MainWindow::itemDoubleClicked(QListWidgetItem *item){
    char msg[1000];
    const char *str = item->text().toUtf8();
    snprintf(msg, sizeof(msg), "/sign %s", str);
    int tab_index = ui->tabs->currentIndex();

    chat_message_header header;
    header.user_id = 0;
    header.msg_flags = 0;
    header.msg_length = strlen(msg)+1;
    header.channel = 0; // replace this with the channel id

    for (uint32_t x = 0; x < n_channels ; x++){
        if (channel[x]->qt_tab_id == tab_index){
            header.channel = channel[x]->channel_id;
            x = n_channels;
        }
    }

    unsigned char data[1000];
    data[0] = CLIENT_MESSAGE_CHAT_MESSAGE;
    memcpy(&data[1], &header, sizeof(header));
    memcpy(&data[1+sizeof(header)], msg, header.msg_length);
    socket.write((char*)data, 1+sizeof(header)+header.msg_length);

    printf ("Item double clicked %s\n",str);fflush(stdout);
}

Widget_Pointers *MainWindow::createTab(QWidget *tab_x){
    //QWidget *tab_x = new QWidget();
    QHBoxLayout *horizontalLayout_x; // MainLayout
    QHBoxLayout *horizontalLayout_y; // Ranks/Games Layout
    QVBoxLayout *verticalLayout_x; // Games+Ranks Layout
    QVBoxLayout *verticalLayout_y; // Browser Layout
    QVBoxLayout *verticalLayout_z; // UserList Layout

    QTextBrowser *textBrowser_x;// = new QTextBrowser;
    QListWidget *listWidget_x;// = new QListWidget; //games
    QListWidget *listWidget_y;// = new QListWidget; // ranks
    QListWidget *user_list_x;// = new QListWidget; //users

    //tab_x = new QWidget();
    ui->tabs->addTab(tab_x, QString("Test"));
    tab_x->setObjectName(QStringLiteral("tab_x"));

    textBrowser_x = new QTextBrowser(tab_x);
    textBrowser_x->setObjectName(QStringLiteral("textBrowser_x"));

    //textBrowser_x->setTextColor(Qt::darkGreen);

    user_list_x = new QListWidget(tab_x);
    user_list_x->setObjectName(QStringLiteral("test_users"));

    user_list_x->setSortingEnabled(1);

    horizontalLayout_x = new QHBoxLayout(tab_x);
    horizontalLayout_x->setSpacing(6);
    horizontalLayout_x->setObjectName(QStringLiteral("horizontalLayout_x"));

    horizontalLayout_y = new QHBoxLayout();
    horizontalLayout_y->setSpacing(6);
    horizontalLayout_y->setObjectName(QStringLiteral("horizontalLayout_y"));

    verticalLayout_x = new QVBoxLayout();
    verticalLayout_x ->setSpacing(6);
    verticalLayout_x ->setObjectName(QStringLiteral("horizontalLayout_x"));

    verticalLayout_y = new QVBoxLayout();
    verticalLayout_y->setSpacing(6);
    verticalLayout_y->setObjectName(QStringLiteral("horizontalLayout_y"));

    verticalLayout_z = new QVBoxLayout();
    verticalLayout_z->setSpacing(6);
    verticalLayout_z->setObjectName(QStringLiteral("horizontalLayout_z"));

    verticalLayout_x->addLayout(horizontalLayout_y); //GamesLayout add rank/games
    verticalLayout_x->addLayout(verticalLayout_y); //GamesLayout add chat
    horizontalLayout_x->addLayout(verticalLayout_x); //Main Add GameChat
    horizontalLayout_x->addLayout(verticalLayout_z); //Main Add UserList

    listWidget_x = new QListWidget(tab_x);
    listWidget_x->setObjectName(QStringLiteral("listWidget_5"));

    horizontalLayout_y->addWidget(listWidget_x);

    listWidget_y = new QListWidget(tab_x);
    listWidget_y->setObjectName(QStringLiteral("listWidget_6"));

    horizontalLayout_y->addWidget(listWidget_y);

    //listWidget_z = new QListWidget(tab_x);
    //listWidget_z->setObjectName(QStringLiteral("listWidget_7"));

    //horizontalLayout_x->addWidget(listWidget_z);

    verticalLayout_z->addWidget(user_list_x);

    verticalLayout_y->addWidget(textBrowser_x);

    verticalLayout_x->setStretch(0,1);
    verticalLayout_x->setStretch(1,3);
    horizontalLayout_x->setStretch(0, 6);
    horizontalLayout_x->setStretch(1, 1);

    listWidget_x->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    listWidget_y->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    user_list_x->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));

    listWidget_x->setMinimumHeight(110);
    listWidget_y->setMinimumHeight(110);

    listWidget_x->setMaximumHeight(110);
    listWidget_y->setMaximumHeight(110);

    user_list_x->setMinimumWidth(125);
    user_list_x->setMaximumWidth(125);

    /*set background colors of widgets
    textBrowser_x->setStyleSheet("background-color: grey;"
                                 "font: 10pt;");
    user_list_x->setStyleSheet("background-color: grey;"
                               "font: 8pt;");
    listWidget_x->setStyleSheet("background-color: grey;"
                                "font: 8pt;");
    listWidget_y->setStyleSheet("background-color: grey;"
                                "font: 8pt;");
    */

    //double click games_list
    connect(listWidget_x, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(itemDoubleClicked(QListWidgetItem*)));

    //can click on user list
    //user_list_x->setContextMenuPolicy(Qt::CustomContextMenu);
    //connect(user_list_x, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));

    //listWidget_x->addItem("Game1");
    //listWidget_y->addItem("Rank2");
    //listWidget_z->addItem("Test3");
    //user_list_x->sortItems(Qt::DescendingOrder);
    //container[channel[x]->qt_tab_id-1].user_lists->sortItems(Qt::AscendinOrder);

    Widget_Pointers *ptr = (Widget_Pointers*)malloc(sizeof(Widget_Pointers));
    ptr->browsers = textBrowser_x;
    ptr->user_lists = user_list_x;
    ptr->rank_lists = listWidget_y;
    ptr->games_lists = listWidget_x;

    ptr->hLayoutX = horizontalLayout_x;
    ptr->hLayoutY = horizontalLayout_y;
    ptr->vLayoutX = verticalLayout_x;
    ptr->vLayoutY = verticalLayout_y;
    ptr->vLayoutZ = verticalLayout_z;

    return ptr;
}

/* unused right now
void MainWindow::showContextMenu(const QPoint& pos){

        int tab_index = ui->tabs->currentIndex();
        tab_index--;
        // Handle global position
        QPoint globalPos = container[tab_index].user_lists->mapToGlobal(pos);
        // EDITED THIS FUNCTION TO BECOME ABOVE - QPoint globalPos = listWidget->mapToGlobal(pos);

        // Create menu and insert some actions
        QMenu myMenu;
        myMenu.addAction("Insert", this, SLOT(addItem()));
        myMenu.addAction("Erase",  this, SLOT(eraseItem()));
        myMenu.addAction("Compare",  this, SLOT(compareItem()));

        // Show context menu at handling position
        myMenu.exec(globalPos);

}
*/

/*
void MainWindow::addItem() {
    int tab_index = ui->tabs->currentIndex();
    tab_index--;
    static int i = 0;
    container[tab_index].user_lists->addItem(QString::number(++i));
}*/

// what the fuck is this
/*
void MainWindow::compareItem() {
    int tab_index = ui->tabs->currentIndex();
    tab_index--;

}*/

void MainWindow::enter_chat() {
    QString message = ui->chat_input->text().trimmed();
    std::string msg = message.toStdString();
    //Get current tab to get chan_id to pass message to proper text browser
    int tab_index = ui->tabs->currentIndex();

    chat_message_header header;
    header.channel = 0;
    int z = -1;

    for (uint32_t x = 0; x < n_channels ; x++){
        if (channel[x]->qt_tab_id == tab_index){
            header.channel = channel[x]->channel_id;
            z = x;
            x = n_channels;
        }
    }

    if (message == ""){
        return;
    }
    //Saves entered input for up/down history
    if ( z != -1){
        if (!channel[z]->history_list){
            qDebug() << "History List Error, Should Never Happen";
            return;
        }
        fflush(stdout);
        if (tab_index != 0){
            if (channel[z]->history_list->size() == 10){
                fflush(stdout);
                for(int i = 0 ; i < 9; i++){
                    channel[z]->history_list->swap(i,i+1);
                }
                channel[z]->history_list->removeAt(9);
                channel[z]->history_list->insert(9,message);
                channel[z]->current_history = 10;
             }
            else{
                fflush(stdout);
                channel[z]->current_history = channel[z]->history_list->size()+1;
                *channel[z]->history_list << (QString) message;
            }
        }
    }
    //if message received = clear, clear chat of current browser
    if (msg == ".clear" || msg == "/clear" || msg =="-clear"){
        if (tab_index == 0){
            ui->status_browser->clear();
            ui->chat_input->clear();
            ui->chat_input->setFocus();
            return;
        }
        for (uint y = 0; y < n_channels; ++y) {
            if (channel[y] && channel[y]->qt_tab_id == tab_index) {
                fflush(stdout);
                channel[y]->channel_data->browsers->clear();
                ui->chat_input->clear();
                ui->chat_input->setFocus();
                return;
            }
        }
    }

    unsigned char data[1000];
    header.user_id = 0;
    //header.channel = 0;
    header.msg_flags = 0;
    const int len = msg.length() + 1;
    if (len == 1) return;
    header.msg_length = len;
    data[0] = CLIENT_MESSAGE_CHAT_MESSAGE;
    memcpy(&data[1],&header,sizeof(header));
    memcpy(&data[1+sizeof(header)], msg.c_str(), len);
    socket.write((char*)data, 1 + sizeof(header) + header.msg_length);

    ui->chat_input->clear();
    // Put the focus back into the input box so they can type again:
    ui->chat_input->setFocus();
}

void MainWindow::setupTabs() {
    int temp = ui->tabs->currentIndex();
    QString foo,tmp;
        for (int i = 0; i < 7; ++i)
        {
            tmp = QString::number(n_tabs+1);
            foo = "Tab " + tmp;
            QWidget * w = new QWidget;
            temp = ui->tabs->addTab(w, foo);
            delete w;
            //printf("%d \n",temp);
            ui->tabs->setTabText(temp, foo );
            ui->tabs->setCurrentIndex(temp);
            n_tabs++;
        }
    ui->tabs->setCurrentIndex(0);
    //connect(ui->widget, SIGNAL(currentChanged(int)),this,SLOT(onTabChanged(int)));
}

void MainWindow::closeTab_(int index) {
    // Remove the tab using removeTab(). Be aware that the
    // page widget itself is not deleted!

    //dont wanna remove status tab
    if (index == 0){
        return;
    }
    //send packet to say i am leaving chat tab
    for (uint x = 0; x < n_channels ; x++){
        if (channel[x]->qt_tab_id == index){
            std::string msg = "/leave";
            chat_message_header header;
            unsigned char data[1000];
            header.user_id = 0;
            header.channel = channel[x]->channel_id;
            header.msg_flags = 0;
            int len = msg.length() + 1;
            header.msg_length = len;
            data[0] = CLIENT_MESSAGE_CHAT_MESSAGE;
            memcpy(&data[1],&header,sizeof(header));
            memcpy(&data[1+sizeof(header)], msg.c_str(), len);
            socket.write((char*)data, 1 + sizeof(header) + header.msg_length);

            x = n_channels;
        }
    }
}

void MainWindow::manual_connect() {
    if (connection_attempted == 2) {
        socket.connectToHostEncrypted("sniperdad.com", 8443);

        QString myString = "Connecting to server....";
        QString styledString="<span style=\" font-size:10pt; font-weight:600; color:#D21B1B;\" > ";
        styledString.append(myString);
        styledString.append("</span>");
        ui->status_browser->setHtml(styledString);
        //ui->status_browser->append(styledString);

        login_sent = 0;
        connection_attempted = 1;
    }
}

void MainWindow::update(){
    if (!connection_attempted) {
        socket.connectToHostEncrypted("sniperdad.com", 8443);
        ui->status_browser->append("Connecting to server....");
        login_sent = 0;
        connection_attempted = 1;
    } else {
        if (!login_sent) {
            login_sent = 1;
            ui->status_browser->append("Connected.");

            if (use_token) {
                user_login_msg login;
                login.version = NETWORK_PROTOCOL_VERSION;
                login.using_token = 1;
                login.region = 0; // ??
                memcpy(login.username, username, sizeof(username));
                memcpy(login.auth, token, sizeof(token));
                unsigned char data[1+sizeof(login)];
                data[0] = CLIENT_MESSAGE_CONNECT;
                memcpy(&data[1], &login, sizeof(login));
                socket.write((char*)data, 1+sizeof(login));
            } else {
                ui->status_browser->append("Use /login (user) (pass) to login.");
                ui->status_browser->append("Adding -save after your password will automatically login in the future.");
            }

        }
        const int ret = socket.read(&buffer[buffer_pos], RECV_MAXLEN - buffer_pos);
        if (ret > 0) {
            unsigned int remaining_length = ret;
            unsigned char *position = (unsigned char*)buffer;
            while (remaining_length) {
                if (position[0] >= 200) { // disconnect msg
                    if (position[0] == SERVER_MESSAGE_DISCONNECT_SERVER_SHUTDOWN) {
                        const char *dcmsg = "Disconnected from server. Server is shutting down. Please restart your client.";
                        for (uint x = 0; x < n_channels; ++x) {
                            channel[x]->channel_data->browsers->append(dcmsg);
                        }
                        ui->status_browser->append(dcmsg);
                    } else if (position[0] == SERVER_MESSAGE_DISCONNECT_USER_LOGGED_IN_ELSEWHERE) {
                        const char *dcmsg = "Disconnected from server. User logged in elsewhere.";
                        for (uint x = 0; x < n_channels; ++x) {
                            channel[x]->channel_data->browsers->append(dcmsg);
                        }
                        ui->status_browser->append(dcmsg);
                    } else printf("Disconnected from server. Error code: %i\n",position[0]);fflush(stdout);
                    buffer_pos = 0;

                    if (position[0] != SERVER_MESSAGE_DISCONNECT_USER_LOGGED_IN_ELSEWHERE) connection_attempted = 0;
                    else connection_attempted = 2;
                    socket.close();
                    return;
                } else {
                    //printf("msg type %i\n",position[0]);fflush(stdout);
                    switch (position[0]) {
                        case SERVER_MESSAGE_AUTH_SUCCESSFUL:
                            while (n_channels) {
                                delete ui->tabs->widget(channel[0]->qt_tab_id);

                                free(channel[0]->users);
                                free(channel[0]);
                                // shift tabs over
                                n_channels--;
                                for (uint32_t y = 0; y < n_channels; ++y) {
                                    channel[y] = channel[y+1];
                                    channel[y]->qt_tab_id--;
                                }
                            }
                            remaining_length -= 1;
                            position = &position[1];
                            break;
                        case SERVER_MESSAGE_BAD_AUTH_TOKEN:
                            remove("user");
                            ui->status_browser->append("Server reported bad or expired token. Log in manually by using /login (username) (password).");

                            remaining_length -= 1;
                            position = &position[1];
                            break;
                        case SERVER_MESSAGE_NEW_AUTH_TOKEN: {
                            token_update update;
                            memcpy(&update, &position[1], sizeof(update));
                            FILE *f = fopen("user","w");
                            if (f) {
                                fprintf(f,"%s\n%s",update.username,update.token);
                                fclose(f);
                            }

                            remaining_length -= 1+sizeof(update);
                            position = &position[1+sizeof(update)];
                            break;
                        }
                        case SERVER_MESSAGE_USER_INFO:
                            if (remaining_length > sizeof(user_info)) {
                                user_info info;
                                memcpy(&info, &position[1], sizeof(info));
                                users[info.user_id] = std::string(info.username);
                                remaining_length -= 1 + sizeof(info);
                                position = &position[1+sizeof(info)];
                            } else {
                                goto buffer_remaining;
                            }
                            break;
                        case SERVER_MESSAGE_CHANNEL_UPDATE_GAMEINFO: {
                            if (remaining_length > sizeof(game_state)) {
                                game_state new_state;
                                memcpy(&new_state, &position[1], sizeof(new_state));
                                for (uint32_t x = 0; x < n_channels; ++x) {
                                    if (channel[x] && channel[x]->channel_id == new_state.league_id) {
                                        if (new_state.game_id >= channel[x]->n_games) {
                                            game_state *ptr = (game_state*)malloc(sizeof(game_state)*(new_state.game_id+1));
                                            memset(ptr, 0, sizeof(game_state)*(new_state.game_id+1));
                                            memcpy(ptr, channel[x]->games, sizeof(game_state)*channel[x]->n_games);
                                            free(channel[x]->games);
                                            channel[x]->games = ptr;
                                            channel[x]->games[new_state.game_id] = new_state;
                                            channel[x]->n_games = new_state.game_id+1;
                                        } else {
                                            channel[x]->games[new_state.game_id] = new_state;
                                        }
                                        redraw_game_list(x);
                                        //container[channel[x]->qt_tab_id-1].games_lists->addItem("hello");
                                        x = n_channels;
                                    }
                                }
                                remaining_length -= 1 + sizeof(new_state);
                                position = &position[1+sizeof(new_state)];
                            } else goto buffer_remaining;
                            break;
                        }
                        case SERVER_MESSAGE_CHANNEL_UPDATE_LEADERBOARDS: {
                            if (remaining_length > sizeof(rank_state)) {
                                rank_state new_state;
                                memcpy(&new_state, &position[1], sizeof(rank_state));
                                for (uint16_t x = 0; x < n_channels; ++x) {
                                    if (channel[x]->channel_id == new_state.channel) {
                                        channel[x]->ratings[new_state.position] = new_state.rating;
                                        strncpy(channel[x]->rankings[new_state.position], new_state.name, 64);
                                        redraw_rank_list(x);
                                        x = n_channels;
                                    }
                                }
                                remaining_length -= 1 + sizeof(rank_state);
                                position = &position[1+sizeof(rank_state)];
                            } else goto buffer_remaining;
                            break;
                        }
                        case SERVER_MESSAGE_CHAT_MESSAGE: {
                            if (remaining_length > sizeof(chat_message_header)) {
                                chat_message_header header;
                                memcpy(&header, &position[1], sizeof(header));
                                if (remaining_length - 1 - sizeof(header) >= header.msg_length) {
                                    char msgbuffer[2000];
                                    memcpy(msgbuffer, &position[1+sizeof(header)], header.msg_length);
                                    msgbuffer[header.msg_length] = 0;
                                    if (header.user_id == 0){
                                        //const int tab_index = ui->tabs->currentIndex();
                                        if (header.msg_flags & MSG_FLAG_PRINTALL){
                                            ui->status_browser->append(msgbuffer);
                                        }
                                    }
                                    for (uint8_t x = 0; x < n_channels; ++x) {
                                        if (channel[x] && (channel[x]->channel_id == header.channel || header.msg_flags & MSG_FLAG_PRINTALL)) {
                                            channel[x]->channel_data->browsers->append(msgbuffer);
                                            /*
                                            if (header.msg_flags & MSG_FLAG_IMPORTANT) {
                                                ui->tabs->setCurrentIndex(channel[x]->qt_tab_id);
                                            }*/
                                        }
                                    }
                                    remaining_length -= 1 + sizeof(header) + header.msg_length;
                                    position = &position[1 + sizeof(header) + header.msg_length];
                                } else goto buffer_remaining;
                            } else goto buffer_remaining;
                            break;
                        }
                        case SERVER_MESSAGE_AUTH_ERROR:
                            // no special way to handle this.
                            remaining_length--;
                            position = &position[1];
                            break;
                        case SERVER_MESSAGE_PING: {
                            const char pong = CLIENT_MESSAGE_PONG;
                            socket.write(&pong, 1);
                            remaining_length--;
                            position = &position[1];
                            break;
                        }
                        case SERVER_MESSAGE_CONNECT:
                            if (remaining_length > sizeof(server_connect_msg)) {
                                server_connect_msg conmsg;
                                memcpy(&conmsg, &position[1], sizeof(conmsg));
                                // this message is currently irrelevant. figure out a way to handle client updates and it will become relevant.

                                remaining_length -= 1 + sizeof(conmsg);
                                position = &position[1+sizeof(conmsg)];
                            } else goto buffer_remaining;
                            break;
                        case SERVER_MESSAGE_CHANNEL_REMOVE_USER:
                            if (remaining_length > sizeof(chat_user)) {
                                chat_user user_update;
                                memcpy(&user_update, &position[1], sizeof(user_update));

                                for (uint x = 0; x < n_channels; ++x) {
                                    if (channel[x] && channel[x]->channel_id == user_update.channel) {
                                        channel[x]->completer_list->clear();
                                        for (int y = 0; y < channel[x]->n_users; ++y) {
                                            if (channel[x]->users[y].user_id == user_update.user_id) {
                                                channel[x]->n_users--;

                                                channel[x]->users[y] = channel[x]->users[channel[x]->n_users];
                                                y = channel[x]->n_users;
                                            }
                                        }
                                        channel[x]->channel_data->user_lists->clear();
                                        for (int y = 0; y < channel[x]->n_users; ++y) {
                                            const char *username = channel[x]->users[y].username;
                                            channel[x]->channel_data->user_lists->addItem(username);
                                            *channel[x]->completer_list << (QString)username;
                                        }
                                        channel[x]->channel_data->user_lists->sortItems(Qt::AscendingOrder);
                                        x = n_channels;
                                    }
                                }
                                remaining_length -= 1 + sizeof(user_update);
                                position = &position[1+sizeof(user_update)];
                            } else goto buffer_remaining;
                            break;
                        case SERVER_MESSAGE_CHANNEL_ADD_USER:
                            if (remaining_length > sizeof(chat_user)) {
                                user_info user_update;
                                memcpy(&user_update, &position[1], sizeof(user_update));

                                for (uint x = 0; x < n_channels; ++x) {
                                    if (channel[x] && channel[x]->channel_id == user_update.channel_id) {


                                        user_info *ptr = (user_info*)malloc(sizeof(user_info)*(channel[x]->n_users+1));
                                        memcpy(ptr, channel[x]->users, sizeof(user_info) * channel[x]->n_users);
                                        free(channel[x]->users);
                                        channel[x]->users = ptr;
                                        channel[x]->users[channel[x]->n_users] = user_update;
                                        channel[x]->n_users++;

                                        *channel[x]->completer_list=empty;
                                        channel[x]->channel_data->user_lists->clear();
                                        // i dont actually remember why we're clearing this and readding everyone
                                        for (int y = 0; y < channel[x]->n_users; ++y) {
                                            const char *username = channel[x]->users[y].username;
                                            channel[x]->channel_data->user_lists->addItem(username);
                                            *channel[x]->completer_list << (QString)username;
                                        }

                                        channel[x]->channel_data->user_lists->sortItems(Qt::AscendingOrder);
                                        x = n_channels;
                                    }

                                }
                                remaining_length -= 1 + sizeof(user_info);
                                position = &position[1+sizeof(user_info)];
                            } else goto buffer_remaining;
                            break;
                        case SERVER_MESSAGE_CHANNEL_UPDATE_USER:
                            if (remaining_length > sizeof(user_info)) {
                                user_info update;
                                memcpy(&update, &position[1], sizeof(update));

                                for (uint x = 0; x < n_channels; ++x) {
                                    if (channel[x] && channel[x]->channel_id == update.channel_id) {

                                        for (int y = 0; y < channel[x]->n_users; ++y) {
                                            if (channel[x]->users[y].user_id == update.user_id) {
                                                channel[x]->users[y] = update;

                                                for (int z = 0; z < channel[x]->n_users; ++z) {
                                                     const char *username = channel[x]->users[z].username;
                                                     channel[x]->channel_data->user_lists->addItem(username);
                                                }
                                                y = channel[x]->n_users;
                                                x = n_channels;
                                            }
                                        }
                                        channel[x]->channel_data->user_lists->sortItems(Qt::AscendingOrder);
                                        x = n_channels;
                                    }
                                }
                                remaining_length -= 1 + sizeof(user_info);
                                position = &position[1+sizeof(user_info)];
                            } else goto buffer_remaining;
                            break;
                        case SERVER_MESSAGE_CHANNEL_JOIN:
                            if (remaining_length > sizeof(channel_info)) {
                                channel_info new_channel;
                                memcpy(&new_channel, &position[1], sizeof(new_channel));

                                Channel **ptr = (Channel**)malloc(sizeof(Channel*)*(n_channels+1));
                                memcpy(ptr, channel, sizeof(Channel**)*n_channels);
                                free(channel);
                                channel = ptr;
                                channel[n_channels] = (Channel*)malloc(sizeof(Channel));
                                channel[n_channels]->flags = new_channel.channel_flags;
                                channel[n_channels]->channel_id = new_channel.channel_id;
                                channel[n_channels]->n_users = 0;
                                channel[n_channels]->n_games = 0;
                                channel[n_channels]->games = NULL;
                                channel[n_channels]->users = NULL;
                                channel[n_channels]->completer_list = new QStringList;
                                channel[n_channels]->history_list = new QList<QString>;
                                channel[n_channels]->current_history = 0;
                                memcpy(channel[n_channels]->channel_name,new_channel.channel_name,sizeof(new_channel.channel_name));

                                QString chan_n;
                                chan_n = QString::fromStdString(new_channel.channel_name);

                                channel[n_channels]->w = new QWidget;
                                const int temp = ui->tabs->addTab(w, chan_n);
                                channel[n_channels]->channel_data = createTab(w);
                                n_tabs++;

                                ui->tabs->setTabText(temp, chan_n);
                                ui->tabs->setCurrentIndex(temp);

                                channel[n_channels]->qt_tab_id = temp ;
                                for (int x = 0; x < 100; ++x) {
                                    channel[n_channels]->rankings[x][0] = '\0';
                                    channel[n_channels]->ratings[x] = 0;
                                }

                                n_channels++;

                                remaining_length -= 1 + sizeof(channel_info);
                                position = &position[1+sizeof(channel_info)];
                            } else goto buffer_remaining;
                            break;
                        case SERVER_MESSAGE_CHANNEL_LEAVE:
                            if (remaining_length > sizeof(uint16_t)) {
                                uint16_t id;
                                memcpy(&id, &position[1], sizeof(id));
                                for (uint x = 0; x < n_channels; ++x) {
                                    if (channel[x] && channel[x]->channel_id == id) {
                                        n_tabs--;
                                        delete channel[x]->channel_data->browsers;
                                        delete channel[x]->channel_data->games_lists;
                                        delete channel[x]->channel_data->user_lists;
                                        delete channel[x]->channel_data->rank_lists;
                                        /*
                                        verticalLayout_x->addLayout(horizontalLayout_y); //GamesLayout add rank/games
                                        verticalLayout_x->addLayout(verticalLayout_y); //GamesLayout add chat
                                        horizontalLayout_x->addLayout(verticalLayout_x); //Main Add GameChat
                                        horizontalLayout_x->addLayout(verticalLayout_z); //Main Add UserList
                                        so delete HX and VY first since child of VX.. then VX can be deleted, then VZ, then HX last
                                        */
                                        delete channel[x]->channel_data->hLayoutY;
                                        delete channel[x]->channel_data->vLayoutY;
                                        delete channel[x]->channel_data->vLayoutX;
                                        delete channel[x]->channel_data->vLayoutZ;
                                        delete channel[x]->channel_data->hLayoutX;

                                        delete channel[x]->completer_list;
                                        delete channel[x]->history_list;
                                        delete ui->tabs->widget(channel[x]->qt_tab_id);

                                        free(channel[x]->users);
                                        free(channel[x]);
                                        // shift tabs over
                                        n_channels--;
                                        for (uint y = x; y < n_channels; ++y) {
                                            channel[y] = channel[y+1];
                                            channel[y]->qt_tab_id--;
                                        }
                                        x = n_channels;
                                    }
                                }
                                remaining_length -= 1 + sizeof(uint16_t);
                                position = &position[1+sizeof(uint16_t)];
                            } else goto buffer_remaining;
                            break;
                        default:
                            break;
                            // protocol error condition. something went horribly wrong.
                    }
                }
                if (0) { // openssl tier memery - i'd write a separate function but i dont even knwo the context of this shit
                    // buffer our incomplete messaage for next read.
                    buffer_remaining:
                    memmove(buffer, position, remaining_length);
                    buffer_pos = remaining_length;
                }
            }
        }
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loginButton_clicked()
{
   //ui->stackedWidget->setCurrentWidget(chatroom);
   // connect(socket, SIGNAL(encrypted()), this, SLOT(ready()));
    //socket.connectToHostEncrypted("sniperdad.com", 8443);
    //socket.read();

}

void MainWindow::chatbutton_clicked()
{   //Send data to server when clicked
    //socket.write(test_message, 1+sizeof(h)+h.msg_length);
}

void MainWindow::update_user_list(){
    //get user data from server

    //input user into list

    //function adds user in list to list widget
   /*
    for (int i = 0; i < user_list_len ; i++){
        new QListWidgetItem(picture, user[i], userListWidget);
    }
*/
}

void MainWindow::update_chat(){

}

void MainWindow::clear_chat(){
    ui->status_browser->clearHistory();
}

void MainWindow::pushButton_clicked(){

}

// DO THIS WHEN CONNECTED
//  ui->stackedWidget->setCurrentIndex(1);

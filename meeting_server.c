/*
  meeting_server.c
*/

#include "/Users/sannamiyusaku/CLionProjects/mynet/mynet.h"
//#include "mynet.h"

#include "meeting.h"
#include <errno.h>

#define S_BUFSIZE 512 /* 送信用バッファサイズ */
#define R_BUFSIZE 512 /* 受信用バッファサイズ */
#define BACKLOG     5 /* 接続を待ち受けるためにOSが用意する待ち行列の数 */

#define EMPTY 0

/* 大域変数 global variables */
member client = NULL;   /* クライアントの情報（線形リスト） */
fd_set mask;

/* プライベート関数 static */
static void send_client(char *buf, int sock);
static void login_client(char *username, int sock);
static void logout_client(char *username, int sock);

/* サーバメインルーチン */
void meeting_server(char* username, int port_number) {
    member p;
    struct idobata *packet;

    char   s_buf[S_BUFSIZE];
    char   r_buf[R_BUFSIZE];
    char message[S_BUFSIZE];
    int  max_sd; /* ディスクリプタ最大値 */
    int strsize;
    int udpsock, tcpsock_listen, tcpsock_accepted;
    //int count = 0;

    fd_set readfds;
    struct sockaddr_in from_adrs;
    socklen_t from_len;

    /* サーバの初期化 */
    udpsock          = init_udpserver(port_number);
    tcpsock_listen   = init_tcpserver(port_number, BACKLOG);
    tcpsock_accepted = EMPTY; /* サーバ起動時は未登録 */

    /* ビットマスクの準備 */
    FD_ZERO(&mask);
    FD_SET(0, &mask);           /* 標準入力（キーボード） */
    FD_SET(udpsock, &mask);        /* UDPソケット */
    FD_SET(tcpsock_listen, &mask); /* TCPソケット(Listen) */

    /* メインループ */
    for(;;) {
        //printf("count: %d\n", count++);
        /* ----------受信データの有無をチェック----------- */
        readfds = mask;
        /* 全ソケットを探索して最大値を毎回のループで探す */
        max_sd = max_socket(client, udpsock, tcpsock_listen, tcpsock_accepted);
        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        /* バッファを空にする */
        memset(s_buf, '\0', sizeof(s_buf));
        memset(r_buf, '\0', sizeof(r_buf));
        memset(message, '\0', sizeof(message));

        if( FD_ISSET(0, &readfds) ) {                 /* 標準入力（キーボード） */
            fgets(s_buf, MESG_LENGTH, stdin);
            snprintf(message, S_BUFSIZE, "[%s]%s", username, s_buf);
            create_packet(s_buf, MESG, message);
            send_client(s_buf, -1);    /* サーバの発言メッセージを全クライアントに送信 */
        }

        if( FD_ISSET(udpsock, &readfds) ) {              /* UDPソケット */
            from_len = sizeof(from_adrs);
            strsize = Recvfrom(udpsock, r_buf, R_BUFSIZE-1, 0,
                     (struct sockaddr *) &from_adrs, &from_len);
        }

        if( FD_ISSET(tcpsock_listen, &readfds) ) {       /* TCPソケット(Listen) */
            tcpsock_accepted = Accept(tcpsock_listen, NULL, NULL);
            /* TCPソケット(accept)
             * （= 接続したクライアントの個別のTCPソケット(p->sock)）のビットマスクをセット */
            FD_SET(tcpsock_accepted, &mask);
        }

        if(tcpsock_accepted != EMPTY) {
            if( FD_ISSET(tcpsock_accepted, &readfds) ) { /* TCPソケット(Accept) */
                strsize = Recv(tcpsock_accepted, r_buf, R_BUFSIZE-1, 0);
            }
        }

        for(p = client; p != NULL; p = p->next) {
                if(FD_ISSET(p->sock, &readfds)) {     /* TCPソケット(各クライアント) */
                    fflush(stdout);
                    strsize = Recv(p->sock, r_buf, R_BUFSIZE-1, 0);
                    break;
                }
        }

        /* ----------パケットの解析と処理---------- */
        r_buf[strsize] = '\0';

        /*「QUIT」パケットを送信せずに異常終了したクライアントをログアウトさせる */
        if(strsize == 0 || strncmp(r_buf, "POST POST ", 10) == 0) {
            fprintf(stderr, "The connection has been lost.\n");
            logout_client(p->username, p->sock);
            strsize = 1;
        }
        else {
            packet = (struct idobata *)r_buf; /* packetがバッファの先頭を指す */

            switch (analyze_header(packet->header)) { /* ヘッダに応じて分岐 */

                case HELO: /*「HERE」パケットを送信 */
                    create_packet(s_buf, HERE, NULL);
                    strsize = (int)strlen(s_buf);
                    Sendto(udpsock, s_buf, strsize, 0,
                           (struct sockaddr *) &from_adrs, sizeof(from_adrs));
                    break;

                case JOIN: /* ログイン処理 */
                    chop_nl(packet->data);    /* Usernameに改行があれば除く */
                    login_client(packet->data, tcpsock_accepted);
                    tcpsock_accepted = EMPTY; /* ログイン処理後はaccept用のソケットを空に戻す */
                    break;

                case POST: /*「MESG」パケットを送信 */
                    snprintf(message, S_BUFSIZE, "[%s]%s", p->username, packet->data);
                    printf("%s", message);
                    fflush(stdout);
                    create_packet(s_buf, MESG, message);
                    send_client(s_buf, p->sock); /* 発言者以外のクライアントに送信 */
                    break;

                case QUIT: /* ログアウト処理 */
                    logout_client(p->username, p->sock);
                    break;

                default:
                    // do not anything
                    break;
            }
        }
    }
}

/* パケットの送信中にクライアントが抜ける場合を考慮して送信する関数 */
static void send_client(char *buf, int sock) {
    member p = client;
    int client_socket;

    /* sockで指定したソケット番号以外の全クライアントにパケットを送信 */
    if((client_socket = send_packet(client, buf, sock)) != 0) {
        if (errno == EPIPE) {
            while(p->sock == client_socket) {
                p = p->next;
            }

            /* シグナルSIGPIPEが発生したときは対象のクライアントをログアウトさせる */
            fprintf(stderr, "User [%s] has already disconnected.\n", p->username);
            logout_client(p->username, client_socket);
        } else {
            exit_errmesg("send()");
        }
    }
}

/* ログイン処理を行う関数 */
static void login_client(char *username, int sock) {
    if(client == NULL) { /* 一人目のユーザ */
        client = create_member();

        /* ユーザ情報を保存 */
        client->sock = sock;
        strncpy(client->username, username, NAME_LENGTH);
        client->next = NULL;
    }
    else {               /* 二人目以降のユーザ */
        insert_client(client, username, sock);
    }

    fprintf(stderr, "User [%s] logged in.\n", username);
}

/* ログアウト処理を行う関数 */
static void logout_client(char *username, int sock) {
    member temp;

    FD_CLR(sock, &mask);    /* ログアウトしたクライアントのディスクリプタを監視マスクから取り除く */
    close(sock);               /* ソケットをクローズ */

    if(client->sock == sock) { /* 削除対象が先頭ノードの場合 */
        temp = client;
        client = client->next;
        free(temp);
    } else {                   /* ２番目以降のノードの削除 */
        delete_client(client, sock);
    }

    fprintf(stderr, "User [%s] logged out.\n", username);
}
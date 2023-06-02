/*
  meeting_client.c
*/

#include "/Users/sannamiyusaku/CLionProjects/mynet/mynet.h"
//#include "mynet.h"

#include "meeting.h"

#include <sys/select.h>
#include <sys/time.h>


#define S_BUFSIZE 512   /* 送信用バッファサイズ */
#define R_BUFSIZE 512   /* 受信用バッファサイズ */

/* クライアントメインルーチン */
void meeting_client(char* username, char* servername, int port_number) {

    struct idobata *packet;
    char s_buf[S_BUFSIZE],r_buf[R_BUFSIZE];
    char message[MESG_LENGTH];
    int tcpsock, strsize;
    fd_set mask, readfds;

    /* サーバに接続する */
    tcpsock = init_tcpclient(servername, port_number);
    fprintf(stderr, "\nConnected to the server. [%s] [%d]\n"
                    "You can disconnect by Entering \"QUIT\".\n", servername, port_number);
    fflush(stdout);

    /* ビットマスクの準備 */
    FD_ZERO(&mask);
    FD_SET(0, &mask);    /* 標準入力（キーボード） */
    FD_SET(tcpsock, &mask); /* ソケット */

    /* 「JOIN」パケットを送信 */
    create_packet(s_buf, JOIN, username);
    strsize = (int)strlen(s_buf);
    Send(tcpsock, s_buf, strsize, 0);

    for(;;) {
        /* 受信データの有無をチェック */
        readfds = mask;
        select(tcpsock+1, &readfds, NULL, NULL, NULL);

        /* バッファを空にする */
        memset(s_buf, '\0', sizeof(s_buf));
        memset(r_buf, '\0', sizeof(r_buf));
        memset(message, '\0', sizeof(message));

        if( FD_ISSET(0, &readfds) ) {
            fgets(message, MESG_LENGTH, stdin);

            /* 「QUIT」パケットを送信してサーバとの接続を閉じる */
            if(strncmp(message, "QUIT", 4) == 0) {
                create_packet(s_buf, QUIT, NULL);
                strsize = (int)strlen(s_buf);
                Send(tcpsock, s_buf, strsize, 0);
                fprintf(stderr, "Disconnected.\n");
                break;
            }

            /* QUIT以外なら「POST」パケットを送信 */
            create_packet(s_buf, POST, message);
            strsize = (int)strlen(s_buf);
            Send(tcpsock, s_buf, strsize, 0);
        }

        if( FD_ISSET(tcpsock, &readfds) ) {
            strsize = Recv(tcpsock, r_buf, R_BUFSIZE-1, 0);

            /* サーバがダウンした場合はクライアントも終了する */
            if(strsize == 0) {
                fprintf(stderr, "\nServer is down.\n");
                break;
            }

            r_buf[strsize] = '\0';
            packet = (struct idobata *)r_buf; /* packetがバッファの先頭を指す */
            if(analyze_header(packet->header) == MESG) {
                printf("%s", packet->data); /* 発言メッセージを表示 */
                fflush(stdout);
            }
        }
    }

    close(tcpsock);
    exit(EXIT_SUCCESS);
}
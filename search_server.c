/*
  search_server.c
*/

#include "/Users/sannamiyusaku/CLionProjects/mynet/mynet.h"
//#include "mynet.h"

#include "meeting.h"

#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>


#define S_BUFSIZE      10   /* 送信用バッファサイズ */
#define R_BUFSIZE      10   /* 受信用バッファサイズ */
#define TIMEOUT_SEC     1   /* ブロードキャストがタイムアウトするまでの時間[s] */
#define CONECT_ATTEMPT  3   /* 接続試行回数 */

#define OK -1

/* 起動時のルーチン */
char *search_server(int port_number) {
    struct sockaddr_in broadcast_adrs;
    struct sockaddr_in from_adrs;
    struct idobata *packet;
    socklen_t from_len;

    int udpsock;
    int broadcast_sw = 1;
    fd_set mask, readfds;
    struct timeval timeout;

    char s_buf[S_BUFSIZE], r_buf[R_BUFSIZE];
    int strsize;
    int i;

    /* ブロードキャストアドレスの情報をsockaddr_in構造体に格納する */
    set_sockaddr_in_broadcast(&broadcast_adrs, (in_port_t)port_number);

    /* ソケットをDGRAMモードで作成する */
    udpsock = init_udpclient();

    /* ソケットをブロードキャスト可能にする */
    if(setsockopt(udpsock, SOL_SOCKET, SO_BROADCAST,
                  (void *)&broadcast_sw, sizeof(broadcast_sw)) == -1) {
        exit_errmesg("setsockopt()");
    }

    /* ビットマスクの準備 */
    FD_ZERO(&mask);
    FD_SET(udpsock, &mask); /* UDPソケット */

    /* 「HELLO」パケットを作成 */
    create_packet(s_buf, HELO, NULL);
    strsize = (int)strlen(s_buf);

    fprintf(stderr, "Looking for a server");

    /* サーバとの接続を試行 */
    for(i = 0; i < CONECT_ATTEMPT; i++) {
        /* 文字列をサーバに送信する */
        Sendto(udpsock, s_buf, strsize, 0,
               (struct sockaddr *)&broadcast_adrs, sizeof(broadcast_adrs));
        fprintf(stderr, ".");

        /* 受信待機 */
        for(;;) {

            /* 受信データの有無をチェック */
            readfds = mask;
            timeout.tv_sec = TIMEOUT_SEC;
            timeout.tv_usec = 0;

            if (select(udpsock+1, &readfds, NULL, NULL, &timeout) == 0) {
                if(i == CONECT_ATTEMPT-1) {
                    return "failed"; //接続に失敗したら試行を終了
                }
                break;
            }

            from_len = sizeof(from_adrs);
            Recvfrom(udpsock, r_buf, R_BUFSIZE-1, 0,
                               (struct sockaddr *) &from_adrs, &from_len);

            packet = (struct idobata *)r_buf; /* packetがバッファの先頭を指す */
            if(analyze_header(packet->header) == HERE) {
                i = OK;
                break;
            }
        }

        if(i == OK) {
            break; //接続に成功したら試行を終了
        }
    }

    close(udpsock);             /* ソケットをクローズ */

    /* サーバのIPアドレスを返す */
    return inet_ntoa(from_adrs.sin_addr);
}
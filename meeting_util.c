/*
  meeting_client.c
*/

#include "/Users/sannamiyusaku/CLionProjects/mynet/mynet.h"
//#include "mynet.h"

#include "meeting.h"

#include <stdlib.h>
#include <sys/select.h>

#define MSGBUF_SIZE 512

/* -----メインプログラムから呼び出される関数----- */

/* パケットの作成 */
char *create_packet(char *buffer, u_int32_t type, char *message ) {

    switch( type ){
        case HELO:
            snprintf( buffer, MSGBUF_SIZE, "HELO" );
            break;
        case HERE:
            snprintf( buffer, MSGBUF_SIZE, "HERE" );
            break;
        case JOIN:
            snprintf( buffer, MSGBUF_SIZE, "JOIN %s", message );
            break;
        case POST:
            snprintf( buffer, MSGBUF_SIZE, "POST %s", message );
            break;
        case MESG:
            snprintf( buffer, MSGBUF_SIZE, "MESG %s", message );
            break;
        case QUIT:
            snprintf( buffer, MSGBUF_SIZE, "QUIT" );
            break;
        default:
            /* Undefined packet type */
            return(NULL);
            //break;
    }
    return(buffer);
}

/* パケットのヘッダを解析 */
u_int32_t analyze_header( char *header ) {
    if( strncmp( header, "HELO", 4 ) == 0 ) return(HELO);
    if( strncmp( header, "HERE", 4 ) == 0 ) return(HERE);
    if( strncmp( header, "JOIN", 4 ) == 0 ) return(JOIN);
    if( strncmp( header, "POST", 4 ) == 0 ) return(POST);
    if( strncmp( header, "MESG", 4 ) == 0 ) return(MESG);
    if( strncmp( header, "QUIT", 4 ) == 0 ) return(QUIT);
    return 0;
}

/* 文字列を指定したソケット番号以外の全クライアントに送信する関数 */
int send_packet(member client, char *message, int sock) {
    member p;

    if(client != NULL) {
        for(p = client; p != NULL; p = p->next) {
            if(p->sock != sock) {
                if(send(p->sock, message, strlen(message), MSG_NOSIGNAL) == -1) {
                    /* シグナルSIGPIPEが発生したときは、対象のクライアントのソケット番号を返す */
                    return p->sock;
                }
            }
        }
    }
    return 0;
}

/* struct member型の変数へのメモリ割り当てを行う関数 */
member create_member() {
    member new;

    /* クライアント情報の保存用構造体の初期化 */
    if((new = (member)malloc(sizeof(struct _member))) == NULL) {
        exit_errmesg("malloc()");
    }

    return new;
}

/* ログイン処理を行う関数 */
void insert_client(member client, char *username, int sock) {
    member p = client;
    member new;

    while (p->next != NULL) {
        p = p->next;
    }

    /* クライアント情報の保存用構造体の初期化 */
    new = create_member();

    /* ユーザ情報を保存 */
    new->sock = sock;
    strncpy(new->username, username, NAME_LENGTH);
    new->next = NULL;

    /* 線形リストの末尾に接続 */
    p->next = new;
}

/* ログアウト処理を行う関数 */
void delete_client(member client, int sock) {
    member p = client;
    member temp;

    while (p->next != NULL) {
        if(p->next->sock == sock) {
            temp = p->next;
            p->next = temp->next;
            free(temp);
            break;
        }
        p = p->next;
    }
}

/* 全クライアントのソケット番号と、UDPとTCPのソケット番号の中から最大値を探す関数 */
int max_socket(member client, int sock1, int sock2, int sock3) {
    member p;
    int  max = (sock1 > sock2) ? sock1:sock2;
    max = (sock3 > max) ? sock3:max;

    if(client != NULL) {
        max = (client->sock > max) ? client->sock:max;

        for(p = client->next; p != NULL; p = p->next) {
            if(max < p->sock) {
                max = p->sock;
            }
        }
    }

    return max;
}

/* 文字列の最後に改行コードがあればそれを取り除く処理をする関数 */
char *chop_nl(char *s) {
    int len;
    len = (int)strlen(s);
    if( s[len-1] == '\n' ) {
        s[len-1] = '\0';
    }
    return(s);
}
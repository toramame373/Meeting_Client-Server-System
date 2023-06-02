/*
  meeting.h
*/

#ifndef MEETING_H
#define MEETING_H

#include "/Users/sannamiyusaku/CLionProjects/mynet/mynet.h"
//#include "mynet.h"

#define HELO    1
#define HERE    2
#define JOIN    3
#define POST    4
#define MESG    5
#define QUIT    6

#define NAME_LENGTH  15  /* ユーザ名の長さ */
#define MESG_LENGTH 488  /* 発言メッセージの長さ */

/* パケットの構造の定義 */
struct idobata {
    char header[4];   /* パケットのヘッダ部(4バイト) */
    char sep;         /* セパレータ(空白、またはゼロ) */
    char data[];      /* データ部分(メッセージ本体) */
};

/* ユーザ情報を管理する構造体の定義（線形リスト） */
typedef struct _member {
    char username[NAME_LENGTH];     /* ユーザ名 */
    int  sock;                      /* ソケット番号 */
    struct _member *next;           /* 次のユーザ */
} *member;

/* 起動時のルーチン */
char *search_server(int port_number);

/* サーバメインルーチン */
void meeting_server(char* username, int port_number);

/* クライアントメインルーチン */
void meeting_client(char* username, char* servername, int port_number);

/* パケットの作成 */
char *create_packet(char *buffer, u_int32_t type, char *message);

/* パケットのヘッダを解析 */
u_int32_t analyze_header( char *header );

/* 指定したソケット番号以外の全クライアントにパケットを送信する関数 */
int send_packet(member client, char *message, int sock);

/* struct member型の変数へのメモリ割り当てを行う関数 */
member create_member();

/* ノードの追加を行う関数 */
void insert_client(member client, char *username, int sock);

/* ノードの削除を行う関数 */
void delete_client(member client, int sock);

/* 全クライアントのソケット番号と、UDPとTCPのソケット番号の中から最大値を探す関数 */
int max_socket(member client, int sock1, int sock2, int sock3);

/* 文字列の最後に改行コードがあればそれを取り除く処理をする関数 */
char *chop_nl(char *s);

#endif //MEETING_H

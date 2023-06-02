/*
 * main.c
 */


/* 実行する環境に応じて適切な方のコメントアウトを外してビルドしてください
 * 提出時には演習室の環境用のインクルード文のコメントアウトを外しています
 * 全てのソースプログラムでmynet.hのインクルード部分は同様にしてあります */

// CLion（自分のPCの環境用）
#include "/Users/sannamiyusaku/CLionProjects/mynet/mynet.h"
// Eclipse（演習室環境用）
//#include "mynet.h"

#include "meeting.h"

#define SERVER_LEN   256   /* サーバ名格納用バッファサイズ */
#define DEFAULT_PORT 50001 /* ポート番号既定値 */

extern char *optarg;
extern int optind, opterr, optopt;

int main(int argc, char *argv[]) {
    int port_number = DEFAULT_PORT;
    char username[NAME_LENGTH] = "Anonymous"; /* 匿名のユーザー */;
    char servername[SERVER_LEN];
    int c;

    /* オプション文字列の取得 */
    opterr = 0;
    while (1) {
        c = getopt(argc, argv, "n:p:h");
        if (c == -1) break;

        switch (c) {

            case 'n' :  /* サーバ名の指定 */
                snprintf(username, NAME_LENGTH, "%s", optarg);
                break;

            case 'p':  /* ポート番号の指定 */
                port_number = (int)atoi(optarg);
                break;

            case '?' :
                fprintf(stderr, "Unknown option '%c'\n", optopt);
            case 'h' :
                fprintf(stderr, "Usage: %s -n user_name -p port_number \n", argv[0]);
                exit(EXIT_FAILURE);
                //break;
            default:
                // do not anything
                break;
        }
    }

    /* サーバを探す */
    strcpy(servername, search_server(port_number));

    /* サーバーが見つからなければ、自身がサーバになる */
    if(strcmp(servername, "failed") == 0) {
        fprintf(stderr, "\nServer is not found.\nFrom now on you are the server.\n");
        meeting_server(username, port_number);
    }
    else {
        meeting_client(username, servername, port_number);
    }

    exit(EXIT_SUCCESS);
}
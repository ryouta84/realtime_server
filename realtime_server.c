/**
 * 固定長でデータを送受信するリアルタイムサーバー
 * 起動時の引数で参加プレイヤー数を指定する
 */

#include <arpa/inet.h>
#include <memory.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "realtime_server.h"

player_info *player_infos;

int is_little_endian()
{
    int x = 1;
    if (*(char *)&x)
    {
        return 1;
    }
    return 0;
}

/**
 * floatのbyte列を反転させる
 */
float reverse_float(float value)
{
    if (!is_little_endian())
    {
        return value;
    }

    float ret_value;
    unsigned char temp32[4];
    memcpy(temp32, &value, sizeof(value));
    unsigned char swap_temp[4];
    // バイトを入れ替える
    memcpy(swap_temp, &value, sizeof(value));
    memcpy(temp32, swap_temp + 3, sizeof(char));
    memcpy(temp32 + 1, swap_temp + 2, sizeof(char));
    memcpy(temp32 + 2, swap_temp + 1, sizeof(char));
    memcpy(temp32 + 3, swap_temp, sizeof(char));
    memcpy(&ret_value, temp32, sizeof(temp32));

    return ret_value;
}

/**
 * self_sock以外の全プレイヤーに送信する
 * self_sockを0に指定することで参加している全プレイヤーに送信することができる
 */
void write_other_players(int connected_sock_array[], size_t len, void *buf, size_t buf_size, int self_sock)
{
    for (size_t i = 0; i < len; i++)
    {
        if (connected_sock_array[i] == self_sock)
        {
            continue;
        }
#ifdef DEBUG
        printf("size = %d\n", (u_int32_t)buf_size);
        payload *data = (payload *)buf;
        printf("fd = %d < player_id=%3d, command=%3d, command_target=%3d, padding=%3d hp=%d, x=%f, y=%f\n", connected_sock_array[i], data->player_id, data->command, data->command_target, data->padding, (int)ntohl(data->hp), reverse_float(data->x), reverse_float(data->y));
#endif
        // ネットワークバイトオーダーのままなので変換不要
        int written_size = write(connected_sock_array[i], buf, buf_size);
        // TODO: 一回で全てのデータをwriteできなかった場合に対応する
    }
}

/**
 * ソケットで受信できるまで待機する
 * read可能なソケットがresultに入る
 * connected_sock_arrayとresultは同じサイズにすること
 */
int polling(int connected_sock_array[], size_t len, int result[])
{
    fd_set read_player_set;
    FD_ZERO(&read_player_set);
    for (size_t i = 0; i < len; i++)
    {
        FD_SET(connected_sock_array[i], &read_player_set);
    }

    // TODO: 必ずしも末尾の要素のソケットが最大値とは限らない?
    // 何か来るまで待ち続ける
    int select_result = select(connected_sock_array[len - 1] + 1, &read_player_set, NULL, NULL, NULL);
    if (select_result <= 0)
    {
        perror("select error\n");
        return -1;
    }

    for (size_t i = 0; i < len; i++)
    {
        if (FD_ISSET(connected_sock_array[i], &read_player_set))
        {
            result[i] = connected_sock_array[i];
        }
        else
        {
            result[i] = POLLING_NONE;
        }
    }

    return 0;
}

/**
 *起動時に指定したプレイヤー数が接続するまで待つ
 *connected_sock_arrayにacceptされたソケットが保存される
 */
int ready(int sock, int connected_sock_array[], size_t len)
{
    int connected_num = 0;
    while (1)
    {
        int target_sock[1] = {sock};
        int result_sock[1] = {POLLING_NONE};
        if (polling(target_sock, 1, result_sock) == -1)
        {
            return -1;
        }

        if (result_sock[0] != POLLING_NONE)
        {
            struct sockaddr_storage client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            socklen_t address_size = sizeof(client_addr);
            connected_num++;
            connected_sock_array[connected_num - 1] = accept(result_sock[0], (struct sockaddr *)&client_addr, &address_size);
            printf("fd = %d : accepted\n", connected_sock_array[connected_num - 1]);
        }

        // 接続数が最大プレイヤー数まで達していたら参加受付完了コマンドを送信する
        if (connected_num == len)
        {
            // プレイヤーIDを決定してグローバル変数に保存しておく
            for (size_t i = 0; i < connected_num; i++)
            {
                player_info pi = {0};
                pi.player_id = i + 1;
                pi.player_sock = connected_sock_array[i];
                player_infos[i] = pi;
            }

            payload ready_end_cmd = {0};
            ready_end_cmd.command = (char)READY;
            write_other_players(connected_sock_array, connected_num, &ready_end_cmd, sizeof(ready_end_cmd), WRITE_ALL_PLAYER);
            return 0;
        }
    }
}

void move(int player_sockets[], size_t len, payload *data, size_t size, int self)
{
    // TODO: readをrecv(MSG_PEEK)にしてsize==READ_BUF_SIZEでなかったらreadしないようにした方がいい？
    // 受信したデータをそのまま他のプレイヤーに送信する。
    write_other_players(player_sockets, len, data, size, self);
}

/**
 * 接続順にプレイヤーIDを割り振って通知する
 */
void who_am_i(int player_sock, size_t len)
{
    payload who_am_i_cmd = {0};
    who_am_i_cmd.command = (char)WHO_AM_I;
    for (size_t i = 0; i < len; i++)
    {
        if (player_infos[i].player_sock == player_sock)
        {
            who_am_i_cmd.player_id = player_infos[i].player_id;
            break;
        }
    }
    int send_player[1] = {player_sock};

    write_other_players(send_player, 1, &who_am_i_cmd, sizeof(who_am_i_cmd), WRITE_ALL_PLAYER);
}

void rpc(int connected_sock_array[], size_t len, payload *data, size_t size, int self)
{
    switch (data->command)
    {
    case NOT_CMD:
        break;
    case MOVE:
        move(connected_sock_array, len, data, size, self);
        break;
    case WHO_AM_I:
        who_am_i(self, len);
        break;
    default:
        break;
    }
}

/**
 * 参加プレイヤーからデータを受け取り他の全プレイヤーに送信する
**/
int deliver_data_all_player(int connected_sock_array[], size_t len)
{
    // 各プレイヤーのデータを送受信する
    while (1)
    {
        int readable_fds[len];
        if (polling(connected_sock_array, len, readable_fds) == -1)
        {
            return -1;
        }

        for (size_t i = 0; i < len; i++)
        {
            if (readable_fds[i] == POLLING_NONE)
            {
                continue;
            }

            char buf[READ_BUF_SIZE];
            // クライアントをどれかひとつ切断するとselectがすぐ返るようになるがソケットにはデータが何もない現象が起きるので一人でも切断されたらcloseして終了する
            size_t size = read(readable_fds[i], buf, READ_BUF_SIZE);
            if (size == 0 || size == -1)
            {
                return -1;
            }
            payload *data = (payload *)buf;
            rpc(connected_sock_array, len, data, size, readable_fds[i]);
        }
    }
}

int create_tcp_socket()
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket error\n");
        return -1;
    }
    printf("fd = %d \n", sock);

    // Address already in use対策
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(int));

    return sock;
}

void close_sock_all(int connected_sock_array[], size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        printf("%d \n", connected_sock_array[i]);
        close(connected_sock_array[i]);
        printf("close\n");
    }
}

int main(int argc, char **argv)
{
    // 参加プレイヤー数
    int player_num = atoi(argv[1]);
    if (player_num <= 1)
    {
        perror("player number error\n");
        return 0;
    }

    // 設定
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    int sock = create_tcp_socket();

    socklen_t addr_len = sizeof(addr);
    if (bind(sock, (struct sockaddr *)&addr, addr_len))
    {
        perror("bind error\n");
        return 1;
    }

    if (listen(sock, player_num) == -1)
    {
        perror("listen error\n");
        return -1;
    }

    player_infos = malloc(sizeof(player_info) * player_num);

    // 接続済みソケット配列
    int connected_sock_array[player_num];
    int ready_result = 0;
    ready_result = ready(sock, connected_sock_array, sizeof(connected_sock_array) / sizeof(connected_sock_array[0]));
    if (ready_result == -1)
    {
        perror("ready result error\n");
        return -1;
    }

    close(sock);

    // 誰かが切断するまで送受信をし続ける
    deliver_data_all_player(connected_sock_array, sizeof(connected_sock_array) / sizeof(connected_sock_array[0]));
    free(player_infos);
    close_sock_all(connected_sock_array, sizeof(connected_sock_array) / sizeof(connected_sock_array[0]));

    return 0;
}

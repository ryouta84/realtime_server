#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "realtime_server.h"

int is_little_endian()
{
    int x = 1;
    if (*(char *)&x)
    {
        return 1;
    }
    return 0;
}

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
    memcpy(swap_temp, &value, sizeof(value));
    memcpy(temp32, swap_temp + 3, sizeof(char));
    memcpy(temp32 + 1, swap_temp + 2, sizeof(char));
    memcpy(temp32 + 2, swap_temp + 1, sizeof(char));
    memcpy(temp32 + 3, swap_temp, sizeof(char));
    memcpy(&ret_value, temp32, sizeof(temp32));

    return ret_value;
}

int main(int argc, char **argv)
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket error\n");
        return -1;
    }
    printf("fd = %d \n", sock);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(55550);
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("connect error\n");
        close(sock);
        return -1;
    }

    sleep(1);

    for (size_t i = 0; i < 10; i++)
    {
        payload buf = {
            2,
            MOVE,
            12,
            0,
            htonl(100),
            reverse_float(0.1 + i), // [-10, 10]
            reverse_float(0.1 + i), // [-10, 10]
        };
        write(sock, &buf, sizeof(payload));
        usleep(1000000 * 1.1);
    }

    sleep(1);
    close(sock);

    return 0;
}

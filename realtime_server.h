// ポート番号
#define PORT 55550

#define POLLING_NONE -1

typedef struct
{
    u_char player_id;
    u_char command;
    u_char command_target;
    u_char padding;
    int hp;
    float x;
    float y;
} payload;

typedef struct
{
    int player_sock;
    u_char player_id;
} player_info;

enum cmd
{
    NOT_CMD,
    MOVE = 1,
    DEATH,
    READY = 90,
    WHO_AM_I,
    CLOSE,
};

// read()する時のバッファサイズ
#define READ_BUF_SIZE sizeof(payload)

#define WRITE_ALL_PLAYER 0

# リアルタイムサーバー

## 概要

固定長のデータを送受信するサーバーです。  
TCPを使用しています。  
55550番ポートを使用。  

## 機能

* 指定クライアント数が揃うまで待ち受け
* プレイヤーIDの割り振り
* クライアントからの受け取ったデータをその他のクライアントに送信

## データ詳細

16byte

```C
    u_char player_id; // プレイヤーID
    u_char command;   // コマンド種別
    u_char command_target; // コマンドの対象
    u_char padding; // 構造体のパディング
    int hp;
    float x; // x座標
    float y; // y座標(クライアントではz座標)
```

## 使用方法

```bash
// コンパイル
gcc realtime_server.c -o realtime_server -DDEBUG
// 参加プレイヤーが二人の場合
./realtime_server 2
```

以下クライアント操作

1. rs_clientを起動する
1. サーバーのIPアドレスを入力してConnectボタンを押下
1. 全プレイヤーが参加するまでまつ(デフォルトでは２プレイヤー)
1. 画面には変化はないが全プレイヤーが接続したら画面クリックで移動可能になる。

# リアルタイムサーバー

## 概要

固定長のデータを送受信するサーバーです。
TCPを使用しています。

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
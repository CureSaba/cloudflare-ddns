# DDNS Updater for Cloudflare

CloudflareのDNSレコードを自動的に更新するDDNS（Dynamic DNS）クライアントです。
現在のグローバルIPアドレスを取得し、変更があった場合にCloudflare APIを通じてAレコードを更新します。

## 機能

- 現在のグローバルIPアドレスを自動取得
- IPアドレスに変更がない場合はAPIリクエストをスキップ
- 複数ドメインの同時管理
- ゾーン情報のローカルキャッシュによるAPI呼び出し削減
- 既存レコードの更新、または新規レコードの作成に対応

## 必要環境

- CMake 4.2 以上
- C++26 対応のコンパイラ（GCC 14 / Clang 18 以上推奨）
- インターネット接続（Cloudflare API アクセス用）

依存ライブラリはビルド時に自動的にダウンロードされます。

| ライブラリ | バージョン | 用途 |
|---|---|---|
| [cpr](https://github.com/libcpr/cpr) | 1.14.2 | HTTP リクエスト |
| [nlohmann/json](https://github.com/nlohmann/json) | v3.12.0 | JSON の読み書き |
| [curl](https://curl.se/) | (cpr 依存) | HTTP 通信基盤 |

## セットアップ

### 1. リポジトリのクローン

```bash
git clone <repository-url>
cd ddns
```

### 2. 設定ファイルの編集

`ddns.json` を編集し、CloudflareのAPIトークンと対象ドメインを設定します。

```json
{
  "api_token": "your_cloudflare_api_token_here",
  "domains": ["home.example.com"],
  "ip": ""
}
```

| フィールド | 説明 |
|---|---|
| `api_token` | Cloudflare API トークン（`Zone:DNS:Edit` 権限が必要） |
| `domains` | 更新対象のドメイン名（配列で複数指定可能） |
| `ip` | 最後に更新したIPアドレス（自動管理、手動変更不要） |

#### Cloudflare APIトークンの取得方法

1. [Cloudflareダッシュボード](https://dash.cloudflare.com/) にログイン
2. 右上のプロフィールアイコン → **My Profile**
3. **API Tokens** タブ → **Create Token**
4. **Edit zone DNS** テンプレートを使用し、対象ゾーンを指定して作成

### 3. ビルド

```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
```

### 4. 実行

実行ファイルは `ddns.json` と同じディレクトリに配置してください。

```bash
./cmake-build-debug/ddns
```

## 自動実行の設定（定期実行）

### Linux / macOS (cron)

プログラムは実行時のカレントディレクトリから `ddns.json` を読み書きするため、
必ず `cd` でプロジェクトディレクトリに移動してから実行してください。

```cron
*/5 * * * * cd /path/to/ddns && ./ddns
```

`crontab -e` で上記を追加してください。`/path/to/ddns` はプロジェクトの実際のパスに置き換えてください。

### Windows (タスクスケジューラ)

タスクスケジューラで5分ごとに実行するタスクを作成してください。

## 動作の仕組み

```
1. ddns.json を読み込む
2. 現在のグローバルIPアドレスを取得（https://api.fixitlater.org/ip/）
3. 前回保存したIPと比較 → 変化なければ終了
4. 各ドメインについて:
   a. キャッシュからゾーンIDを検索
   b. キャッシュに無ければ Cloudflare API からゾーン一覧を取得
   c. 既存のAレコードがあれば更新、なければ新規作成
5. 成功したら ddns.json に現在のIPを保存
```

## ライセンス

このプロジェクトは [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html) のもとで公開されています。

```
DDNS Updater for Cloudflare
Copyright (C) 2026

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
```


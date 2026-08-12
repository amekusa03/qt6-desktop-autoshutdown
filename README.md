# AutoShutdown

無操作（アイドル）状態を検出して自動的にシステムをシャットダウンする Linux 向けの Qt アプリケーションです。
システムのバックグラウンドに常駐し、無駄な電力消費を防ぐために自動で電源をオフにします。

---

## 🚀 主な機能

* **バックグラウンド常駐**: ウィンドウを閉じても終了せず、システムトレイ（タスクトレイ）に常駐して監視を継続します。
* **アイドル監視**:
  * GNOME環境（MutterデスクトップのD-Busインターフェース）のアイドル時間取得に対応。
  * X11環境（`xprintidle`）のアイドル時間取得に対応。
* **パラメータカスタマイズ**:
  * 無操作タイムアウト時間（60秒〜3600秒）の設定。
  * アイドル状態のチェック間隔（5秒〜60秒）の設定。
  * 設定した値は自動的に `config.ini` に保存されます。
* **安全なシャットダウン処理**:
  * タイムアウトに達すると `sudo shutdown -h +1` が実行され、1分間の猶予期間が設けられます。
  * 猶予期間内であれば、UIからいつでもシャットダウンをキャンセルできます。
* **通知機能**: `notify-send` を通じて、監視開始、シャットダウン開始、シャットダウンキャンセルの通知を受け取れます。

---

## 🛠 依存関係

動作およびビルドには以下のパッケージが必要です。

### 1. 必須パッケージ (Ubuntu/Debian系の例)
```bash
sudo apt install build-essential qtcreator qtbase5-dev qtchooser qt5-qmake
```
※ Qt6環境でビルドする場合は、`qt6-base-dev` 等を適宜インストールしてください。

### 2. アイドル検出のための依存関係
* **GNOME環境**: 追加設定なしで動作します。
* **X11環境（GNOME以外）**: `xprintidle` コマンドが必要です。
  ```bash
  sudo apt install xprintidle
  ```

### 3. 通知用パッケージ
* `notify-send` コマンドを使用するため、通常は標準でインストールされていますが、必要に応じて以下を導入してください。
  ```bash
  sudo apt install libnotify-bin
  ```

---

## 🔑 シャットダウン権限の設定 (推奨)

本アプリは `sudo shutdown` コマンドを使用してシャットダウンやキャンセルを実行します。パスワード入力なしでこれを実行できるように、`sudoers` の設定を推奨します。

1. 以下のコマンドで設定ファイルを開きます：
   ```bash
   sudo visudo
   ```
2. ファイルの末尾に、ご自身のユーザー名（例: `fedora`）にあわせて以下の行を追加します：
   ```text
   fedora ALL=(ALL) NOPASSWD: /usr/sbin/shutdown
   ```
   *※ パスは環境に合わせて調整してください (`which shutdown` で確認できます)。通常は `/usr/sbin/shutdown` もしくは `/sbin/shutdown` です。*

---

## 📦 ビルドおよび実行方法

### Qt Creator からビルドする
1. Qt Creator を起動し、**「ファイル」** -> **「ファイルやプロジェクトを開く...」** から [AutoShutdown.pro](AutoShutdown.pro) を選択します。
2. 使用するキットを選択して「プロジェクトの構成」を行います。
3. ビルドして実行します。

### コマンドラインからビルドする
```bash
# プロジェクトディレクトリに移動
cd /home/kusa/ドキュメント/Qt/AutoShutdown

# Makefileの生成 (qmake)
qmake AutoShutdown.pro

# ビルドの実行
make
```
ビルド完了後、実行ファイル `./AutoShutdown` が生成されます。

---

## ⚙️ 常駐（自動起動）とインストールの設定

本アプリケーションをシステムの起動（ログイン）時に自動で実行し、バックグラウンドに常駐させるには、以下の手順でインストールと自動起動（Autostart）設定を行います。

### 1. 実行ファイルのインストール（配置）

ビルドして生成された実行ファイル `AutoShutdown` を、パスの通ったディレクトリ（例: `/usr/local/bin` または個人の `~/.local/bin`）にコピーします。

```bash
# システム全体にインストールする場合（推奨）
sudo cp AutoShutdown /usr/local/bin/

# または、現在のユーザーのみのディレクトリにインストールする場合
mkdir -p ~/.local/bin
cp AutoShutdown ~/.local/bin/
```

### 2. 自動起動（スタートアップ）への登録

デスクトップ環境（GNOME等）の起動時にアプリを自動起動させるため、`~/.config/autostart/` ディレクトリにデスクトップエントリファイルを作成します。

```bash
# 自動起動設定ディレクトリを作成（存在しない場合）
mkdir -p ~/.config/autostart

# 設定ファイルの作成・編集
nano ~/.config/autostart/autoshutdown.desktop
```

ファイルの内容として以下を入力して保存します。

**`/usr/local/bin/` に配置した場合：**
```ini
[Desktop Entry]
Type=Application
Name=AutoShutdown
Comment=Idle detection and auto shutdown service
Exec=/usr/local/bin/AutoShutdown
Icon=system-shutdown
Terminal=false
Categories=Utility;
X-GNOME-Autostart-enabled=true
```

**`~/.local/bin/` に配置した場合：**
（※ `Exec` のパスをご自身のホームディレクトリに合わせて変更してください）
```ini
[Desktop Entry]
Type=Application
Name=AutoShutdown
Comment=Idle detection and auto shutdown service
Exec=/home/ご自身のユーザー名/.local/bin/AutoShutdown
Icon=system-shutdown
Terminal=false
Categories=Utility;
X-GNOME-Autostart-enabled=true
```

これで、次回ログイン時から自動的にアプリケーションが起動し、システムトレイ（タスクトレイ）に常駐してアイドル監視を開始します。

---

## 📄 ライセンス

このプロジェクトは [MIT ライセンス](LICENSE) のもとで公開されています。

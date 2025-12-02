# SMTP邮件发送工具

这是一个支持发送多附件、多收件人邮件的工具，包含Shell脚本（Linux/macOS）和C++程序（Windows）。

## 功能特点

- 支持发送多个附件
- 支持多个收件人、抄送和密送
- 通过配置文件管理SMTP服务器信息
- 支持SSL/TLS加密传输（自动根据端口选择合适的加密方式）
- 提供命令行参数控制收件人、附件、主题和正文
- 使用curl直接连接SMTP服务器，无需额外配置本地邮件服务
- **Windows版本支持自动创建配置文件模板、中文路径和日志记录**

## 配置文件

所有工具共用同一个配置文件 `send_email.ini`。

### 配置项说明
配置文件包含以下配置项：
- `SMTP_SERVER`: SMTP服务器地址
- `SMTP_PORT`: SMTP服务器端口
- `SMTP_ENCRYPTION`: SMTP加密方式（可选值：none、ssl、tls）
  - none: 不使用加密
  - ssl: 使用SMTPS加密（通常使用端口465）
  - tls: 使用STARTTLS加密（通常使用端口587）
  - 注意：如果未指定，将根据端口自动选择加密方式
- `SMTP_USER`: SMTP认证用户名
- `SMTP_PASS`: SMTP认证密码
- `SENDER`: 发件人地址
- `SENDER_NAME`: 发件人名称（可选，设置后邮件显示为：发件人名称<发件人地址>）

### 默认邮件配置
配置文件还包含以下默认邮件配置项（可在命令行参数中被覆盖）：
- `DEFAULT_TO_EMAILS`: 默认收件人（多个收件人用逗号分隔）
- `DEFAULT_CC_EMAILS`: 默认抄送（多个收件人用逗号分隔）
- `DEFAULT_BCC_EMAILS`: 默认密送（多个收件人用逗号分隔）
- `DEFAULT_SUBJECT`: 默认邮件主题
- `DEFAULT_BODY`: 默认邮件正文
- `DEFAULT_ATTACHMENTS`: 默认附件（多个附件用逗号分隔）

## Windows 版本 (C++)

### 使用方法

1. 首次运行程序（不带参数）会自动在程序目录下生成 `send_email.ini` 配置文件模板。
2. 编辑 `send_email.ini`，填入您的SMTP服务器信息。
3. 再次运行程序发送邮件。

### 命令行示例

```cmd
send_email.exe -t "user1@example.com,user2@example.com" -f "D:\data\file1.pdf,D:\data\file2.txt" -s "重要通知" -m "请查看附件的内容。"
```

### 参数说明
- `-t`: 收件人地址（多个收件人用逗号分隔，必填）
- `-c`: 抄送地址（多个抄送地址用逗号分隔，可选）
- `-b`: 密送地址（多个密送地址用逗号分隔，可选）
- `-s`: 邮件主题（可选）
- `-m`: 邮件正文（可选）
- `-f`: 附件路径（多个附件用逗号分隔，可选）
- `-h`: 显示帮助信息

### 日志
程序会在所在目录下生成 `send_email.log`，记录发送时间、收件人、主题、附件列表和状态。

## Linux/macOS 版本 (Shell)

### 依赖要求
脚本依赖于 `curl` 工具。

### 使用方法

1. 确保脚本有执行权限：
   ```bash
   chmod +x send_email.sh
   ```
2. 创建并编辑配置文件 `send_email.ini`。
3. 执行脚本发送邮件：
   ```bash
   ./send_email.sh -t "user1@example.com" -f "/path/to/file.txt" -s "邮件主题" -b "邮件正文"
   ```

### 参数说明
- `-t`: 收件人地址（多个收件人用逗号分隔，必填）
- `-c`: 抄送地址（多个抄送地址用逗号分隔，可选）
- `-b`: 密送地址（多个密送地址用逗号分隔，可选）
- `-s`: 邮件主题（可选）
- `-b`: 邮件正文（可选，注意Shell脚本中使用的是-b参数，Windows版使用-m）
- `-f`: 附件路径（多个附件用逗号分隔，可选）
- `-h`: 显示帮助信息

## 作者
https://github.com/git-howard/

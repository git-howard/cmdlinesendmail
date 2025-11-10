# SMTP邮件发送脚本

这是一个支持发送多附件、多收件人邮件的Shell脚本，使用SMTP协议直接发送邮件。

## 功能特点

- 支持发送多个附件
- 支持多个收件人、抄送和密送
- 通过配置文件管理SMTP服务器信息
- 支持SSL/TLS加密传输（自动根据端口选择合适的加密方式）
- 提供命令行参数控制收件人、附件、主题和正文
- 使用curl直接连接SMTP服务器，无需额外配置本地邮件服务

## 依赖要求

脚本依赖于`curl`工具，大多数Linux/Unix系统默认已安装。如果没有，请安装：

### Ubuntu/Debian系统
```bash
sudo apt-get update
sudo apt-get install curl
```

### CentOS/RHEL系统
```bash
sudo yum install curl
```

### macOS系统
```bash
# macOS通常已预装curl，如果没有则运行
brew install curl
```

## 使用方法

1. 确保脚本有执行权限：
   ```bash
   chmod +x send_email.sh
   ```

2. 编辑配置文件 `email_config.conf`，设置您的SMTP服务器信息。

3. 执行脚本发送邮件：
   ```bash
   ./send_email.sh -t "收件人1,收件人2" -f "附件1,附件2" -s "邮件主题" -b "邮件正文"
   ```

## 命令行参数说明

- `-t`: 收件人地址（多个收件人用逗号分隔，必填）
- `-c`: 抄送地址（多个抄送地址用逗号分隔，可选）
- `-b`: 密送地址（多个密送地址用逗号分隔，可选）
- `-s`: 邮件主题（可选，默认为"测试邮件"）
- `-f`: 附件路径（多个附件用逗号分隔，可选）
- `-h`: 显示帮助信息

## 示例

发送带两个附件给两个收件人的邮件：
```bash
./send_email.sh -t "user1@example.com,user2@example.com" -f "/path/to/file1.pdf,/path/to/file2.txt" -s "重要通知" -b "请查看附件的内容。"
```

发送抄送邮件：
```bash
./send_email.sh -t "user1@example.com" -c "cc@example.com" -s "抄送测试" -b "这是一封抄送测试邮件。"
```

## 注意事项

1. 请确保SMTP服务器配置正确，特别是端口和认证信息。
2. 附件路径应该是绝对路径或相对于当前工作目录的路径。
3. 对于密码中包含特殊字符的情况，请确保在配置文件中正确转义。
4. 加密方式优先级：
   - 如果在配置文件中明确指定了`SMTP_ENCRYPTION`，脚本将优先使用该配置
   - 如果未指定`SMTP_ENCRYPTION`，脚本会根据端口自动选择加密方式：
     - 端口465：使用SMTPS（SSL）加密连接
     - 端口587：使用SMTP + STARTTLS加密连接
     - 其他端口：使用普通SMTP连接
5. 确保网络可以访问SMTP服务器，特别是在有防火墙的环境中。
6. 如遇到认证失败，请检查用户名和密码是否正确，某些邮箱服务可能需要使用应用专用密码而非登录密码。

## 配置文件说明

配置文件 `email_config.conf` 包含以下配置项：
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
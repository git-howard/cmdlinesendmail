#!/bin/bash

# 邮件发送脚本 - 支持多收件人和多附件
# 使用方法: ./send_email.sh -t "收件人1,收件人2" -f "附件1,附件2" -s "邮件主题" -b "邮件正文"

# 默认配置文件路径
CONFIG_FILE="./email_config.conf"

# 加载配置文件
if [ ! -f "$CONFIG_FILE" ]; then
  echo "错误: 配置文件 $CONFIG_FILE 不存在"
  echo "请先创建配置文件，可参考email_config.example"
  exit 1
fi

# 从配置文件读取配置
source "$CONFIG_FILE"

# 从配置文件读取默认值
# 注意：这些默认值将在命令行参数解析时被覆盖
TO_EMAILS="$DEFAULT_TO_EMAILS"
CC_EMAILS="$DEFAULT_CC_EMAILS"
BCC_EMAILS="$DEFAULT_BCC_EMAILS"
SUBJECT="$DEFAULT_SUBJECT"
BODY="$DEFAULT_BODY"
ATTACHMENTS="$DEFAULT_ATTACHMENTS"



# 解析命令行参数
while getopts "t:c:b:s:f:c:h" opt; do
  case $opt in
    t) TO_EMAILS="$OPTARG" ;;
    c) CC_EMAILS="$OPTARG" ;;
    b) BCC_EMAILS="$OPTARG" ;;
    s) SUBJECT="$OPTARG" ;;
    f) ATTACHMENTS="$OPTARG" ;;
    h) echo "用法: $0 -t "收件人1,收件人2" -f "附件1,附件2" -s "主题" -b "正文" -c "抄送" -b "密送"" && exit 0 ;;
    *) echo "未知选项，请使用 -h 查看帮助" && exit 1 ;;
  esac
done

# 检查必要的参数
if [ -z "$TO_EMAILS" ]; then
  echo "错误: 必须指定收件人地址 (-t)"
  exit 1
fi

# 检查必要的配置项
if [ -z "$SMTP_SERVER" ] || [ -z "$SMTP_PORT" ] || [ -z "$SMTP_USER" ] || [ -z "$SMTP_PASS" ] || [ -z "$SENDER" ]; then
  echo "错误: 配置文件缺少必要的配置项"
  exit 1
fi

# 设置默认加密方式（如果未配置）
if [ -z "$SMTP_ENCRYPTION" ]; then
  # 根据端口自动选择加密方式
  if [ "$SMTP_PORT" -eq 465 ]; then
    SMTP_ENCRYPTION="ssl"
  elif [ "$SMTP_PORT" -eq 587 ]; then
    SMTP_ENCRYPTION="tls"
  else
    SMTP_ENCRYPTION="none"
  fi
  echo "未指定SMTP_ENCRYPTION，根据端口$SMTP_PORT自动选择为$SMTP_ENCRYPTION"
fi

# 创建临时文件
TMP_DIR=$(mktemp -d)
TMP_FILE="$TMP_DIR/email.txt"
ATTACH_DIR="$TMP_DIR/attachments"
mkdir -p "$ATTACH_DIR"

# 生成邮件头部
# 如果设置了SENDER_NAME，则使用格式：发件人名称<发件人地址>
if [ ! -z "$SENDER_NAME" ]; then
  echo "From: $SENDER_NAME <$SENDER>" > "$TMP_FILE"
else
  echo "From: $SENDER" > "$TMP_FILE"
fi
echo "To: $TO_EMAILS" >> "$TMP_FILE"

if [ ! -z "$CC_EMAILS" ]; then
  echo "Cc: $CC_EMAILS" >> "$TMP_FILE"
fi

if [ ! -z "$BCC_EMAILS" ]; then
  echo "Bcc: $BCC_EMAILS" >> "$TMP_FILE"
fi

echo "Subject: $SUBJECT" >> "$TMP_FILE"
echo "MIME-Version: 1.0" >> "$TMP_FILE"

# 检查是否有附件
if [ ! -z "$ATTACHMENTS" ]; then
  # 生成边界
  BOUNDARY="====$(date +%s)===="
  echo "Content-Type: multipart/mixed; boundary="$BOUNDARY"" >> "$TMP_FILE"
  echo "" >> "$TMP_FILE"
  echo "--$BOUNDARY" >> "$TMP_FILE"
  echo "Content-Type: text/plain; charset=utf-8" >> "$TMP_FILE"
  echo "Content-Transfer-Encoding: 8bit" >> "$TMP_FILE"
  echo "" >> "$TMP_FILE"
  echo "$BODY" >> "$TMP_FILE"
  echo "" >> "$TMP_FILE"
  
  # 处理每个附件
  IFS="," read -ra ATTACH <<< "$ATTACHMENTS"
  for file in "${ATTACH[@]}"; do
    # 去除文件名前后空格
    file=$(echo "$file" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')
    
    if [ -f "$file" ]; then
      # 获取文件名（不含路径）
      filename=$(basename "$file")
      # 复制文件到临时目录
      cp "$file" "$ATTACH_DIR/$filename"
      
      echo "--$BOUNDARY" >> "$TMP_FILE"
      echo "Content-Type: application/octet-stream; name="$filename"" >> "$TMP_FILE"
      echo "Content-Transfer-Encoding: base64" >> "$TMP_FILE"
      echo "Content-Disposition: attachment; filename="$filename"" >> "$TMP_FILE"
      echo "" >> "$TMP_FILE"
      
      # 将文件内容转换为base64并添加到邮件
      base64 "$ATTACH_DIR/$filename" >> "$TMP_FILE"
      echo "" >> "$TMP_FILE"
    else
      echo "警告: 附件文件 $file 不存在，将被跳过"
    fi
  done
  
  # 添加边界结束标记
  echo "--$BOUNDARY--" >> "$TMP_FILE"
else
  # 无附件，纯文本邮件
  echo "Content-Type: text/plain; charset=utf-8" >> "$TMP_FILE"
  echo "Content-Transfer-Encoding: 8bit" >> "$TMP_FILE"
  echo "" >> "$TMP_FILE"
  echo "$BODY" >> "$TMP_FILE"
fi

# 发送邮件
echo "正在发送邮件..."

# 使用curl通过SMTP发送邮件
# 根据配置的加密方式确定连接参数
echo "使用$SMTP_ENCRYPTION加密方式连接到$SMTP_SERVER:$SMTP_PORT"

case "$SMTP_ENCRYPTION" in
  ssl)
    # 使用SSL (SMTPS)
    curl -v --ssl-reqd \
      --url "smtps://$SMTP_SERVER:$SMTP_PORT" \
      --mail-from "$SENDER" \
      --mail-rcpt "$TO_EMAILS" \
      --user "$SMTP_USER:$SMTP_PASS" \
      --upload-file "$TMP_FILE"
    ;;
  tls)
    # 使用STARTTLS
    curl -v --ssl \
      --url "smtp://$SMTP_SERVER:$SMTP_PORT" \
      --mail-from "$SENDER" \
      --mail-rcpt "$TO_EMAILS" \
      --user "$SMTP_USER:$SMTP_PASS" \
      --upload-file "$TMP_FILE"
    ;;
  none)
    # 不使用加密
    curl -v \
      --url "smtp://$SMTP_SERVER:$SMTP_PORT" \
      --mail-from "$SENDER" \
      --mail-rcpt "$TO_EMAILS" \
      --user "$SMTP_USER:$SMTP_PASS" \
      --upload-file "$TMP_FILE"
    ;;
  *)
    echo "错误: 未知的SMTP加密方式 '$SMTP_ENCRYPTION'"
    echo "请在配置文件中设置正确的加密方式: none、ssl或tls"
    exit 1
    ;;
esac

# 检查发送状态
if [ $? -eq 0 ]; then
  echo "邮件发送成功"
else
  echo "邮件发送失败，请检查SMTP配置和网络连接"
fi

# 清理临时文件
rm -rf "$TMP_DIR"

echo "邮件发送完成"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <filesystem>
#include <random>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;

// --- Configuration and Globals ---

struct Config {
    std::string smtp_server;
    std::string smtp_port;
    std::string smtp_encryption; // none, ssl, tls
    std::string smtp_user;
    std::string smtp_pass;
    std::string sender;
    std::string sender_name;

    // Defaults
    std::string default_to_emails;
    std::string default_cc_emails;
    std::string default_bcc_emails;
    std::string default_subject;
    std::string default_body;
    std::string default_attachments;
};

struct Email {
    std::string to_emails;
    std::string cc_emails;
    std::string bcc_emails;
    std::string subject;
    std::string body;
    std::vector<std::string> attachments;
};

// --- Helper Functions ---

// Convert wstring to utf-8 string
std::string wstring_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Get executable directory
fs::path get_executable_dir() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    std::string full_path(result, (count > 0) ? count : 0);
    return fs::path(full_path).parent_path();
#endif
}

// Get current datetime string
std::string get_current_datetime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Write to log file
void write_log(const std::string& sender, const Email& email, bool success) {
    fs::path exe_dir = get_executable_dir();
    fs::path log_path = exe_dir / "send_email.log";
    
    std::ofstream log_file(log_path, std::ios::app);
    if (log_file.is_open()) {
        std::string attach_str;
        for (size_t i = 0; i < email.attachments.size(); ++i) {
            // Only log the filename, not full path, to keep it clean
            attach_str += fs::path(email.attachments[i]).filename().string();
            if (i < email.attachments.size() - 1) attach_str += ", ";
        }
        if (attach_str.empty()) attach_str = "None";

        log_file << "[" << get_current_datetime() << "] "
                 << "Sender: " << sender << ", "
                 << "To: [" << email.to_emails << "], "
                 << "Cc: [" << email.cc_emails << "], "
                 << "Bcc: [" << email.bcc_emails << "], "
                 << "Subject: \"" << email.subject << "\", "
                 << "Attachments: [" << attach_str << "], "
                 << "Status: " << (success ? "Success" : "Failed")
                 << std::endl;
    }
}

// Trim string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n\"");
    if (std::string::npos == first) {
        return str;
    }
    size_t last = str.find_last_not_of(" \t\r\n\"");
    return str.substr(first, (last - first + 1));
}

// Remove quotes only from ends if present
std::string remove_quotes(const std::string& str) {
    std::string s = str;
    // Simple trim of whitespace first
    size_t first = s.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    s = s.substr(first, (last - first + 1));

    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    if (s.size() >= 2 && s.front() == '\'' && s.back() == '\'') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// Base64 Encoder
const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}

// --- Logic ---

void create_template_config(const fs::path& filename) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "错误: 无法创建配置文件模板 " << filename.string() << std::endl;
        exit(1);
    }
    ofs << "# SMTP服务器配置\n"
        << "# 请根据实际情况修改以下配置\n\n"
        << "# SMTP服务器地址\n"
        << "SMTP_SERVER=smtp.example.com\n\n"
        << "# SMTP服务器端口\n"
        << "SMTP_PORT=465\n\n"
        << "# SMTP加密方式（可选值：none、ssl、tls）\n"
        << "# none: 不使用加密\n"
        << "# ssl: 使用SMTPS加密（通常使用端口465）\n"
        << "# tls: 使用STARTTLS加密（通常使用端口587）\n"
        << "# 注意：如果未指定，将根据端口自动选择加密方式\n"
        << "SMTP_ENCRYPTION=ssl\n\n"
        << "# SMTP认证用户名\n"
        << "SMTP_USER=user@example.com\n\n"
        << "# SMTP认证密码\n"
        << "SMTP_PASS=password\n\n"
        << "# 发件人地址\n"
        << "# 注意：SMTP服务器要求发件人地址必须与授权用户(SMTP_USER)相同\n"
        << "SENDER=\"user@example.com\"\n\n"
        << "# 发件人名称（可选）\n"
        << "# 如果设置，邮件显示为：发件人名称<发件人地址>\n"
        << "SENDER_NAME=\"Sender Name\"\n\n"
        << "# 默认邮件配置\n"
        << "# 以下配置项可以在命令行参数中被覆盖\n"
        << "# 默认收件人（多个收件人用逗号分隔）\n"
        << "DEFAULT_TO_EMAILS=\"\"\n"
        << "# 默认抄送（多个收件人用逗号分隔）\n"
        << "DEFAULT_CC_EMAILS=\"\"\n"
        << "# 默认密送（多个收件人用逗号分隔）\n"
        << "DEFAULT_BCC_EMAILS=\"\"\n"
        << "# 默认邮件主题\n"
        << "DEFAULT_SUBJECT=\"测试邮件\"\n"
        << "# 默认邮件正文\n"
        << "DEFAULT_BODY=\"这是一封测试邮件。\"\n"
        << "# 默认附件（多个附件用逗号分隔）\n"
        << "DEFAULT_ATTACHMENTS=\"\"\n";
    
    std::cout << "配置文件 " << filename.string() << " 不存在，已自动创建模板。\n"
              << "请修改配置后重试。" << std::endl;
}

Config load_config(const fs::path& filename) {
    Config config;
    
    if (!fs::exists(filename)) {
        create_template_config(filename);
        exit(0);
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误: 无法读取配置文件 " << filename.string() << std::endl;
        exit(1);
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remove comments
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = trim(line.substr(0, eq_pos));
            std::string value = remove_quotes(line.substr(eq_pos + 1));

            if (key == "SMTP_SERVER") config.smtp_server = value;
            else if (key == "SMTP_PORT") config.smtp_port = value;
            else if (key == "SMTP_ENCRYPTION") config.smtp_encryption = value;
            else if (key == "SMTP_USER") config.smtp_user = value;
            else if (key == "SMTP_PASS") config.smtp_pass = value;
            else if (key == "SENDER") config.sender = value;
            else if (key == "SENDER_NAME") config.sender_name = value;
            else if (key == "DEFAULT_TO_EMAILS") config.default_to_emails = value;
            else if (key == "DEFAULT_CC_EMAILS") config.default_cc_emails = value;
            else if (key == "DEFAULT_BCC_EMAILS") config.default_bcc_emails = value;
            else if (key == "DEFAULT_SUBJECT") config.default_subject = value;
            else if (key == "DEFAULT_BODY") config.default_body = value;
            else if (key == "DEFAULT_ATTACHMENTS") config.default_attachments = value;
        }
    }
    return config;
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        std::string trimmed = trim(token);
        if (!trimmed.empty()) {
            tokens.push_back(trimmed);
        }
    }
    return tokens;
}

void show_help(const char* program_name) {
    std::cout << "用法: " << program_name << " -t \"收件人1,收件人2\" -f \"附件1,附件2\" -s \"主题\" -m \"正文\" -c \"抄送\"" << std::endl;
    std::cout << "\n参数说明:\n";
    std::cout << "  -t  收件人地址（多个收件人用逗号分隔，必填）\n";
    std::cout << "  -c  抄送地址（多个抄送地址用逗号分隔，可选）\n";
    std::cout << "  -b  密送地址（多个密送地址用逗号分隔，可选）\n";
    std::cout << "  -s  邮件主题（可选）\n";
    std::cout << "  -m  邮件正文（可选）\n";
    std::cout << "  -f  附件路径（多个附件用逗号分隔，可选）\n";
    std::cout << "  -h  显示帮助信息\n";
}

int main(int argc, char* argv[]) {
    // Set code page to UTF-8 for console output
    system("chcp 65001 > nul");

    std::vector<std::string> args;
#ifdef _WIN32
    int argc_w = 0;
    LPWSTR* argv_w = CommandLineToArgvW(GetCommandLineW(), &argc_w);
    if (argv_w == NULL) {
        std::cerr << "Error parsing command line arguments" << std::endl;
        return 1;
    }
    for (int i = 0; i < argc_w; ++i) {
        args.push_back(wstring_to_utf8(argv_w[i]));
    }
    LocalFree(argv_w);
#else
    for (int i = 0; i < argc; ++i) {
        args.push_back(argv[i]);
    }
#endif

    if (args.size() <= 1) {
        show_help(args[0].c_str());
        return 0;
    }

    // Determine config file path based on executable location
    fs::path exe_dir = get_executable_dir();
    fs::path config_path = exe_dir / "email_config.conf";

    Config config = load_config(config_path);

    Email email;
    email.to_emails = config.default_to_emails;
    email.cc_emails = config.default_cc_emails;
    email.bcc_emails = config.default_bcc_emails;
    email.subject = config.default_subject;
    email.body = config.default_body;
    if (!config.default_attachments.empty()) {
        email.attachments = split(config.default_attachments, ',');
    }

    // Argument parsing
    for (size_t i = 1; i < args.size(); ++i) {
        std::string arg = args[i];
        if (arg == "-h" || arg == "--help") {
            show_help(args[0].c_str());
            return 0;
        }
        if (i + 1 < args.size()) {
            if (arg == "-t") {
                email.to_emails = args[++i];
            } else if (arg == "-c") {
                email.cc_emails = args[++i];
            } else if (arg == "-b") {
                email.bcc_emails = args[++i];
            } else if (arg == "-s") {
                email.subject = args[++i];
            } else if (arg == "-f") {
                std::string attach_str = args[++i];
                email.attachments = split(attach_str, ',');
            } else if (arg == "-m") {
                email.body = args[++i];
            }
        }
    }

    // Validate inputs
    if (email.to_emails.empty()) {
        std::cerr << "错误: 必须指定收件人地址 (-t)" << std::endl;
        return 1;
    }
    if (config.smtp_server.empty() || config.smtp_port.empty() || config.smtp_user.empty() || config.smtp_pass.empty() || config.sender.empty()) {
        std::cerr << "错误: 配置文件缺少必要的配置项 (SMTP_SERVER, SMTP_PORT, SMTP_USER, SMTP_PASS, SENDER)" << std::endl;
        return 1;
    }

    // Auto-detect encryption
    if (config.smtp_encryption.empty()) {
        int port = std::stoi(config.smtp_port);
        if (port == 465) config.smtp_encryption = "ssl";
        else if (port == 587) config.smtp_encryption = "tls";
        else config.smtp_encryption = "none";
        std::cout << "未指定SMTP_ENCRYPTION，根据端口" << config.smtp_port << "自动选择为" << config.smtp_encryption << std::endl;
    }

    // Create temp file
    std::string tmp_dir = fs::temp_directory_path().string();
    std::string tmp_file = tmp_dir + "/email_" + std::to_string(std::time(nullptr)) + ".txt";
    
    std::ofstream ofs(tmp_file, std::ios::binary);
    if (!ofs) {
        std::cerr << "错误: 无法创建临时文件 " << tmp_file << std::endl;
        return 1;
    }

    // Write Headers
    if (!config.sender_name.empty()) {
        ofs << "From: " << config.sender_name << " <" << config.sender << ">\r\n";
    } else {
        ofs << "From: " << config.sender << "\r\n";
    }
    ofs << "To: " << email.to_emails << "\r\n";
    if (!email.cc_emails.empty()) ofs << "Cc: " << email.cc_emails << "\r\n";
    if (!email.bcc_emails.empty()) ofs << "Bcc: " << email.bcc_emails << "\r\n";
    ofs << "Subject: " << email.subject << "\r\n";
    ofs << "MIME-Version: 1.0\r\n";

    std::string boundary = "====" + std::to_string(std::time(nullptr)) + "====";

    if (!email.attachments.empty()) {
        ofs << "Content-Type: multipart/mixed; boundary=\"" << boundary << "\"\r\n\r\n";
        ofs << "--" << boundary << "\r\n";
        ofs << "Content-Type: text/plain; charset=utf-8\r\n";
        ofs << "Content-Transfer-Encoding: 8bit\r\n\r\n";
        ofs << email.body << "\r\n\r\n";

        for (const auto& filepath : email.attachments) {
            if (!fs::exists(filepath)) {
                std::cerr << "警告: 附件文件 " << filepath << " 不存在，将被跳过" << std::endl;
                continue;
            }

            std::string filename = fs::path(filepath).filename().string();
            
            ofs << "--" << boundary << "\r\n";
            ofs << "Content-Type: application/octet-stream; name=\"" << filename << "\"\r\n";
            ofs << "Content-Transfer-Encoding: base64\r\n";
            ofs << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n\r\n";

            // Read file and encode base64
            std::ifstream ifs(filepath, std::ios::binary);
            std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(ifs), {});
            std::string encoded = base64_encode(buffer.data(), buffer.size());
            
            for (size_t i = 0; i < encoded.length(); i += 76) {
                ofs << encoded.substr(i, 76) << "\r\n";
            }
            ofs << "\r\n";
        }
        ofs << "--" << boundary << "--\r\n";
    } else {
        ofs << "Content-Type: text/plain; charset=utf-8\r\n";
        ofs << "Content-Transfer-Encoding: 8bit\r\n\r\n";
        ofs << email.body << "\r\n";
    }
    ofs.close();

    // Send Email using curl
    std::cout << "正在发送邮件..." << std::endl;
    std::cout << "使用" << config.smtp_encryption << "加密方式连接到" << config.smtp_server << ":" << config.smtp_port << std::endl;

    std::string curl_cmd = "curl.exe -v";
    if (config.smtp_encryption == "ssl") {
        curl_cmd += " --ssl-reqd --url \"smtps://" + config.smtp_server + ":" + config.smtp_port + "\"";
    } else if (config.smtp_encryption == "tls") {
        curl_cmd += " --ssl --url \"smtp://" + config.smtp_server + ":" + config.smtp_port + "\"";
    } else {
        curl_cmd += " --url \"smtp://" + config.smtp_server + ":" + config.smtp_port + "\"";
    }

    curl_cmd += " --mail-from \"" + config.sender + "\"";
    
    std::vector<std::string> all_rcpts = split(email.to_emails, ',');
    std::vector<std::string> ccs = split(email.cc_emails, ',');
    std::vector<std::string> bccs = split(email.bcc_emails, ',');
    all_rcpts.insert(all_rcpts.end(), ccs.begin(), ccs.end());
    all_rcpts.insert(all_rcpts.end(), bccs.begin(), bccs.end());

    for (const auto& rcpt : all_rcpts) {
        curl_cmd += " --mail-rcpt \"" + rcpt + "\"";
    }

    curl_cmd += " --user \"" + config.smtp_user + ":" + config.smtp_pass + "\"";
    curl_cmd += " --upload-file \"" + tmp_file + "\"";

    int ret = system(curl_cmd.c_str());

    // Cleanup
    fs::remove(tmp_file);

    // Log the result
    bool success = (ret == 0);
    write_log(config.sender, email, success);

    if (success) {
        std::cout << "邮件发送成功" << std::endl;
        return 0;
    } else {
        std::cout << "邮件发送失败，请检查SMTP配置和网络连接" << std::endl;
        return 1;
    }
}
